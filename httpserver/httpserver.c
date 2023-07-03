#include "string.h"

#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "httpserver.h"

#define TCP_PORT 80
#define DEBUG_printf printf
#define POLL_TIME_S 5
#define HTTP_RESPONSE_HEADERS "HTTP/1.1 %d OK\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"
#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 302 Redirect\nLocation: http://%s/\n\n"

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
    http_route_fn handler = NULL;
    char header[1024];

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
    // Copy the request into the buffer
    pbuf_copy_partial(p, header, p->tot_len > sizeof(header) - 1 ? sizeof(header) - 1 : p->tot_len, 0);

    // Invoke the appropriate route handler
    con->result_len = 0;
    for (int i = 0; i < server->routes_len; i++) {
        if (strncmp(server->routes[i].method_and_path, header,
                    strlen(server->routes[i].method_and_path)) == 0) {
            handler = server->routes[i].handler;
            break;
        }
    }

    if (handler) {
        // Copy the full request and pass it to the handler
        pbuf_copy_partial(p, con->request,
                          p->tot_len > sizeof(con->request) - 1 ? sizeof(con->request) - 1 : p->tot_len, 0);
        con->result_len = handler(con->request, p->tot_len, con->result, sizeof(con->result));
    }

    // Generate content
    DEBUG_printf("Request: %s\n", con->request);
    DEBUG_printf("Result: %s\n", con->result);

    // Check we had enough buffer space
    if (con->result_len > sizeof(con->result) - 1) {
        DEBUG_printf("Too much result data %d\n", con->result_len);
        return tcp_close_client_connection(con, pcb, ERR_CLSD);
    }

    // Generate web page
    if (con->result_len > 0) {
        con->header_len = snprintf(header, sizeof(header), HTTP_RESPONSE_HEADERS,
            200, con->result_len);
        if (con->header_len > sizeof(header) - 1) {
            DEBUG_printf("Too much header data %d\n", con->header_len);
            return tcp_close_client_connection(con, pcb, ERR_CLSD);
        }
    } else {
        // Send redirect
        con->header_len = snprintf(header, sizeof(header), HTTP_RESPONSE_REDIRECT,
            ipaddr_ntoa(con->gw));
        DEBUG_printf("Sending redirect %s", header);
    }

    // Send the headers to the client
    con->sent_len = 0;
    err = tcp_write(pcb, header, con->header_len, 0);
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
