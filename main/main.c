#include <sys/cdefs.h>
#include <esp_log.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"


#include "esp_lvgl_port.h"
#include <demos/lv_demos.h>
#include <esp_lcd_panel_vendor.h>

#define LCD_LOG_TAG "LCD"

#define LCD_HOST  SPI2_HOST

#define ST7789_LCD_BK_LIGHT_ON_LEVEL  1
#define ST7789_PIN_NUM_SCLK           12
#define ST7789_PIN_NUM_MOSI           11
#define ST7789_PIN_NUM_MISO           (-1)
#define ST7789_PIN_NUM_LCD_DC         15
#define ST7789_PIN_NUM_LCD_RST        16
#define ST7789_PIN_NUM_LCD_CS         (-1)
#define ST7789_PIN_NUM_BK_LIGHT       14



static lv_disp_t * disp;
esp_lcd_panel_handle_t lcd_panel_handle;
/* The component calls esp_lcd_panel_draw_bitmap API for send data to the screen. There must be called
lvgl_port_flush_ready(disp) after each transaction to display. The best way is to use on_color_trans_done
callback from esp_lcd IO config structure. */
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_t ** pDisp = (lv_disp_t **)user_ctx;
    lvgl_port_flush_ready(*pDisp);
    return false;
}


void LVGL_CentralButton(void) {
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_height(btn, 30);

    lv_obj_t *label;
    label = lv_label_create(btn);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label, "LEGEND");

    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 10);
    lv_style_set_border_color(&style_btn, lv_color_white());
    lv_style_set_border_opa(&style_btn, LV_OPA_30);
    lv_obj_add_style(btn, &style_btn, LV_STATE_DEFAULT);
}

void app_main(){
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);
    ESP_LOGI(LCD_LOG_TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << ST7789_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    ESP_LOGI(LCD_LOG_TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
            .sclk_io_num = ST7789_PIN_NUM_SCLK,
            .mosi_io_num = ST7789_PIN_NUM_MOSI,
            .miso_io_num = ST7789_PIN_NUM_MISO,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 240 * 240 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(LCD_LOG_TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
            .dc_gpio_num = ST7789_PIN_NUM_LCD_DC,
            .cs_gpio_num = ST7789_PIN_NUM_LCD_CS,
            .pclk_hz = SPI_MASTER_FREQ_80M,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .spi_mode = 2,
            .trans_queue_depth = 40,
            .on_color_trans_done = notify_lvgl_flush_ready,
            .user_ctx = &disp
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(LCD_LOG_TAG, "Install ST7789 panel driver");
    esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = ST7789_PIN_NUM_LCD_RST,
            .rgb_endian = LCD_RGB_ENDIAN_RGB,
            .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &lcd_panel_handle));

    //ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle)); //disabled reset here, delay not enough, need fixup.
    gpio_set_level(ST7789_PIN_NUM_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(ST7789_PIN_NUM_LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(ST7789_PIN_NUM_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_panel_handle));

    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_panel_handle, true));

    ESP_LOGI(LCD_LOG_TAG, "Turn on LCD backlight");
    gpio_set_level(ST7789_PIN_NUM_BK_LIGHT, ST7789_LCD_BK_LIGHT_ON_LEVEL);
    esp_lcd_panel_invert_color(lcd_panel_handle, true);
    /////////////////////
    /* Add LCD screen */
    const lvgl_port_display_cfg_t disp_cfg = {
            .io_handle = io_handle,
            .panel_handle = lcd_panel_handle,
            .buffer_size = 240*240,
            .double_buffer = true,
            .hres = 240,
            .vres = 240,
            .monochrome = false,
            /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
            .rotation = {
                    .swap_xy = true,
                    .mirror_x = true,
                    .mirror_y = false,
            },
            .flags = {
                    .buff_dma = true,
            }
    };
    disp = lvgl_port_add_disp(&disp_cfg);

    lvgl_port_lock(0);

    lv_demo_music();
//    lv_demo_benchmark_set_max_speed(true);
//    lv_demo_benchmark();


    /* Screen operation done -> release for the other task */
    lvgl_port_unlock();

}