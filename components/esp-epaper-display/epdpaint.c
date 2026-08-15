#include "epdpaint.h"
#include "device_macro.h"

void
paint_init(paint_t *paint, unsigned char *image, int width, int height)
{
    paint->rotate = ROTATE_0;
    paint->image  = image;
    paint->width  = width % 8 ? width + 8 - (width % 8) : width;
    paint->height = height;
}

void
paint_clear(paint_t *paint, int colored)
{
    for (int x = 0; x < paint->width; x++) {
        for (int y = 0; y < paint->height; y++) {
            paint_draw_absolute_pixel(paint, x, y, colored);
        }
    }
}

void
paint_draw_absolute_pixel(paint_t *paint, int x, int y, int colored)
{
    if (x < 0 || x >= paint->width || y < 0 || y >= paint->height) {
        return;
    }
    if (IF_INVERT_COLOR) {
        if (colored) {
            paint->image[(x + y * paint->width) / 8] |= 0x80 >> (x % 8);
        } else {
            paint->image[(x + y * paint->width) / 8] &= ~(0x80 >> (x % 8));
        }
    } else {
        if (colored) {
            paint->image[(x + y * paint->width) / 8] &= ~(0x80 >> (x % 8));
        } else {
            paint->image[(x + y * paint->width) / 8] |= 0x80 >> (x % 8);
        }
    }
}

unsigned char *
paint_get_image(paint_t *paint)
{
    return paint->image;
}

int
paint_get_width(paint_t *paint)
{
    return paint->width;
}

void
paint_set_width(paint_t *paint, int width)
{
    paint->width = width % 8 ? width + 8 - (width % 8) : width;
}

int
paint_get_height(paint_t *paint)
{
    return paint->height;
}

void
paint_set_height(paint_t *paint, int height)
{
    paint->height = height;
}

int
paint_get_rotate(paint_t *paint)
{
    return paint->rotate;
}

void
paint_set_rotate(paint_t *paint, int rotate)
{
    paint->rotate = rotate;
}

void
paint_draw_pixel(paint_t *paint, int x, int y, int colored)
{
    int point_temp;
    if (paint->rotate == ROTATE_0) {
        if (x < 0 || x >= paint->width || y < 0 || y >= paint->height) {
            return;
        }
        paint_draw_absolute_pixel(paint, x, y, colored);
    } else if (paint->rotate == ROTATE_90) {
        if (x < 0 || x >= paint->height || y < 0 || y >= paint->width) {
            return;
        }
        point_temp = x;
        x          = paint->width - 1 - y;
        y          = point_temp;
        paint_draw_absolute_pixel(paint, x, y, colored);
    } else if (paint->rotate == ROTATE_180) {
        if (x < 0 || x >= paint->width || y < 0 || y >= paint->height) {
            return;
        }
        x = paint->width - x;
        y = paint->height - y;
        paint_draw_absolute_pixel(paint, x, y, colored);
    } else if (paint->rotate == ROTATE_270) {
        if (x < 0 || x >= paint->height || y < 0 || y >= paint->width) {
            return;
        }
        point_temp = x;
        x          = y;
        y          = paint->height - point_temp;
        paint_draw_absolute_pixel(paint, x, y, colored);
    }
}

void
paint_draw_char_at(paint_t *paint, int x, int y, char ascii_char, sFONT *font, int colored)
{
    int i, j;
    unsigned int char_offset = (ascii_char - ' ') * font->Height * (font->Width / 8 + (font->Width % 8 ? 1 : 0));
    const unsigned char *ptr = &font->table[char_offset];

    for (j = 0; j < font->Height; j++) {
        for (i = 0; i < font->Width; i++) {
            if (*ptr & (0x80 >> (i % 8))) {
                paint_draw_pixel(paint, x + i, y + j, colored);
            }
            if (i % 8 == 7) {
                ptr++;
            }
        }
        if (font->Width % 8 != 0) {
            ptr++;
        }
    }
}

