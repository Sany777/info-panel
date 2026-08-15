#ifndef CLOCK_MODULE_H
#define CLOCK_MODULE_H

#include "stdbool.h"
#include <sys/time.h>

struct tm *
get_cur_time_tm(void);
void
init_sntp();
void
stop_sntp();
const char *
snprintf_time(const char *format);
void
set_offset(int offset_hours, bool use_dst);

#endif