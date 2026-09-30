#include "display.h"

Display::Display() {
    // Firmware equivalent:
    //   u8g2_Setup_st7565_ea_dogm128_f(&u8g2_, U8G2_R0, <spi cb>, <gpio cb>);
    u8g2_Setup_calcsim_128x64_f(&u8g2_, U8G2_R0);
    u8g2_InitDisplay(&u8g2_);
    u8g2_SetPowerSave(&u8g2_, 0);
    u8g2_SetFontPosBaseline(&u8g2_);
    u8g2_SetFontMode(&u8g2_, 1);
    u8g2_SetDrawColor(&u8g2_, 1);
}

void Display::select(Font f) {
    switch (f) {
        case Font::Tiny:  u8g2_SetFont(&u8g2_, u8g2_font_4x6_tr); break;
        case Font::Small: u8g2_SetFont(&u8g2_, u8g2_font_5x7_tf); break;
        case Font::Main:  u8g2_SetFont(&u8g2_, u8g2_font_6x10_tf); break;
    }
}
void Display::clear() { u8g2_ClearBuffer(&u8g2_); }
void Display::present() { u8g2_SendBuffer(&u8g2_); }
void Display::setClip(int x0, int y0, int x1, int y1) { u8g2_SetClipWindow(&u8g2_, x0, y0, x1, y1); }
void Display::clearClip() { u8g2_SetMaxClipWindow(&u8g2_); }
void Display::setColor(int c) { u8g2_SetDrawColor(&u8g2_, c); }
void Display::setContrast(int c) { u8g2_SetContrast(&u8g2_, c); }

int Display::ascent(Font f) { select(f); return u8g2_GetAscent(&u8g2_); }
int Display::descent(Font f) { select(f); int d = u8g2_GetDescent(&u8g2_); return d < 0 ? -d : d; }
int Display::textWidth(const std::string &s, Font f) {
    if (s.empty()) return 0;
    select(f); return u8g2_GetUTF8Width(&u8g2_, s.c_str());
}
void Display::text(int x, int y, const std::string &s, Font f) {
    if (s.empty()) return;
    select(f); u8g2_DrawUTF8(&u8g2_, x, y, s.c_str());
}
void Display::pixel(int x, int y) { u8g2_DrawPixel(&u8g2_, x, y); }
void Display::hLine(int x, int y, int w) { if (w > 0) u8g2_DrawHLine(&u8g2_, x, y, w); }
void Display::vLine(int x, int y, int h) { if (h > 0) u8g2_DrawVLine(&u8g2_, x, y, h); }
void Display::line(int x0, int y0, int x1, int y1) { u8g2_DrawLine(&u8g2_, x0, y0, x1, y1); }
void Display::box(int x, int y, int w, int h) { if (w > 0 && h > 0) u8g2_DrawBox(&u8g2_, x, y, w, h); }
void Display::frame(int x, int y, int w, int h) { if (w > 0 && h > 0) u8g2_DrawFrame(&u8g2_, x, y, w, h); }
void Display::glyph(int x, int y, const char *const *rows, int h) {
    for (int r = 0; r < h; ++r)
        for (int c = 0; rows[r][c]; ++c)
            if (rows[r][c] == '#') u8g2_DrawPixel(&u8g2_, x + c, y + r);
}
