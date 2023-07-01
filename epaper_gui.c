#include <stdlib.h>
#include <string.h>

#include "EPD_5in83b_V2.h"
#include "GUI_Paint.h"

#include "ina219.h"

#include "epaper.h"
#include "epaper_gui.h"

void epaper_gui_draw() {
    void *img_black = NULL, *img_red = NULL;
    uint16_t width, height;
    float battery;
    char battery_string[20];

    width = (EPD_5IN83B_V2_WIDTH % 8 == 0) ? (EPD_5IN83B_V2_WIDTH / 8) : (EPD_5IN83B_V2_WIDTH / 8 + 1);
    height = EPD_5IN83B_V2_HEIGHT;

    epaper_init();

    img_black = malloc(width * height);
    if (img_black == NULL) {
        printf("< epaper: gui: malloc() failed for black\n");
        goto error;
    }

    img_red = malloc(width * height);
    if (img_red == NULL) {
        printf("< epaper: gui: malloc() failed for red\n");
        goto error;
    }

    Paint_NewImage(img_black, EPD_5IN83B_V2_WIDTH, EPD_5IN83B_V2_HEIGHT, 0, WHITE);
    Paint_NewImage(img_red, EPD_5IN83B_V2_WIDTH, EPD_5IN83B_V2_HEIGHT, 0, WHITE);

    // collect UI elements
    battery = ina219_percentage(&g_ina219);
    sprintf(battery_string, "Akku: %d", (int)battery);

    // black part
    Paint_SelectImage(img_black);
    Paint_Clear(WHITE);
    Paint_DrawString_EN(0, 0, battery_string, &Font16,
            BLACK, WHITE);

    // red part
    Paint_SelectImage(img_red);
    Paint_Clear(WHITE);

    EPD_5IN83B_V2_Display(img_black, img_red);
    DEV_Delay_ms(2000);

    // send epaper display into deep sleep
    // FIXME - unable to recover after sleep (?)
    // epaper_sleep();

error:
    if (img_red) {
        free(img_red);
    }
    if (img_black) {
        free(img_black);
    }
}
