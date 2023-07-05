#include "route_root.h"

void http_get_root(const http_request_t *req, http_response_t *res) {
    int rc;

    // Generate result
    const char *body = ""
        "<html>"
            "<body>"
                "<p><a href=/led>LED</a>"
                "<p><a href=/fs>FS</a>"
            "</body>"
        "</html>";

    rc = snprintf(res->body, res->body_maxlen, body);
    if (rc < res->body_maxlen) {
        res->body_len = rc;
    }

    // Generate header
    rc = snprintf(res->header, res->header_maxlen, HTTP_RESPONSE_OK, 200, res->body_len);
    if (rc < res->header_maxlen) {
        res->header_len = rc;
    }
}
