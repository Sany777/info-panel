#include "device_task.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "device_common.h"
#include "epaper_adapter.h"

void test_epaper_task(void *pv) {
  ESP_LOGI("EPD_TEST", "E-paper test");
  
  device_set_pin(PIN_EP_EN, 1);
  vTaskDelay(100 / portTICK_PERIOD_MS); 

  epaper_init();
  
  int toggle = 0;
  while(1) {
      ESP_LOGI("EPD_TEST", "Drawing test screen %d...", toggle);
      
      epaper_clear();
      
      if (toggle == 0) {
          draw_rect(10, 10, 200, 100, BLACK, true);
          epaper_print_str(20, 20, FONT_SIZE_20, WHITE, "BLACK TEST");
      } else if (toggle == 1) {
          draw_rect(10, 10, 200, 100, RED, true);
          epaper_print_str(20, 20, FONT_SIZE_20, WHITE, "RED TEST");
      } else {
          epaper_print_str(50, 50, FONT_SIZE_20, BLACK, "CLEAR TEST");
      }
      
      ESP_LOGI("EPD_TEST", "Calling epaper_update()...");
      epaper_update();
      ESP_LOGI("EPD_TEST", "Update finished. Waiting 10 seconds...");
      
      toggle = (toggle + 1) % 3;
      vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}
