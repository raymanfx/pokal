#include "EPD_5in83b_V2.h"

#include "epaper.h"

static bool g_once = false;
static bool g_active = false;

int epaper_init() {
    if (g_active) {
        return 0;
    }

    if (!g_once) {
        if (DEV_Module_Init() != 0){
            return -1;
        }
        g_once = true;
    }

    EPD_5IN83B_V2_Init();
    DEV_Delay_ms(500);
    g_active = true;

    return 0;
}

int epaper_sleep() {
    if (!g_active) {
        return 0;
    }

    EPD_5IN83B_V2_Sleep();
    DEV_Delay_ms(2000);//important, at least 2s
    g_active = false;

    return 0;
}
