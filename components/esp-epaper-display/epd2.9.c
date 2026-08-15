#include "epd2.9.h"
#include "esp_log.h"
#include <stdlib.h>

void
epd_init_struct(epd_t *epd)
{
    epd->reset_pin = RST_PIN;
    epd->dc_pin    = DC_PIN;
    epd->cs_pin    = CS_PIN;
    epd->busy_pin  = BUSY_PIN;
    epd->width     = EPD_WIDTH;
    epd->height    = EPD_HEIGHT;
}

int
epd_ldir_init(epd_t *epd)
{
    int ret     = epd_init(epd);
    epd->width  = EPD_HEIGHT;
    epd->height = EPD_WIDTH;
    return ret;
}

void
epd_reset(epd_t *epd)
{
    epd_digital_write(epd->reset_pin, 1);
    epd_delay_ms(200);
    epd_digital_write(epd->reset_pin, 0);
    epd_delay_ms(10);
    epd_digital_write(epd->reset_pin, 1);
    epd_delay_ms(200);
}

void
epd_send_command(epd_t *epd, unsigned char command)
{
    epd_digital_write(epd->dc_pin, 0);
    epd_digital_write(epd->cs_pin, 0);
    epd_spi_transfer(command);
    epd_digital_write(epd->cs_pin, 1);
}

void
epd_send_data(epd_t *epd, unsigned char data)
{
    epd_digital_write(epd->dc_pin, 1);
    epd_digital_write(epd->cs_pin, 0);
    epd_spi_transfer(data);
    epd_digital_write(epd->cs_pin, 1);
}

void
epd_wait_until_idle(epd_t *epd)
{
    int timeout = 0;
    while (epd_digital_read(epd->busy_pin) == 1) { // 1: busy, 0: idle
        epd_delay_ms(50);
        timeout++;
        if (timeout > 600) { // 30 seconds max
            ESP_LOGE("EPD", "Timeout waiting for BUSY pin!");
            break;
        }
    }
}

static void
set_ram_pointer(epd_t *epd)
{
    epd_send_command(epd, 0x4E); // set RAM x address count to 0;
    epd_send_data(epd, 0x00);
    epd_send_command(epd, 0x4F); // set RAM y address count to 0;
    epd_send_data(epd, 0x00);
    epd_send_data(epd, 0x00);
}

void
epd_display_black(epd_t *epd, const unsigned char *frame_buffer)
{
    if (frame_buffer != NULL) {
        set_ram_pointer(epd);
        epd_send_command(epd, 0x24);
        epd_digital_write(epd->dc_pin, 1);
        epd_digital_write(epd->cs_pin, 0);
        for (int i = 0; i < epd->width * epd->height / 8; i++) {
            epd_spi_transfer(frame_buffer[i]);
        }
        epd_digital_write(epd->cs_pin, 1);
    }
}

void
epd_display_red(epd_t *epd, const unsigned char *frame_buffer)
{
    if (frame_buffer != NULL) {
        set_ram_pointer(epd);
        epd_send_command(epd, 0x26);
        epd_digital_write(epd->dc_pin, 1);
        epd_digital_write(epd->cs_pin, 0);
        for (int i = 0; i < epd->width * epd->height / 8; i++) {
            epd_spi_transfer(frame_buffer[i]);
        }
        epd_digital_write(epd->cs_pin, 1);
    }
}

void
epd_display_frame(epd_t *epd)
{
    epd_send_command(epd, 0x22);
    epd_send_data(epd, 0xF7);
    epd_send_command(epd, 0x20);
    epd_delay_ms(100);
    epd_wait_until_idle(epd);
}

void
epd_clear_frame(epd_t *epd)
{
    set_ram_pointer(epd);
    epd_send_command(epd, 0x24);
    epd_digital_write(epd->dc_pin, 1);
    epd_digital_write(epd->cs_pin, 0);
    for (int i = 0; i < epd->width * epd->height / 8; i++) {
        epd_spi_transfer(0xFF);
    }
    epd_digital_write(epd->cs_pin, 1);

    set_ram_pointer(epd);
    epd_send_command(epd, 0x26);
    epd_digital_write(epd->dc_pin, 1);
    epd_digital_write(epd->cs_pin, 0);
    for (int i = 0; i < epd->width * epd->height / 8; i++) {
        epd_spi_transfer(0x00);
    }
    epd_digital_write(epd->cs_pin, 1);
}

void
epd_sleep(epd_t *epd)
{
    epd_send_command(epd, 0x10);
    epd_send_data(epd, 0x01);
}

int
epd_init(epd_t *epd)
{
    if (epd_if_init() != 0) {
        return -1;
    }
    epd_reset(epd);

    epd_send_command(epd, 0x12); // SWRESET
    epd_wait_until_idle(epd);

    epd_send_command(epd, 0x01); // Driver output control
    epd_send_data(epd, 0x27);
    epd_send_data(epd, 0x01);
    epd_send_data(epd, 0x00);

    epd_send_command(epd, 0x11); // data entry mode
    epd_send_data(epd, 0x03);

    epd_send_command(epd, 0x44); // set Ram-X address start/end position
    epd_send_data(epd, 0x00);
    epd_send_data(epd, 0x0F);

    epd_send_command(epd, 0x45); // set Ram-Y address start/end position
    epd_send_data(epd, 0x00);
    epd_send_data(epd, 0x00);
    epd_send_data(epd, 0x27);
    epd_send_data(epd, 0x01);

    epd_send_command(epd, 0x3C); // BorderWavefrom
    epd_send_data(epd, 0x05);

    epd_send_command(epd, 0x21); // Display update control
    epd_send_data(epd, 0x00);
    epd_send_data(epd, 0x80);

    epd_send_command(epd, 0x18);
    epd_send_data(epd, 0x80);

    set_ram_pointer(epd);
    epd_wait_until_idle(epd);

    return 0;
}
