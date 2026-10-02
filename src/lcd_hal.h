#pragma once
/*
 * lcd_hal.h -- simulator-side "display driver" for real U8g2.
 *
 * The desktop uses this virtual 320x240 panel as a drop-in U8g2 display target.
 * The physical ST7789 TFT will use its own driver in the firmware build:
 *
 *      application  ->  U8g2 (real fonts, real drawing code, real buffer)
 *                          |
 *                    display_cb  <-- backend-specific
 *                     /            \
 *          firmware: ST7789 driver  sim: u8x8_d_calcsim_320x240 (this file)
 *
 * The simulator copies U8g2's 1-bit pixels into a logical framebuffer for SDL.
 *
 * This does not emulate ST7789 color, SPI traffic, or hardware refresh timing.
 */
#include "u8g2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CALCSIM_LCD_W 320
#define CALCSIM_LCD_H 240

/* What is currently "on the glass": 1 = dark pixel. Filled by display_cb
   every time U8g2 sends tiles (u8g2_SendBuffer). */
extern uint8_t g_calcsim_lcd_pixels[CALCSIM_LCD_H][CALCSIM_LCD_W];
extern uint8_t g_calcsim_lcd_powered;   /* 0 after u8g2_SetPowerSave(..,1) */
extern uint8_t g_calcsim_lcd_contrast;  /* last value from u8g2_SetContrast */

/* Full-buffer (_f) setup for the simulator's 320x240 logical TFT. */
void u8g2_Setup_calcsim_320x240_f(u8g2_t *u8g2, const u8g2_cb_t *rotation);

#ifdef __cplusplus
}
#endif
