#include "epaper_adapter.h"

#include "epaper.h"
#include "epd2.9.h"
#include "epdpaint.h"
#include "fonts.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "device_macro.h"
#include "esp_log.h"

#define SCREEN_HEIGHT 128
#define SCREEN_WIDTH 296

static paint_t paint;
static paint_t paint_red;
static epd_t epd;
static int is_initialized = 0;

static unsigned char screen[SCREEN_WIDTH * SCREEN_HEIGHT / 8];
static unsigned char screen_red[SCREEN_WIDTH * SCREEN_HEIGHT / 8];

static char text_buf[150];
static const int USE_SCREEN_WIDTH = SCREEN_WIDTH - 4;
static sFONT *font_list[]         = {&Font12, &Font16, &Font20};

void
epaper_init()
{
    if (!is_initialized) {
        epd_init_struct(&epd);
        paint_init(&paint, screen, epd.width, epd.height);
        paint_init(&paint_red, screen_red, epd.width, epd.height);
        paint_set_rotate(&paint, ROTATE_90);
        paint_set_rotate(&paint_red, ROTATE_90);
        is_initialized = 1;
    }
    epd_ldir_init(&epd);
}

static sFONT *
get_font(size_t font_id)
{
    if (font_id >= FONT_SIZE_MAX) {
        return NULL;
    }
    return font_list[font_id];
}

void
epaper_print_str(int hor, int ver, font_size_t font_size, color_t color, const char *str)
{
    sFONT *font = get_font(font_size);
    if (font) {
        char buf[150];
        strncpy(buf, str, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        size_t str_len        = strlen(buf);
        int str_width         = font->Width * str_len;
        while (hor + str_width > USE_SCREEN_WIDTH && str_len > 0) {
            str_len--;
            buf[str_len] = '\0';
            str_width    = font->Width * str_len;
        }
        paint_draw_string_at(color == RED ? &paint_red : &paint, hor, ver, buf, font, color);
    }
}

void
epaper_display_image(int x, int y, int w, int h, color_t color, const unsigned char *bitmap)
{
    paint_t *p        = color == RED ? &paint_red : &paint;
    int16_t byteWidth = (w + 7) / 8;
    uint8_t byte      = 0;

    for (uint16_t j = 0; j < h; j++) {
        for (uint16_t i = 0; i < w; i++) {
            if (i & 7)
                byte <<= 1;
            else {
                byte = bitmap[j * byteWidth + i / 8];
            }
            if (!(byte & 0x80)) {
                paint_draw_pixel(p, x + i, y + j, color);
            }
        }
    }
}

static sFONT *
get_fit_font(font_size_t font_id, size_t char_num)
{
    size_t ind       = font_id;
    sFONT *font      = NULL;
    size_t str_width = 0;
    do {
        if (ind >= FONT_SIZE_MAX)
            break;
        font = get_font(ind);
        ind -= 1;
        str_width = font->Width * char_num;
    } while (str_width > USE_SCREEN_WIDTH);
    return font;
}

void
epaper_print_centered_str(int ver, font_size_t font_size, color_t color, const char *str)
{
    int hor = 5, str_width;
    char buf[150];
    strncpy(buf, str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    size_t str_len       = strlen(buf);

    sFONT *font = get_fit_font(font_size, str_len);
    if (font) {
        str_width = font->Width * str_len;
        while (str_width > USE_SCREEN_WIDTH && str_len > 0) {
            str_len--;
            buf[str_len] = '\0';
            str_width    = font->Width * str_len;
        }
        if (str_width < USE_SCREEN_WIDTH) {
            hor += (USE_SCREEN_WIDTH - str_width) / 2;
        }
        paint_draw_string_at(color == RED ? &paint_red : &paint, hor, ver, buf, font, color);
    }
}

void
epaper_printf_centered(int ver, font_size_t font_size, color_t color, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(text_buf, ARR_LEN(text_buf), format, args);
    va_end(args);
    epaper_print_centered_str(ver, font_size, color, text_buf);
}

void
epaper_printf(int hor, int ver, font_size_t font_size, color_t color, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(text_buf, ARR_LEN(text_buf), format, args);
    va_end(args);
    epaper_print_str(hor, ver, font_size, color, text_buf);
}

void
epaper_refresh()
{
    epd_display_frame(&epd);
}

bool
epaper_update()
{
    epd_wait_until_idle(&epd);
    epd_display_black(&epd, screen);
    epd_display_red(&epd, screen_red);
    epd_display_frame(&epd);
    return true;
}

void
epaper_clear()
{
    memset(screen_red, 0, sizeof(screen_red));
    memset(screen, 0xff, sizeof(screen));
}

void
draw_rect(int hor_0, int ver_0, int hor_1, int ver_1, color_t color, int filled)
{
    if (filled) {
        paint_draw_filled_rectangle(color == RED ? &paint_red : &paint, hor_0, ver_0, hor_1, ver_1, color);
    } else {
        paint_draw_rectangle(color == RED ? &paint_red : &paint, hor_0, ver_0, hor_1, ver_1, color);
    }
}

void
draw_circle(int hor, int ver, int radius, color_t color, int filled)
{
    if (filled) {
        paint_draw_filled_circle(color == RED ? &paint_red : &paint, hor, ver, radius, 1);
    } else {
        paint_draw_circle(color == RED ? &paint_red : &paint, hor, ver, radius, 1);
    }
}

void
draw_line(int hor_0, int ver_0, int hor_1, int ver_1, color_t color)
{
    paint_draw_line(color == RED ? &paint_red : &paint, hor_0, ver_0, hor_1, ver_1, 1);
}

void
draw_horizontal_line(int hor_0, int hor_1, int ver, int width, color_t color)
{
    paint_draw_filled_rectangle(color == RED ? &paint_red : &paint, hor_0, ver, hor_1, ver + width, 1);
}

void
draw_vertical_line(int ver_0, int ver_1, int hor, int width, color_t color)
{
    paint_draw_filled_rectangle(color == RED ? &paint_red : &paint, hor, ver_0, hor + width, ver_1, color);
}

void
epaper_set_rotate(int rotate)
{
    if (rotate >= 0 && rotate < 4) {
        paint_set_rotate(&paint, rotate);
    }
}

void
epaper_display_part()
{
    epd_wait_until_idle(&epd);
    epd_display_black(&epd, screen);
}

void
epaper_wait()
{
    epd_wait_until_idle(&epd);
}
