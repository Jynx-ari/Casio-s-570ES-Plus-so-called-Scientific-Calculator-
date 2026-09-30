#pragma once
// display.h -- thin C++ wrapper over REAL U8g2 (PROJECT.md section 14).
// UI code talks to Display only. Firmware: same class, real ST7565 setup call.
#include <string>
#include "lcd_hal.h"

class Display {
public:
    enum class Font { Tiny, Small, Main };   // 4x6 (indicators), 5x7 (exponents), 6x10 (main)

    Display();

    void clear();
    void present();
    void setClip(int x0, int y0, int x1, int y1);   // x1/y1 exclusive
    void clearClip();
    void setColor(int c);                            // 1 = dark pixel, 0 = erase

    int  ascent(Font f);                             // pixels above baseline (positive)
    int  descent(Font f);                            // pixels below baseline (positive)
    int  textWidth(const std::string &utf8, Font f);
    void text(int x, int baselineY, const std::string &utf8, Font f);

    void pixel(int x, int y);
    void hLine(int x, int y, int w);
    void vLine(int x, int y, int h);
    void line(int x0, int y0, int x1, int y1);
    void box(int x, int y, int w, int h);
    void frame(int x, int y, int w, int h);
    // 1-bit picture given as rows of '#'/'.' (used for pi, arrows: no font has them)
    void glyph(int x, int y, const char *const *rows, int h);

    static constexpr int width() { return CALCSIM_LCD_W; }
    static constexpr int height() { return CALCSIM_LCD_H; }
    static bool pixelAt(int x, int y) { return g_calcsim_lcd_pixels[y][x] != 0; }
    static bool powered() { return g_calcsim_lcd_powered != 0; }
    static int contrast() { return g_calcsim_lcd_contrast; }
    void setContrast(int c);

private:
    u8g2_t u8g2_;
    void select(Font f);
};
