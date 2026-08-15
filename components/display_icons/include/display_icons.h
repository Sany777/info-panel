#ifndef DISPLAY_ICONS_H
#define DISPLAY_ICONS_H

#include "bitmap_icons.h"

const unsigned char *
get_battery_icon_bitmap(const int percentage);
const unsigned char *
get_forecast_data_icon(int id, int day);

#endif