#include "string.h"

#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "httpserver.h"

#define TCP_PORT 80
#define DEBUG_printf printf
#define POLL_TIME_S 5

static err_t tcp_close_client_connection(http_connection_t *con, struct tcp_pcb *client_pcb, err_t close_err) {
    if (client_pcb) {
        assert(con && con->pcb == client_pcb);
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);
        err_t err = tcp_close(client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(client_pcb);
            close_err = ERR_ABRT;
        }
        if (con) {
            free(con);
        }
    }
    return close_err;
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    http_connection_t *con = (http_connection_t*)arg;
    DEBUG_printf("tcp_server_sent %u\n", len);
    con->sent_len += len;
    if (con->sent_len >= con->header_len + con->result_len) {
        DEBUG_printf("all done\n");
        return tcp_close_client_connection(con, pcb, ERR_OK);
    }
    return ERR_OK;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    http_connection_t *con = (http_connection_t*)arg;
    http_server_t *server = con->server;
    http_route_t *route = NULL;
    char response_header[1024];
    int rx_len, rx_maxlen;

    if (!p) {
        DEBUG_printf("connection closed\n");
        return tcp_close_client_connection(con, pcb, ERR_OK);
    }
    assert(con && con->pcb == pcb);

    if (p->tot_len <= 0) {
        pbuf_free(p);
        return ERR_OK;
    }

    DEBUG_printf("tcp_server_recv %d err %d\n", p->tot_len, err);
#if 0
    for (struct pbuf *q = p; q != NULL; q = q->next) {
        DEBUG_printf("in: %.*s\n", q->len, q->payload);
    }
#endif

    // Warn if we do not have enough space to store the payload
    rx_maxlen = sizeof(con->request) - con->request_len - 1;
    rx_len = p->tot_len > rx_maxlen ? rx_maxlen : p->tot_len;
    if (p->tot_len > rx_len) {
        DEBUG_printf("warning: not enough space for payload (%d vs %d)\n", p->tot_len, rx_len);
    }

    // Copy the request into the buffer
    pbuf_copy_partial(p, con->request + con->request_len, rx_len, 0);
    con->request_len += rx_len;

    // Detect request body (if any)
    const char *request_body_marker = "\r\n\r\n";
    char *request_body = strstr(con->request, request_body_marker);
    while (request_body && strncmp(request_body, request_body_marker, strlen(request_body_marker)) == 0) {
        request_body += strlen(request_body_marker);
    }

    if (request_body >= con->request + con->request_len) {
        request_body = NULL;
    } else {
        // Sanitize request header and body
        char *cursor;
        while ((cursor = strstr(con->request, request_body_marker)) != NULL) {
            for (size_t i = 0; i < strlen(request_body_marker); i++) {
                // Sanitize request header
                cursor[i] = '\0';
            }
        }
    }

    if ((strstr(con->request, "Content-Length:") != NULL) && (request_body == NULL)) {
        // We expect the request body next, so start another tcp RX
        tcp_recved(pcb, p->tot_len);
        pbuf_free(p);
        return ERR_OK;
    } else {
        // Indicate that we are ready to receive another request
        con->request_len = 0;
    }

    // Create intermediate helper structs
    const http_request_t req = {
        .header = con->request,
        .body = request_body,
    };
    http_response_t res = {
        .header = response_header,
        .header_len = 0,
        .header_maxlen = sizeof(response_header) - 1,
        .body = con->result,
        .body_len = 0,
        .body_maxlen = sizeof(con->result) - 1,
    };

    // Invoke the appropriate route handler
    for (int i = 0; i < server->routes_len; i++) {
        if (strncmp(server->routes[i].method_and_path, req.header,
                    strlen(server->routes[i].method_and_path)) == 0) {
            route = &server->routes[i];
            break;
        }
    }

    if (!route) {
        DEBUG_printf("No handler found for request: %s\n", req.header);
        return tcp_close_client_connection(con, pcb, ERR_CLSD);
    }

    // Debug
    DEBUG_printf("---\n");
    DEBUG_printf("Request Header: \n%s\n", req.header);
    DEBUG_printf("---\n");
    DEBUG_printf("Request Body: \n%s\n", req.body);
    DEBUG_printf("---\n");

    // Invoke the handler which will populate the result struct
    route->handler(&req, &res);
    con->result_len = res.body_len;
    con->header_len = res.header_len;

    // Debug
    DEBUG_printf("---\n");
    DEBUG_printf("Response Header: \n%s\n", res.header);
    DEBUG_printf("---\n");
    DEBUG_printf("Response Body: \n%s\n", res.body);
    DEBUG_printf("---\n");

    if (con->header_len == 0) {
        DEBUG_printf("No response header generated for route: %s\n", route->method_and_path);
        return tcp_close_client_connection(con, pcb, ERR_CLSD);
    }

    // Send the headers to the client
    con->sent_len = 0;
    err = tcp_write(pcb, res.header, con->header_len, 0);
    if (err != ERR_OK) {
        DEBUG_printf("failed to write header data %d\n", err);
        return tcp_close_client_connection(con, pcb, err);
    }

    // Send the body to the client
    if (con->result_len) {
        err = tcp_write(pcb, con->result, con->result_len, 0);
        if (err != ERR_OK) {
            DEBUG_printf("failed to write result data %d\n", err);
            return tcp_close_client_connection(con, pcb, err);
        }
    }
    tcp_recved(pcb, p->tot_len);

    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *pcb) {
    http_connection_t *con = (http_connection_t*)arg;
    DEBUG_printf("tcp_server_poll_fn\n");
    return tcp_close_client_connection(con, pcb, ERR_OK); // Just disconnect clent?
}

static void tcp_server_err(void *arg, err_t err) {
    http_connection_t *con = (http_connection_t*)arg;
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_close_client_connection(con, con->pcb, err);
    }
}

static err_t http_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    http_server_t *server = (http_server_t*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("failure in accept\n");
        return ERR_VAL;
    }
    DEBUG_printf("client connected\n");

    // Create the state for the connection
    http_connection_t *con = calloc(1, sizeof(http_connection_t));
    if (!con) {
        DEBUG_printf("failed to allocate connection\n");
        return ERR_MEM;
    }
    con->server = server;
    con->pcb = client_pcb; // for checking
    con->gw = &server->gw;

    // setup connection to client
    tcp_arg(client_pcb, con);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}

bool http_server_open(http_server_t *server) {
    DEBUG_printf("starting server on port %d\n", TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
    if (err) {
        DEBUG_printf("failed to bind to port %d\n",TCP_PORT);
        return false;
    }

    server->pcb = tcp_listen_with_backlog(pcb, 1);
    if (!server->pcb) {
        DEBUG_printf("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(server->pcb, server);
    tcp_accept(server->pcb, http_server_accept);

    printf("Try connecting to '%s' (press 'd' to disable access point)\n");
    return true;
}

void http_server_close(http_server_t *server) {
    if (server->pcb) {
        tcp_arg(server->pcb, NULL);
        tcp_close(server->pcb);
        server->pcb = NULL;
    }
}
