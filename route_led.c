#include "pico/cyw43_arch.h"

#include "route_led.h"

int http_get_led(const char *request, int request_len, char *result, size_t max_result_len) {
    int len = 0;
    char *params = strchr(request, '?');

    if (params) {
        if (*params) {
            char *space = strchr(request, ' ');
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
        len = snprintf(result, max_result_len, body, "ON", 0, "OFF");
    } else {
        len = snprintf(result, max_result_len, body, "OFF", 1, "ON");
    }

    return len;
}
