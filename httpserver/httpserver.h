#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include "pico/async_context.h"
#include "lwip/ip_addr.h"

typedef int (*http_route_fn)(const char*, int, char*, size_t);

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
    int header_len;
    int result_len;
    ip_addr_t *gw;
} http_connection_t;

typedef enum http_method_t_ {
    HTTP_GET,
} http_method_t;

bool http_server_open(http_server_t *server);
void http_server_close(http_server_t *server);

#endif // _HTTPSERVER_H_
