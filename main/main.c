#include "adc_reader.h"
#include "device_common.h"
#include "device_task.h"
#include "epaper_adapter.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sound_generator.h"

void
app_main()
{
    esp_log_level_set("wifi", ESP_LOG_ERROR);
    esp_log_level_set("wifi_init", ESP_LOG_ERROR);
    device_init();
    sound_generator_init();
    adc_reader_init();
    task_init();
}
