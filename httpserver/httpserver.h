#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include "pico/async_context.h"
#include "lwip/ip_addr.h"

typedef struct http_server_t_ {
    struct tcp_pcb *pcb;
    ip_addr_t gw;
    async_context_t *context;
} http_server_t;

typedef struct http_connection_t_ {
    struct tcp_pcb *pcb;
    int sent_len;
    char headers[128];
    char result[256];
    int header_len;
    int result_len;
    ip_addr_t *gw;
} http_connection_t;

bool http_server_open(http_server_t *server);
void http_server_close(http_server_t *server);

#endif // _HTTPSERVER_H_
