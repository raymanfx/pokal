#include "fs.h"

#include "repl_fs.h"

static void repl_fs_ls(const char *path);
static void repl_fs_cat(const char *path);

void repl_fs(const char *line) {
    if (strncmp(line, "ls", strlen("ls")) == 0) {
        repl_fs_ls(line + strlen("ls") + 1);
    } else if (strncmp(line, "cat", strlen("cat")) == 0) {
        repl_fs_cat(line + strlen("cat") + 1);
    } else {
        printf("< fs: unknown cmd: %s\n", line);
    }
}

static void repl_fs_ls(const char *path) {
    lfs_dir_t dir;
    struct lfs_info info;
    int rc;

    rc = lfs_dir_open(&lfs, &dir, path);
    if (rc < 0) {
        printf("< fs: ls: failed to open dir: %s\n", path);
        return;
    }

    while (1) {
        rc = lfs_dir_read(&lfs, &dir, &info);
        if (rc < 0) {
            printf("< fs: ls: failed to read dir: %s\n", path);
            lfs_dir_close(&lfs, &dir);
            return;
        } else if (rc == 0) {
            break;
        }

        switch (info.type) {
        case LFS_TYPE_REG:
            printf("reg ");
            break;
        case LFS_TYPE_DIR:
            printf("dir ");
            break;
        default:
            printf("?   ");
            break;
        }

        printf("%s\n", info.name);
    }

    lfs_dir_close(&lfs, &dir);
}

static void repl_fs_cat(const char *path) {
    lfs_file_t file;
    char buf[10];
    int rc;
    lfs_ssize_t read;

    rc = lfs_file_open(&lfs, &file, path, LFS_O_RDONLY);
    if (rc < 0) {
        printf("< fs: cat: failed to open file: %s\n", path);
        return;
    }

    while (1) {
        read = lfs_file_read(&lfs, &file, &buf, sizeof(buf));
        if (read < 0) {
            printf("< fs: cat: failed to read file: %s\n", path);
            lfs_file_close(&lfs, &file);
            return;
        } else if (read == 0) {
            break;
        }

        for (int i = 0; i < read; i++) {
            printf("%2x ", buf[i]);
        }
    }
    printf("\n");

    lfs_file_close(&lfs, &file);
}
