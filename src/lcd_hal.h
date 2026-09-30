#pragma once
/*
 * lcd_hal.h -- simulator-side "display driver" for real U8g2.
 *
 * On the real calculator, U8g2 is set up with a genuine controller driver,
 * e.g. u8g2_Setup_st7565_ea_dogm128_f(...), which turns U8g2's 128x64 tile
 * buffer into ST7565 SPI commands. On the desktop there is no ST7565 to talk
 * to, so this file provides a drop-in replacement for that ONE layer:
 *
 *      application  ->  U8g2 (real fonts, real drawing code, real buffer)
 *                          |
 *                    display_cb  <-- the only thing that differs
 *                     /            \
 *          real: u8x8_d_st7565_*    sim: u8x8_d_calcsim_128x64 (this file)
 *
 * Pixel layout (vertical_top_lsb, 8 pages x 128 columns) is the same layout
 * an ST7565's internal RAM uses, so buffer handling matches the real chip.
 *
 * What this does NOT emulate: the ST7565 SPI command stream (page/column
 * address commands, contrast register, bias, etc.). Driver-level bugs would
 * only show up on real hardware.
 */
#include "u8g2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CALCSIM_LCD_W 128
#define CALCSIM_LCD_H 64

/* What is currently "on the glass": 1 = dark pixel. Filled by display_cb
   every time U8g2 sends tiles (u8g2_SendBuffer). */
extern uint8_t g_calcsim_lcd_pixels[CALCSIM_LCD_H][CALCSIM_LCD_W];
extern uint8_t g_calcsim_lcd_powered;   /* 0 after u8g2_SetPowerSave(..,1) */
extern uint8_t g_calcsim_lcd_contrast;  /* last value from u8g2_SetContrast */

/* Full-buffer (_f) setup, analogous to u8g2_Setup_st7565_..._f(). */
void u8g2_Setup_calcsim_128x64_f(u8g2_t *u8g2, const u8g2_cb_t *rotation);

#ifdef __cplusplus
}
#endif
