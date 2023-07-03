#include "route_root.h"

int http_get_root(const char *request, int request_len, char *result, size_t max_result_len) {
    // Generate result
    const char *body = ""
        "<html>"
            "<body>"
                "<p><a href=/led>LED</a>"
            "</body>"
        "</html>";

    return snprintf(result, max_result_len, body);
}
