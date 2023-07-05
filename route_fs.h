#ifndef _ROUTE_FS_H_
#define _ROUTE_FS_H_

#include "httpserver.h"

void http_get_fs(const http_request_t *req, http_response_t *res);
void http_post_fs(const http_request_t *req, http_response_t *res);

static const http_route_t http_route_get_fs = {
    .method_and_path = "GET /fs",
    .handler = http_get_fs,
};

static const http_route_t http_route_post_fs = {
    .method_and_path = "POST /fs",
    .handler = http_post_fs,
};

#endif // _ROUTE_FS_H_