void
paint_draw_string_at(paint_t *paint, int x, int y, const char *text, sFONT *font, int colored)
{
    const char *p_text = text;
    int refcolumn      = x;

    while (*p_text != 0) {
        paint_draw_char_at(paint, refcolumn, y, *p_text, font, colored);
        refcolumn += font->Width;
        p_text++;
    }
}

void
paint_draw_line(paint_t *paint, int x0, int y0, int x1, int y1, int colored)
{
    int dx  = x1 - x0 >= 0 ? x1 - x0 : x0 - x1;
    int sx  = x0 < x1 ? 1 : -1;
    int dy  = y1 - y0 <= 0 ? y1 - y0 : y0 - y1;
    int sy  = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while ((x0 != x1) && (y0 != y1)) {
        paint_draw_pixel(paint, x0, y0, colored);
        if (2 * err >= dy) {
            err += dy;
            x0 += sx;
        }
        if (2 * err <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void
paint_draw_horizontal_line(paint_t *paint, int x, int y, int line_width, int colored)
{
    int i;
    for (i = x; i < x + line_width; i++) {
        paint_draw_pixel(paint, i, y, colored);
    }
}

void
paint_draw_vertical_line(paint_t *paint, int x, int y, int line_height, int colored)
{
    int i;
    for (i = y; i < y + line_height; i++) {
        paint_draw_pixel(paint, x, i, colored);
    }
}

void
paint_draw_rectangle(paint_t *paint, int x0, int y0, int x1, int y1, int colored)
{
    int min_x, min_y, max_x, max_y;
    min_x = MIN(x0, x1);
    max_x = MAX(x0, x1);
    min_y = MIN(y0, y1);
    max_y = MAX(y0, y1);

    paint_draw_horizontal_line(paint, min_x, min_y, max_x - min_x + 1, colored);
    paint_draw_horizontal_line(paint, min_x, max_y, max_x - min_x + 1, colored);
    paint_draw_vertical_line(paint, min_x, min_y, max_y - min_y + 1, colored);
    paint_draw_vertical_line(paint, max_x, min_y, max_y - min_y + 1, colored);
}

void
paint_draw_filled_rectangle(paint_t *paint, int x0, int y0, int x1, int y1, int colored)
{
    int min_x, min_y, max_x, max_y;
    int i;
    min_x = MIN(x0, x1);
    max_x = MAX(x0, x1);
    min_y = MIN(y0, y1);
    max_y = MAX(y0, y1);

    for (i = min_x; i <= max_x; i++) {
        paint_draw_vertical_line(paint, i, min_y, max_y - min_y + 1, colored);
    }
}

void
paint_draw_circle(paint_t *paint, int x, int y, int radius, int colored)
{
    int x_pos = -radius;
    int y_pos = 0;
    int err   = 2 - 2 * radius;
    int e2;

    do {
        paint_draw_pixel(paint, x - x_pos, y + y_pos, colored);
        paint_draw_pixel(paint, x + x_pos, y + y_pos, colored);
        paint_draw_pixel(paint, x + x_pos, y - y_pos, colored);
        paint_draw_pixel(paint, x - x_pos, y - y_pos, colored);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if (-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}

void
paint_draw_filled_circle(paint_t *paint, int x, int y, int radius, int colored)
{
    int x_pos = -radius;
    int y_pos = 0;
    int err   = 2 - 2 * radius;
    int e2;

    do {
        paint_draw_pixel(paint, x - x_pos, y + y_pos, colored);
        paint_draw_pixel(paint, x + x_pos, y + y_pos, colored);
        paint_draw_pixel(paint, x + x_pos, y - y_pos, colored);
        paint_draw_pixel(paint, x - x_pos, y - y_pos, colored);
        paint_draw_horizontal_line(paint, x + x_pos, y + y_pos, 2 * (-x_pos) + 1, colored);
        paint_draw_horizontal_line(paint, x + x_pos, y - y_pos, 2 * (-x_pos) + 1, colored);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if (-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}
