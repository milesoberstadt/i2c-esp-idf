#include "ui.h"

UI::UI()
{

    bus = std::unique_ptr<Arduino_DataBus>(new Arduino_ESP32PAR8Q(
        7 /* DC */, 
        6 /* CS */, 
        8 /* WR */, 
        9 /* RD */,
        39 /* D0 */, 
        40 /* D1 */, 
        41 /* D2 */, 
        42 /* D3 */, 
        45 /* D4 */, 
        46 /* D5 */,
        47 /* D6 */, 
        48 /* D7 */));
        
    gfx = std::unique_ptr<Arduino_GFX>(new Arduino_ST7789(
        bus.get(), 
        5 /* RST */, 
        0 /* rotation */, 
        true /* IPS */, 
        170 /* width */, 
        320 /* height */, 
        35 /* col offset 1 */, 
        0 /* row offset 1 */, 
        35 /* col offset 2 */, 
        0 /* row offset 2 */));
    
    pinMode(15 /* PWD */, OUTPUT);   
    digitalWrite(15 /* PWD */, HIGH);

    #ifdef GFX_BL
        pinMode(GFX_BL, OUTPUT);
        digitalWrite(GFX_BL, HIGH);
    #endif

    gfx->begin();
    gfx->setRotation(1);

    w = gfx->width();
    h = gfx->height();
    n = min(w, h);
    n1 = n - 1;
    cx = w / 2;
    cy = h / 2;
    cx1 = cx - 1;
    cy1 = cy - 1;
    cn = min(cx1, cy1);
    cn1 = cn - 1;
    tsa = ((w <= 176) || (h <= 160)) ? 1 : (((w <= 240) || (h <= 240)) ? 2 : 3); // text size A
    tsb = ((w <= 272) || (h <= 220)) ? 1 : 2;                                    // text size B
    tsc = ((w <= 220) || (h <= 220)) ? 1 : 2;                                    // text size C
    ds = (w <= 160) ? 9 : 12;                                                    // digit size

}


UI::~UI()
{
    digitalWrite(15 /* PWD */, LOW);
    #ifdef GFX_BL
        digitalWrite(GFX_BL, LOW);
    #endif
}

void UI::background()
{
    gfx->fillScreen(BLACK);
}

void UI::background(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    gfx->fillRect(x, y, w, h, BLACK);
}

void UI::titleStyle()
{
    gfx->setRotation(90);
    gfx->setTextSize(tsa);
    gfx->setTextColor(WHITE);
    gfx->setTextWrap(false);
    gfx->setCursor(0, 0);
}

void UI::textStyle()
{
    gfx->setRotation(1);
    gfx->setTextSize(tsb);
    gfx->setTextColor(WHITE);
    gfx->setTextWrap(false);
}

void UI::smallTextStyle()
{
    gfx->setRotation(1);
    gfx->setTextSize(tsc);
    gfx->setTextColor(WHITE);
    gfx->setTextWrap(false);
}