#include "fs.h"

#include "route_fs.h"

static void get_fs_dir(const char *path, http_response_t *res);
static void get_fs_file(const char *path, http_response_t *res);

static int extract_path(const char *request, char *path, size_t maxlen) {
    const char *needle = "/fs";
    const char *begin = strstr(request,needle) + strlen(needle);
    size_t path_len = strcspn(begin, " ");

    if (path_len == 0) {
        path[0] = '/';
        path[1] = '\0';
        return 1;
    } else if (path_len >= maxlen) {
        return 0;
    }

    strncpy(path, begin, path_len);
    path[path_len] = '\0';
    return path_len;
}

void http_get_fs(const http_request_t *req, http_response_t *res) {
    const char *body_start = "<html><body>";
    const char *body_end = "</body></html>";
    char path[1024];
    struct lfs_info info;
    int rc;

    rc = extract_path(req->header, path, sizeof(path) - 1);
    if (rc == 0) {
        printf("< %s: path too long %s\n", __func__);
        return;
    }

    rc = lfs_stat(&lfs, path, &info);
    if (rc < 0) {
        printf("< %s: failed to stat %s\n", __func__, path);
        return;
    }

    rc = snprintf(res->body + res->body_len, res->body_maxlen - res->body_len, body_start);
    if (rc < res->body_maxlen - res->body_len) {
        res->body_len += rc;
    }

    switch (info.type) {
    case LFS_TYPE_REG:
        get_fs_file(path, res);
        break;
    case LFS_TYPE_DIR:
        get_fs_dir(path, res);
        break;
    default:
        printf("< %s: cannot handle file type for %s\n", __func__, path);
        break;
    }

    rc = snprintf(res->body + res->body_len, res->body_maxlen - res->body_len, body_end);
    if (rc < res->body_maxlen - res->body_len) {
        res->body_len += rc;
    }

    // Generate header
    rc = snprintf(res->header, res->header_maxlen, HTTP_RESPONSE_OK, 200, res->body_len);
    if (rc < res->header_maxlen) {
        res->header_len = rc;
    }
}

static void get_fs_dir(const char *path, http_response_t *res) {
    lfs_dir_t dir;
    struct lfs_info info;
    int rc;

    rc = lfs_dir_open(&lfs, &dir, path);
    if (rc < 0) {
        printf("< %s: failed to open dir: %s\n", __func__, path);
        return;
    }

    while (1) {
        rc = lfs_dir_read(&lfs, &dir, &info);
        if (rc < 0) {
            printf("< %s: failed to read dir: %s\n", __func__, path);
            lfs_dir_close(&lfs, &dir);
            return;
        } else if (rc == 0) {
            break;
        }

        if (info.type == LFS_TYPE_REG) {
            const char *entry = "<p><a href=\"/fs%s%s\">%s</a></p>";
            rc = snprintf(res->body + res->body_len, res->body_maxlen - res->body_len, entry,
                          path, info.name, info.name);
            if (rc < res->body_maxlen - res->body_len) {
                res->body_len += rc;
            }
        }
    }

    lfs_dir_close(&lfs, &dir);
}

static void get_fs_file(const char *path, http_response_t *res) {
    const char *html;
    lfs_file_t file;
    lfs_ssize_t read;
    int rc;

    rc = lfs_file_open(&lfs, &file, path, LFS_O_RDONLY);
    if (rc < 0) {
        printf("< %s: failed to open file: %s\n", __func__, path);
        return;
    }

    // write the first part of the HTML form
    html = ""
        "<form action=\"/fs%s\" method=\"POST\" enctype=\"text/plain\">"
            "<p><textarea name=\"text\">";
    rc = snprintf(res->body + res->body_len, res->body_maxlen - res->body_len, html, path);
    if (rc < res->body_maxlen - res->body_len) {
        res->body_len += rc;
    }

    // inject the file contents into the HTML code
    read = lfs_file_read(&lfs, &file, res->body + res->body_len, res->body_maxlen - res->body_len);
    if (read < 0) {
        printf("< %s: failed to read file: %s\n", __func__, path);
        lfs_file_close(&lfs, &file);
        return;
    }
    res->body_len += read;

    // write the second part of the HTML form
    html = ""
            "</textarea></p>"
            "<p><input type=\"submit\" value=\"Save\"></p>"
        "</form>";
    rc = snprintf(res->body + res->body_len, res->body_maxlen - res->body_len, html);
    if (rc < res->body_maxlen - res->body_len) {
        res->body_len += rc;
    }

    lfs_file_close(&lfs, &file);
}

void http_post_fs(const http_request_t *req, http_response_t *res) {
    char path[1024];
    const char *key = "text=";
    const char *value;
    lfs_file_t file;
    int file_size;
    lfs_ssize_t written;
    int rc;

    rc = extract_path(req->header, path, sizeof(path) - 1);
    if (rc == 0) {
        printf("< %s: path too long %s\n", __func__);
        return;
    }

    value = strstr(req->body, key);
    if (value == NULL) {
        printf("< %s: failed to find key (%s) in request body\n", __func__, key);
        return;
    }
    value += strlen(key);
    file_size = strlen(value);

    rc = lfs_file_open(&lfs, &file, path, LFS_O_WRONLY);
    if (rc < 0) {
        printf("< %s: failed to open file: %s\n", __func__, path);
        return;
    }

    rc = lfs_file_truncate(&lfs, &file, file_size);
    if (rc < 0) {
        printf("< %s: failed to truncate file: %s\n", __func__, path);
        lfs_file_close(&lfs, &file);
        return;
    }

    // inject the file contents into the HTML code
    written = lfs_file_write(&lfs, &file, value, file_size);
    if (written < 0) {
        printf("< %s: failed to write to file: %s\n", __func__, path);
        lfs_file_close(&lfs, &file);
        return;
    }

    // Generate header
    char location[1024];
    int location_len = strlen("/fs") + strlen(path);
    rc = snprintf(location, sizeof(location) - 1, "/fs%s", path);
    if (rc != location_len) {
        printf("< %s: failed to construct HTML redirect location: %s\n", __func__, path);
        lfs_file_close(&lfs, &file);
    }

    rc = snprintf(res->header, res->header_maxlen, HTTP_RESPONSE_REDIRECT, 302, location);
    if (rc < res->header_maxlen) {
        res->header_len = rc;
    }

    lfs_file_close(&lfs, &file);
}
