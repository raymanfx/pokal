#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "repl.h"

static int repl_getline(char *line, size_t maxlen);

void repl_enter(const struct repl_cmd cmds[], size_t len) {
    char line[REPL_LINE_MAXLEN];

    printf("--- REPL ---\n");
    while (1) {
        memset((void*)line, 0, sizeof(line));
        if (repl_getline(line, REPL_LINE_MAXLEN) != 0) {
            break;
        }

        // special-case this command so we can bail out of the REPL
        if (strncmp(line, "quit", strlen("quit")) == 0 || strncmp(line, "q", strlen("q")) == 0) {
            printf("< quit\n");
            return;
        }

        // dispatch other commands to their respective handlers
        if (repl_dispatch(line, cmds, len) == 0) {
            printf("< repl: unknown cmd: %s\n", line);
        }
    }
}

int repl_dispatch(const char *line, const struct repl_cmd cmds[], size_t len) {
    int count = 0;

    for (int i = 0; i < len; i++) {
        if (strncmp(line, cmds[i].cmd, strlen(cmds[i].cmd)) == 0) {
            cmds[i].handler(line + strlen(cmds[i].cmd) + 1);
            count++;
        }
    }

    return count;
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
