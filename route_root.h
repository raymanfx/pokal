#ifndef _ROUTE_ROOT_H_
#define _ROUTE_ROOT_H_

#include "httpserver.h"

int http_get_root(const char *request, int request_len, char *result, size_t max_result_len);

static const http_route_t http_route_get_root = {
    .method_and_path = "GET /",
    .handler = http_get_root,
};

#endif // _ROUTE_ROOT_H_
