#ifndef _ROUTE_ROOT_H_
#define _ROUTE_ROOT_H_

#include "httpserver.h"

void http_get_root(const http_request_t *req, http_response_t *res);

static const http_route_t http_route_get_root = {
    .method_and_path = "GET /",
    .handler = http_get_root,
};

#endif // _ROUTE_ROOT_H_
