#include <stdlib.h>

#include "EPD_5in83b_V2.h"
#include "GUI_Paint.h"

#include "epaper.h"
#include "epaper_gui.h"
#include "repl.h"
#include "repl_epaper.h"

static void repl_epaper_clear(const char *line);
static void repl_epaper_sleep(const char *line);
static void repl_epaper_test(const char *line);
static void repl_epaper_gui(const char *line);

static struct repl_cmd LUT[] = {
    {
        .cmd = "clear",
        .handler = repl_epaper_clear,
    },
    {
        .cmd = "sleep",
        .handler = repl_epaper_sleep,
    },
    {
        .cmd = "test",
        .handler = repl_epaper_test,
    },
    {
        .cmd = "gui",
        .handler = repl_epaper_gui,
    },
};

void repl_epaper(const char *line) {
    if (repl_dispatch(line, LUT, sizeof(LUT) / sizeof(LUT[0])) == 0) {
        printf("< epaper: unknown cmd: %s\n", line);
    }
}

static int epaper_clear() {
    epaper_init();

    EPD_5IN83B_V2_Clear();
    DEV_Delay_ms(500);
    return 0;
}

static void repl_epaper_clear(const char *line) {
    printf("< epaper: clearing\n");
    epaper_clear();
}

static void repl_epaper_sleep(const char *line) {
    printf("< epaper: going to sleep\n");

    if (epaper_sleep() != 0) {
        printf("< epaper: failed to enter sleep\n");
    }
}

static void repl_epaper_test(const char *line) {
    void *img_black, *img_red;
    uint16_t width, height;

    width = (EPD_5IN83B_V2_WIDTH % 8 == 0) ? (EPD_5IN83B_V2_WIDTH / 8) : (EPD_5IN83B_V2_WIDTH / 8 + 1);
    height = EPD_5IN83B_V2_HEIGHT;

    epaper_init();

    // see https://github.com/waveshareteam/Pico_ePaper_Code/blob/main/c/lib/e-Paper/EPD_5in83b_V2.c#L171
    // EPD_5IN83B_V2_Display()
    printf("< epaper: writing black\n");

    img_black = malloc(width * height);
    if (img_black == NULL) {
        printf("< epaper: malloc() failed for black\n");
        return;
    }

    img_red = malloc(width * height);
    if (img_red == NULL) {
        printf("< epaper: malloc() failed for red\n");
        free(img_black);
        return;
    }

    Paint_NewImage(img_black, EPD_5IN83B_V2_WIDTH, EPD_5IN83B_V2_HEIGHT, 0, WHITE);
    Paint_NewImage(img_red, EPD_5IN83B_V2_WIDTH, EPD_5IN83B_V2_HEIGHT, 0, WHITE);

    Paint_SelectImage(img_black);
    Paint_Clear(WHITE);
    Paint_DrawPoint(10, 80, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 90, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 100, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 110, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    Paint_DrawLine(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(70, 70, 20, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);      
    Paint_DrawRectangle(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(80, 70, 130, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(10, 0, "waveshare", &Font16, BLACK, WHITE);    
    Paint_DrawString_CN(130, 20, "Î¢Ñ©µç×Ó", &Font24CN, WHITE, BLACK);
    Paint_DrawNum(10, 50, 987654321, &Font16, WHITE, BLACK);
    
    //2.Draw red image
    Paint_SelectImage(img_red);
    Paint_Clear(WHITE);
    Paint_DrawCircle(160, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(210, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawLine(85, 95, 125, 95, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(105, 75, 105, 115, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);  
    Paint_DrawString_CN(130, 0,"ÄãºÃAbc", &Font12CN, BLACK, WHITE);
    Paint_DrawString_EN(10, 20, "hello world", &Font12, WHITE, BLACK);
    Paint_DrawNum(10, 33, 123456789, &Font12, BLACK, WHITE);

    EPD_5IN83B_V2_Display(img_black, img_red);
    DEV_Delay_ms(2000);

    free(img_red);
    free(img_black);
}

static void repl_epaper_gui(const char *line) {
    printf("< epaper: drawing GUI\n");
    epaper_gui_draw();
}
