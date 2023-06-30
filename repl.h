#ifndef POKAL_REPL_H
#define POKAL_REPL_H

#define REPL_LINE_MAXLEN 1024
#define REPL_CMD_MAX 10

struct repl_cmd {
    const char *cmd;
    void (*handler)(const char*);
};

extern struct repl_cmd repl_cmds[REPL_CMD_MAX];

void repl_enter();

#endif // POKAL_REPL_H
