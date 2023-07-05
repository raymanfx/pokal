#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include "pico/async_context.h"
#include "lwip/ip_addr.h"

typedef struct http_request_t_ {
    const char *header;
    const char *body;
} http_request_t;

typedef struct http_response_t_ {
    char *header;
    size_t header_len;
    size_t header_maxlen;
    char *body;
    size_t body_len;
    size_t body_maxlen;
} http_response_t;

typedef void (*http_route_fn)(const http_request_t*, http_response_t*);

typedef struct http_route_t_ {
    const char *method_and_path;
    http_route_fn handler;
} http_route_t;

typedef struct http_server_t_ {
    struct tcp_pcb *pcb;
    ip_addr_t gw;
    async_context_t *context;
    http_route_t *routes;
    int routes_len;
} http_server_t;

typedef struct http_connection_t_ {
    http_server_t *server;
    struct tcp_pcb *pcb;
    int sent_len;
    char request[10240];
    char result[10240];
    int request_len;
    int header_len;
    int result_len;
    ip_addr_t *gw;
} http_connection_t;

typedef enum http_method_t_ {
    HTTP_GET,
} http_method_t;

bool http_server_open(http_server_t *server);
void http_server_close(http_server_t *server);

#define HTTP_RESPONSE_OK "HTTP/1.1 %d OK\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"
#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 %d Redirect\nLocation: %s\n\n"

#endif // _HTTPSERVER_H_
