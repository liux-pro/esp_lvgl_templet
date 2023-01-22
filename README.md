# esp-idf & lvgl

This repository shows how to initialize lvgl with minimal code.

# Circuit

esp32s3 + st7789(240*240)

```c
#define ST7789_LCD_BK_LIGHT_ON_LEVEL  1
#define ST7789_PIN_NUM_SCLK           4
#define ST7789_PIN_NUM_MOSI           5
#define ST7789_PIN_NUM_MISO           (-1)
#define ST7789_PIN_NUM_LCD_DC         7
#define ST7789_PIN_NUM_LCD_RST        6
#define ST7789_PIN_NUM_LCD_CS         (-1)
#define ST7789_PIN_NUM_BK_LIGHT       15
```

# Software

The lvgl community adapted the esp-idf environment, and espressif released the lvgl driver package for st7789 lcd screen.  
Both of them are available at [ESP Registry](https://components.espressif.com/). We can now add this two dependency using [idf-component-manager](https://github.com/espressif/)

```bash
idf.py add-dependency "lvgl/lvgl^8.3.0"
idf.py add-dependency "espressif/esp_lvgl_port^1.0.1"
```

# Requirement

IDF 5.0

# Useful links
https://components.espressif.com/components/lvgl/lvgl
https://components.espressif.com/components/espressif/esp_lvgl_port
https://github.com/espressif/esp-bsp/tree/master/components/esp_lvgl_port