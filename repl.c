#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "repl.h"

struct repl_cmd repl_cmds[REPL_CMD_MAX] = {0};

static int repl_getline(char *line, size_t maxlen);
static int repl_dispatch(const char *line);

void repl_enter() {
    char line[REPL_LINE_MAXLEN];

    printf("--- REPL ---\n");
    while (1) {
        memset((void*)line, 0, sizeof(line));
        if (repl_getline(line, REPL_LINE_MAXLEN) != 0) {
            break;
        }

        //printf("< %s\n", line);
        switch (repl_dispatch(line)) {
        case 0:
            return;
        default:
            break;
        }
    }
}

static int repl_getline(char *line, size_t maxlen) {
    size_t len = 0;
    int c;

    if (line == NULL) {
        return -1;
    }

    putchar('>');
    putchar(' ');

    while (1) {
        // read until we hit the end of the stream
        c = getchar();
        if (c == EOF) {
            break;
        }

        // the Pi Pico reads a carriage return (CR) on enter key press via USB CDC serial
        if (c == '\n' || c == '\r') {
            putchar('\n');
            break;
        }

        // echo the read character
        putchar(c);

        if (len == (maxlen - 1)) {
            // ran out of buffer space
            return -1;
        }

        // save the char to the line buffer
        len += 1;
        line[len - 1] = (char)c;
    }

    if (len > 0) {
        line[len] = '\0';
    }

    return 0;
}

static int repl_dispatch(const char *line) {
    if (strncmp(line, "quit", strlen("quit")) == 0 || strncmp(line, "q", strlen("q")) == 0) {
        printf("< quit\n");
        return 0;
    }

    for (int i = 0; i < REPL_CMD_MAX; i++) {
        if (repl_cmds[i].cmd && strncmp(line, repl_cmds[i].cmd, strlen(repl_cmds[i].cmd)) == 0) {
            repl_cmds[i].handler(line + strlen(repl_cmds[i].cmd) + 1);
        }
    }

    return 1;
}
