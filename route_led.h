#ifndef _ROUTE_LED_H_
#define _ROUTE_LED_H_

#include "httpserver.h"

int http_get_led(const char *request, int request_len, char *result, size_t max_result_len);

static const http_route_t http_route_get_led = {
    .method_and_path = "GET /led",
    .handler = http_get_led,
};

#endif // _ROUTE_LED_H_
