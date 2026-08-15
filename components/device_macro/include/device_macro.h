#ifndef DEVICE_MACRO_H
#define DEVICE_MACRO_H

#include "esp_err.h"
#include "esp_log.h"
#include "stddef.h"

#define CHECK_AND_RET_ERR(result_)                                                                                     \
    do {                                                                                                               \
        const int e = result_;                                                                                         \
        if (e != ESP_OK) {                                                                                             \
            ESP_LOGE(__func__, "%s", esp_err_to_name(e));                                                              \
            return e;                                                                                                  \
        }                                                                                                              \
    } while (0)

#define CHECK_AND_GO(result_, label_)                                                                                  \
    do {                                                                                                               \
        const int e = result_;                                                                                         \
        if (e != ESP_OK) {                                                                                             \
            ESP_LOGE(__func__, "%s", esp_err_to_name(e));                                                              \
            goto label_;                                                                                               \
        }                                                                                                              \
    } while (0)

#define CHECK_AND_RET(err_)                                                                                            \
    do {                                                                                                               \
        const int e = err_;                                                                                            \
        if (e != ESP_OK) {                                                                                             \
            ESP_LOGE(__func__, "%s", esp_err_to_name(e));                                                              \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define flag_reset(flags, index) ((flags) &= ~(1 << (index)))

#define flag_set(flags, index) ((flags) |= (1 << (index)))

#define flag_set_value(flags, index, val) ((val) ? flag_set((flags), (index)) : flag_reset((flags), (index)))

#define flag_get(flags, index) ((flags) & (1 << (index)))

#ifndef MIN
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef CLAMP
#define CLAMP(x, low, high) (MIN(MAX(x, low), high))
#endif
#ifndef ARR_LEN
#define ARR_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#endif
