#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "sdkconfig.h"
#include "sentinel.h"
#include <cstring>
namespace sentinel {
static esp_lcd_panel_handle_t panel = nullptr;
static bool present = false;
static bool reserved(int p) {
  return p == 19 || p == 20 || p == 35 || p == 36 || p == 37 || p == 43 ||
         p == 44;
}
bool oled_init() {
  if (reserved(CONFIG_SENTINEL_SDA) || reserved(CONFIG_SENTINEL_SCL) ||
      CONFIG_SENTINEL_SDA == CONFIG_SENTINEL_SCL)
    return false;
  i2c_master_bus_config_t bus_config = {};
  bus_config.i2c_port = I2C_NUM_0;
  bus_config.sda_io_num = static_cast<gpio_num_t>(CONFIG_SENTINEL_SDA);
  bus_config.scl_io_num = static_cast<gpio_num_t>(CONFIG_SENTINEL_SCL);
  bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
  bus_config.glitch_ignore_cnt = 7;
  bus_config.flags.enable_internal_pullup = true;
  i2c_master_bus_handle_t bus = nullptr;
  if (i2c_new_master_bus(&bus_config, &bus) != ESP_OK ||
      i2c_master_probe(bus, CONFIG_SENTINEL_OLED_ADDRESS, 50) != ESP_OK)
    return false;
  esp_lcd_panel_io_i2c_config_t io_config = {};
  io_config.dev_addr = CONFIG_SENTINEL_OLED_ADDRESS;
  io_config.scl_speed_hz = 400000;
  io_config.control_phase_bytes = 1;
  io_config.dc_bit_offset = 6;
  io_config.lcd_cmd_bits = 8;
  io_config.lcd_param_bits = 8;
  esp_lcd_panel_io_handle_t io = nullptr;
  if (esp_lcd_new_panel_io_i2c(bus, &io_config, &io) != ESP_OK)
    return false;
  esp_lcd_panel_dev_config_t cfg = {};
  cfg.reset_gpio_num = -1;
  cfg.bits_per_pixel = 1;
  esp_lcd_panel_ssd1306_config_t vendor = {};
  vendor.height = 32;
  cfg.vendor_config = &vendor;
  if (esp_lcd_new_panel_ssd1306(io, &cfg, &panel) != ESP_OK ||
      esp_lcd_panel_reset(panel) != ESP_OK ||
      esp_lcd_panel_init(panel) != ESP_OK ||
      esp_lcd_panel_disp_on_off(panel, true) != ESP_OK)
    return false;
  present = true;
  return true;
}
// Original compact 3x5 glyphs, expanded vertically in a 128x32 monochrome
// buffer.
static uint16_t glyph(char c) {
  if (c >= 'a' && c <= 'z')
    c -= 32;
  switch (c) {
  case '0':
    return 0b111101101101111;
  case '1':
    return 0b010110010010111;
  case '2':
    return 0b111001111100111;
  case '3':
    return 0b111001111001111;
  case '4':
    return 0b101101111001001;
  case '5':
    return 0b111100111001111;
  case '6':
    return 0b111100111101111;
  case '7':
    return 0b111001001001001;
  case '8':
    return 0b111101111101111;
  case '9':
    return 0b111101111001111;
  case 'A':
    return 0b010101111101101;
  case 'B':
    return 0b110101110101110;
  case 'C':
    return 0b111100100100111;
  case 'D':
    return 0b110101101101110;
  case 'E':
    return 0b111100110100111;
  case 'F':
    return 0b111100110100100;
  case 'G':
    return 0b111100101101111;
  case 'H':
    return 0b101101111101101;
  case 'I':
    return 0b111010010010111;
  case 'J':
    return 0b001001001101111;
  case 'K':
    return 0b101101110101101;
  case 'L':
    return 0b100100100100111;
  case 'M':
    return 0b101111111101101;
  case 'N':
    return 0b101111111111101;
  case 'O':
    return 0b111101101101111;
  case 'P':
    return 0b111101111100100;
  case 'Q':
    return 0b111101101111001;
  case 'R':
    return 0b110101110101101;
  case 'S':
    return 0b111100111001111;
  case 'T':
    return 0b111010010010010;
  case 'U':
    return 0b101101101101111;
  case 'V':
    return 0b101101101101010;
  case 'W':
    return 0b101101111111101;
  case 'X':
    return 0b101101010101101;
  case 'Y':
    return 0b101101010010010;
  case 'Z':
    return 0b111001010100111;
  case ':':
    return 0b000010000010000;
  case '-':
    return 0b000000111000000;
  case '_':
    return 0b000000000000111;
  case '.':
    return 1;
  default:
    return 0;
  }
}
bool oled_show(const char *a, const char *b, const char *c, const char *d) {
  if (!present)
    return false;
  uint8_t bitmap[512] = {};
  const char *lines[] = {a, b, c, d};
  for (int row = 0; row < 4; ++row)
    for (int column = 0; lines[row] && lines[row][column] && column < 21;
         ++column) {
      uint16_t g = glyph(lines[row][column]);
      for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 3; ++x)
          if (g & (1U << (14 - y * 3 - x))) {
            int px = column * 6 + x + 1, py = row * 8 + y + 1;
            bitmap[(py / 8) * 128 + px] |= 1U << (py % 8);
          }
    }
  if (esp_lcd_panel_draw_bitmap(panel, 0, 0, 128, 32, bitmap) != ESP_OK) {
    present = false;
    return false;
  }
  return true;
}
bool button_init() {
  if (reserved(CONFIG_SENTINEL_BUTTON) ||
      CONFIG_SENTINEL_BUTTON == CONFIG_SENTINEL_SDA ||
      CONFIG_SENTINEL_BUTTON == CONFIG_SENTINEL_SCL)
    return false;
  gpio_config_t cfg = {};
  cfg.pin_bit_mask = 1ULL << CONFIG_SENTINEL_BUTTON;
  cfg.mode = GPIO_MODE_INPUT;
  cfg.pull_up_en = GPIO_PULLUP_ENABLE;
  return gpio_config(&cfg) == ESP_OK;
}
bool button_down() {
  return gpio_get_level(static_cast<gpio_num_t>(CONFIG_SENTINEL_BUTTON)) == 0;
}
} // namespace sentinel
