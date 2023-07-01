#include "fs.h"
#include "ina219.h"

#include "repl.h"
#include "repl_ups.h"

static void repl_ups_voltage(const char *line);
static void repl_ups_current(const char *line);
static void repl_ups_percent(const char *line);

static struct repl_cmd LUT[] = {
    {
        .cmd = "v",
        .handler = repl_ups_voltage,
    },
    {
        .cmd = "ma",
        .handler = repl_ups_current,
    },
    {
        .cmd = "%",
        .handler = repl_ups_percent,
    },
};

void repl_ups(const char *line) {
    if (repl_dispatch(line, LUT, sizeof(LUT) / sizeof(LUT[0])) == 0) {
        printf("< ups: unknown cmd: %s\n", line);
    }
}

static void repl_ups_voltage(const char *line) {
    printf("%f V\n", ina219_voltage(&g_ina219));
}

static void repl_ups_current(const char *line) {
    printf("%f mA\n", ina219_current(&g_ina219));
}

static void repl_ups_percent(const char *line) {
    printf("%f mA\n", ina219_percentage(&g_ina219));
}
