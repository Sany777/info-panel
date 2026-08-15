#ifndef EPD2_9_H
#define EPD2_9_H

#include "epdif.h"

#define EPD_WIDTH 128
#define EPD_HEIGHT 296

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int reset_pin;
    unsigned int dc_pin;
    unsigned int cs_pin;
    unsigned int busy_pin;
} epd_t;

void
epd_init_struct(epd_t *epd);
int
epd_init(epd_t *epd);
int
epd_ldir_init(epd_t *epd);
void
epd_send_command(epd_t *epd, unsigned char command);
void
epd_send_data(epd_t *epd, unsigned char data);
void
epd_wait_until_idle(epd_t *epd);
void
epd_reset(epd_t *epd);

void
epd_display_black(epd_t *epd, const unsigned char *frame_buffer);
void
epd_display_red(epd_t *epd, const unsigned char *frame_buffer);
void
epd_display_frame(epd_t *epd);
void
epd_clear_frame(epd_t *epd);
void
epd_sleep(epd_t *epd);

#endif /* EPD2_9_H */
