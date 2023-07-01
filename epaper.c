#include "EPD_5in83b_V2.h"

#include "epaper.h"

int epaper_init() {
    if (DEV_Module_Init() != 0){
        return -1;
    }

    EPD_5IN83B_V2_Init();
    DEV_Delay_ms(500);
    return 0;
}

int epaper_sleep() {
    epaper_init();

    EPD_5IN83B_V2_Sleep();
    return 0;
}
