#ifndef _ROUTE_LED_H_
#define _ROUTE_LED_H_

#include "httpserver.h"

void http_get_led(const http_request_t *req, http_response_t *res);

static const http_route_t http_route_get_led = {
    .method_and_path = "GET /led",
    .handler = http_get_led,
};

#endif // _ROUTE_LED_H_
