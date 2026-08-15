#ifndef EPDPAINT_H
#define EPDPAINT_H

// Display orientation
#define ROTATE_0 0
#define ROTATE_90 1
#define ROTATE_180 2
#define ROTATE_270 3

// Color inverse. 1 or 0 = set or reset a bit if set a colored pixel
#define IF_INVERT_COLOR 1

#include "fonts.h"

typedef struct {
    unsigned char *image;
    int width;
    int height;
    int rotate;
} paint_t;

void
paint_init(paint_t *paint, unsigned char *image, int width, int height);
void
paint_clear(paint_t *paint, int colored);
int
paint_get_width(paint_t *paint);
void
paint_set_width(paint_t *paint, int width);
int
paint_get_height(paint_t *paint);
void
paint_set_height(paint_t *paint, int height);
int
paint_get_rotate(paint_t *paint);
void
paint_set_rotate(paint_t *paint, int rotate);
unsigned char *
paint_get_image(paint_t *paint);
void
paint_draw_absolute_pixel(paint_t *paint, int x, int y, int colored);
void
paint_draw_pixel(paint_t *paint, int x, int y, int colored);
void
paint_draw_char_at(paint_t *paint, int x, int y, char ascii_char, sFONT *font, int colored);
void
paint_draw_string_at(paint_t *paint, int x, int y, const char *text, sFONT *font, int colored);
void
paint_draw_line(paint_t *paint, int x0, int y0, int x1, int y1, int colored);
void
paint_draw_horizontal_line(paint_t *paint, int x, int y, int width, int colored);
void
paint_draw_vertical_line(paint_t *paint, int x, int y, int height, int colored);
void
paint_draw_rectangle(paint_t *paint, int x0, int y0, int x1, int y1, int colored);
void
paint_draw_filled_rectangle(paint_t *paint, int x0, int y0, int x1, int y1, int colored);
void
paint_draw_circle(paint_t *paint, int x, int y, int radius, int colored);
void
paint_draw_filled_circle(paint_t *paint, int x, int y, int radius, int colored);

#endif
