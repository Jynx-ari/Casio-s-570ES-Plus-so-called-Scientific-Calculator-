#include "lcd_hal.h"
#include <string.h>

uint8_t g_calcsim_lcd_pixels[CALCSIM_LCD_H][CALCSIM_LCD_W];
uint8_t g_calcsim_lcd_powered = 1;
uint8_t g_calcsim_lcd_contrast = 0x20;

static const u8x8_display_info_t calcsim_info = {
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  /* post_chip_enable_wait_ns = */ 0,
  /* pre_chip_disable_wait_ns = */ 0,
  /* reset_pulse_width_ms = */ 0,
  /* post_reset_wait_ms = */ 0,
  /* sda_setup_time_ns = */ 0,
  /* sck_pulse_width_ns = */ 0,
  /* sck_clock_hz = */ 4000000UL,
  /* spi_mode = */ 0,
  /* i2c_bus_clock_100kHz = */ 0,
  /* data_setup_time_ns = */ 0,
  /* write_pulse_width_ns = */ 0,
  /* tile_width = */ 16,
  /* tile_hight = */ 8,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 128,
  /* pixel_height = */ 64
};

/* Unpack one 8-pixel-tall column strip (LSB = top) into the glass buffer. */
static void put_column(int x, int y, uint8_t bits)
{
  for (int i = 0; i < 8; i++) {
    int yy = y + i;
    if (x >= 0 && x < CALCSIM_LCD_W && yy >= 0 && yy < CALCSIM_LCD_H)
      g_calcsim_lcd_pixels[yy][x] = (bits >> i) & 1;
  }
}

static uint8_t calcsim_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  (void)msg; (void)arg_int; (void)arg_ptr;
  u8x8_SetGPIOResult(u8x8, 1);   /* no GPIO, no delays needed on desktop */
  return 1;
}

static uint8_t u8x8_d_calcsim_128x64(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch (msg) {
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8x8, &calcsim_info);
      memset(g_calcsim_lcd_pixels, 0, sizeof g_calcsim_lcd_pixels);
      break;
    case U8X8_MSG_DISPLAY_INIT:
      u8x8_d_helper_display_init(u8x8);
      break;
    case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      g_calcsim_lcd_powered = (arg_int == 0);
      break;
    case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      break;
    case U8X8_MSG_DISPLAY_SET_CONTRAST:
      g_calcsim_lcd_contrast = arg_int;
      break;
    case U8X8_MSG_DISPLAY_DRAW_TILE: {
      u8x8_tile_t *t = (u8x8_tile_t *)arg_ptr;
      int x = (t->x_pos * 8) + u8x8->x_offset;
      int y = t->y_pos * 8;
      uint8_t repeat = arg_int;
      do {
        const uint8_t *p = t->tile_ptr;
        for (int tile = 0; tile < t->cnt; tile++)
          for (int col = 0; col < 8; col++)
            put_column(x + tile * 8 + col, y, *p++);
        x += t->cnt * 8;
      } while (--repeat > 0);
      break;
    }
    default:
      return 0;
  }
  return 1;
}

void u8g2_Setup_calcsim_128x64_f(u8g2_t *u8g2, const u8g2_cb_t *rotation)
{
  static uint8_t buf[128 * 8];   /* 128 columns x 8 pages, full frame */
  u8x8_t *u8x8 = u8g2_GetU8x8(u8g2);
  u8x8_SetupDefaults(u8x8);
  u8x8->display_cb = u8x8_d_calcsim_128x64;
  u8x8->gpio_and_delay_cb = calcsim_gpio_and_delay;
  u8x8_SetupMemory(u8x8);
  u8g2_SetupBuffer(u8g2, buf, 8, u8g2_ll_hvline_vertical_top_lsb, rotation);
}
