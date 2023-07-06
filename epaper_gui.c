#include <stdlib.h>
#include <string.h>

#include "EPD_5in83b_V2.h"
#include "GUI_Paint.h"
#include "Bitmapdata.c"

#include "ina219.h"

#include "epaper.h"
#include "epaper_gui.h"
#include "tableread.h"

extern const unsigned char bsHeadliene[];
extern const unsigned char gImage_5in83_V2[];

void epaper_gui_draw() {
    void *img_black = NULL, *img_red = NULL;
    uint16_t width, height;
    float battery;
    int gamestotal;
    char winneroftheyear[20];
    char lastwinner[40];
    char battery_string[20];
    char gamestotal_string[20];
    char gamesyear_string[20];
    char dataBuffer[20];

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
    readFile("db.txt");

    battery = ina219_percentage(&g_ina219);
    sprintf(battery_string, "%d", (int)battery);

    sprintf(gamesyear_string, "Spiele %d: %d",(int)getYearNow(), (int)getGamesInTheYear(getYearNow()));

    gamestotal = getTableRowCount();
    sprintf(gamestotal_string, "Gesamte Spiele: %d", (int)gamestotal);

    for (int j = 0; j < 20; j++) {
        lastwinner[j]=*(getTableData(gamestotal-1,0) + j);
    }
    lastwinner[8]=':';
    lastwinner[9]=' ';
    for (int j = 0; j < 20; j++) {
        lastwinner[j+10]=*(getTableData(gamestotal-1,2) + j);
    }

    sprintf(winneroftheyear, "%i: %s",(int)getYearNow(), "Pia");

    // black part
    Paint_SelectImage(img_black);
    Paint_Clear(WHITE);

    Paint_DrawBitMap(blackData);

    Paint_DrawString_EN(15, 8, battery_string, &Font16,WHITE, BLACK);
    Paint_DrawString_EN(70, 110, gamesyear_string, &Font16,WHITE, BLACK);
    Paint_DrawString_EN(70, 165, gamestotal_string, &Font16,WHITE, BLACK);
    Paint_DrawString_EN(370, 110, lastwinner, &Font20,WHITE, BLACK);
    Paint_DrawString_EN(370, 165, winneroftheyear, &Font20,WHITE, BLACK);

    //table
    int  y=235;
    int  y2=y-50;
    char headline[4][11] = {"Datum", "Spiel", "GewinnerIn", "Score"};
    for(int i;i<9;i++){
        Paint_DrawLine(5, y, 638, y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
        y += 30;
    }
    for(int i;i<10;i++){
        Paint_DrawLine(163-50, y2+25, 163-50, y2+45, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
        Paint_DrawLine(321+40, y2+25, 321+40, y2+45, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
        Paint_DrawLine(479+70, y2+25, 479+70, y2+45, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
        y2 += 30;
    }
    
    y2 = 235-50+30+30;

    //for scrolling
    int x=0;
    if(gamestotal>8)x=gamestotal-8;
    else x=0;

    for(int i=x;i<gamestotal;i++){

        //date    
        for (int j = 0; j < 20; j++) {
            dataBuffer[j]=*(getTableData(i,0) + j);
        }
        Paint_DrawString_EN(5+5, y2, dataBuffer, &Font16,WHITE, BLACK);
        
        //game
        for (int j = 0; j < 20; j++) {
            dataBuffer[j]=*(getTableData(i,1) + j);
        }
        Paint_DrawString_EN(163-50+5, y2, dataBuffer, &Font16,WHITE, BLACK);

        //winner
        for (int j = 0; j < 20; j++) {
            dataBuffer[j]=*(getTableData(i,2) + j);
        }
        Paint_DrawString_EN(321+40+5, y2, dataBuffer, &Font16,WHITE, BLACK);

        //score of the game
        dataBuffer[0]=*(getTableData(i,3));
        if(dataBuffer[0]=='0')Paint_DrawString_EN(479+70+5, y2, " ", &Font16,WHITE, BLACK);
        else if(dataBuffer[0]=='1')Paint_DrawString_EN(479+70+5, y2, "*", &Font16,WHITE, BLACK);
        else if(dataBuffer[0]=='2')Paint_DrawString_EN(479+70+5, y2, "**", &Font16,WHITE, BLACK);
        else if(dataBuffer[0]=='3')Paint_DrawString_EN(479+70+5, y2, "***", &Font16,WHITE, BLACK);
        else if(dataBuffer[0]=='4')Paint_DrawString_EN(479+70+5, y2, "****", &Font16,WHITE, BLACK);
        else if(dataBuffer[0]=='5')Paint_DrawString_EN(479+70+5, y2, "*****", &Font16,WHITE, BLACK);
        else Paint_DrawString_EN(479+70+5, y2, "Score?", &Font16,WHITE, BLACK);

        y2 += 30;
    }
    freeTable();

    // red part
    Paint_SelectImage(img_red);
    Paint_Clear(WHITE);
    Paint_DrawBitMap(redData);
    Paint_DrawString_EN(5+5, 215, headline[0], &Font20,WHITE, RED);
    Paint_DrawString_EN(163-50+5, 215, headline[1], &Font20,WHITE, RED);
    Paint_DrawString_EN(321+40+5, 215, headline[2], &Font20,WHITE, RED);
    Paint_DrawString_EN(479+70+5, 215, headline[3], &Font20,WHITE, RED);

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
