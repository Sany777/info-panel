#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/gpio_struct.h"

#include "epdif.h"

static spi_device_handle_t spi;

void
epd_digital_write(unsigned int pin, int value)
{
    gpio_set_level((gpio_num_t)pin, value);
}

int
epd_digital_read(unsigned int pin)
{
    return gpio_get_level((gpio_num_t)pin);
}

void
epd_delay_ms(unsigned int delaytime)
{
    vTaskDelay(delaytime / portTICK_PERIOD_MS);
}

void
epd_spi_transfer(unsigned char data)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.flags      = SPI_TRANS_USE_TXDATA;
    t.length     = 8;
    t.tx_data[0] = data;
    t.tx_data[1] = data;
    t.tx_data[2] = data;
    t.tx_data[3] = data;
    ret          = spi_device_transmit(spi, &t);
    assert(ret == ESP_OK);
}

int
epd_if_init(void)
{
    if (spi) {
        spi_bus_remove_device(spi);
    }

    spi_bus_free(SPI2_HOST);

    gpio_config_t io_conf = {};
    io_conf.intr_type     = GPIO_INTR_DISABLE;
    io_conf.mode          = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
        ((uint64_t)1 << (uint64_t)DC_PIN) | ((uint64_t)1 << (uint64_t)RST_PIN) | ((uint64_t)1 << (uint64_t)CS_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_set_level((gpio_num_t)CS_PIN, 1);

    gpio_config_t i_conf;
    memset(&i_conf, 0, sizeof(i_conf));
    i_conf.intr_type    = GPIO_INTR_DISABLE;
    i_conf.mode         = GPIO_MODE_INPUT;
    i_conf.pin_bit_mask = ((uint64_t)1 << (uint64_t)BUSY_PIN);
    i_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    i_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&i_conf));

    esp_err_t ret;
    spi_bus_config_t buscfg;
    memset(&buscfg, 0, sizeof(buscfg));
    buscfg.mosi_io_num   = MOSI_PIN;
    buscfg.sclk_io_num   = CLK_PIN;
    buscfg.miso_io_num   = -1;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;

    ret = spi_bus_initialize(SPI2_HOST, &buscfg, 0);

    assert(ret == ESP_OK);

    spi_device_interface_config_t devcfg;
    memset(&devcfg, 0, sizeof(devcfg));
    devcfg.command_bits   = 0;
    devcfg.address_bits   = 0;
    devcfg.dummy_bits     = 0;
    devcfg.clock_speed_hz = 2 * 1000 * 1000;
    devcfg.mode           = 0;
    devcfg.spics_io_num   = -1;
    devcfg.queue_size     = 1;

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);

    assert(ret == ESP_OK);

    return 0;
}
