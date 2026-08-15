#ifndef EPDIF_H
#define EPDIF_H

#include "device_common.h"

#define MOSI_PIN PIN_EP_SDA
#define CLK_PIN PIN_EP_SCL
#define CS_PIN PIN_EP_CS
#define DC_PIN PIN_EP_DC
#define RST_PIN PIN_EP_RES
#define BUSY_PIN PIN_EP_BUSY

#if ESP_IDF_VERSION_MAJOR < 4
#define SPI_HOST HSPI_HOST
#endif

int
epd_if_init(void);
void
epd_digital_write(unsigned int pin, int value);
int
epd_digital_read(unsigned int pin);
void
epd_delay_ms(unsigned int delaytime);
void
epd_spi_transfer(unsigned char data);

#endif
