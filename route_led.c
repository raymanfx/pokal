#include "pico/cyw43_arch.h"

#include "route_led.h"

void http_get_led(const http_request_t *req, http_response_t *res) {
    char *params = strchr(req->header, '?');
    int rc;

    if (params) {
        if (*params) {
            char *space = strchr(req->header, ' ');
            *params++ = 0;
            if (space) {
                *space = 0;
            }
        } else {
            params = NULL;
        }
    }

    // Get the state of the led
    bool value;
    int gpio = 0;
    cyw43_gpio_get(&cyw43_state, gpio, &value);
    int led_state = value;

    // See if the user changed it
    if (params) {
        int led_param = sscanf(params, "led=%d", &led_state);
        if (led_param == 1) {
            if (led_state) {
                // Turn led on
                cyw43_gpio_set(&cyw43_state, gpio, true);
            } else {
                // Turn led off
                cyw43_gpio_set(&cyw43_state, gpio, false);
            }
        }
    }

    // Generate result
    const char *body = ""
        "<html>"
            "<body>"
                "<h1>Hello from Pico W.</h1>"
                "<p>Led is %s</p>"
                "<p><a href=\"?led=%d\">Turn led %s</a>"
            "</body>"
        "</html>";
    if (led_state) {
        rc = snprintf(res->body, res->body_maxlen, body, "ON", 0, "OFF");
        if (rc < res->body_maxlen) {
            res->body_len = rc;
        }
    } else {
        rc = snprintf(res->body, res->body_maxlen, body, "OFF", 1, "ON");
        if (rc < res->body_maxlen) {
            res->body_len = rc;
        }
    }

    // Generate header
    rc = snprintf(res->header, res->header_maxlen, HTTP_RESPONSE_OK, 200, res->body_len);
    if (rc < res->header_maxlen) {
        res->header_len = rc;
    }
}
