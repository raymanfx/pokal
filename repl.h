#ifndef POKAL_REPL_H
#define POKAL_REPL_H

#define REPL_LINE_MAXLEN 1024

struct repl_cmd {
    const char *cmd;
    void (*handler)(const char*);
};

void repl_enter(const struct repl_cmd cmds[], size_t len);
int repl_dispatch(const char *line, const struct repl_cmd cmds[], size_t len);

#endif // POKAL_REPL_H
