#include "sound_generator.h"
#include "device_common.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <stdbool.h>
#include <stddef.h>

#define TICK_PERIOD_MS 10

typedef struct {
    uint16_t on_ms;
    uint16_t off_ms;
} sound_step_t;

static const sound_step_t sig_short[] = {{25, 0}};
static const sound_step_t sig_long[]  = {{50, 0}};

static const sound_step_t *current_table = NULL;
static int current_step                  = 0;
static int current_repeat                = 0;
static int max_repeats                   = 0;
static int tick_counter                  = 0;
static bool buzzer_on                    = false;

static bool custom_series = false;
static sound_step_t custom_step;

static esp_timer_handle_t sound_timer;

static void
sound_timer_cb(void *arg)
{
    if (tick_counter > 0) {
        tick_counter -= TICK_PERIOD_MS;
        if (tick_counter > 0)
            return;
    }

    if (buzzer_on) {
        buzzer_on = false;
        gpio_set_level((gpio_num_t)PIN_BUZZER, 0);

        uint16_t off_time = custom_series ? custom_step.off_ms : current_table[current_step].off_ms;
        if (off_time > 0) {
            tick_counter = off_time;
        } else {
            current_repeat++;
            if (current_repeat >= max_repeats) {
                device_clear_state(BIT_WAIT_SIGNALE);
                esp_timer_stop(sound_timer);
            } else {
                buzzer_on = true;
                gpio_set_level((gpio_num_t)PIN_BUZZER, 1);
                tick_counter = custom_series ? custom_step.on_ms : current_table[current_step].on_ms;
            }
        }
    } else {
        current_repeat++;
        if (current_repeat >= max_repeats) {
            device_clear_state(BIT_WAIT_SIGNALE);
            esp_timer_stop(sound_timer);
        } else {
            buzzer_on = true;
            gpio_set_level((gpio_num_t)PIN_BUZZER, 1);
            tick_counter = custom_series ? custom_step.on_ms : current_table[current_step].on_ms;
        }
    }
}

void
sound_generator_init(void)
{
    esp_timer_create_args_t timer_args = {.callback = &sound_timer_cb, .name = "sound_timer"};
    esp_timer_create(&timer_args, &sound_timer);
}

static void
start_sound(const sound_step_t *table, bool custom, uint16_t on_time, uint16_t off_time, int count)
{
    esp_timer_stop(sound_timer);
    gpio_set_level((gpio_num_t)PIN_BUZZER, 0);

    current_table = table;
    custom_series = custom;
    if (custom) {
        custom_step.on_ms  = on_time;
        custom_step.off_ms = off_time;
    }
    current_step   = 0;
    current_repeat = 0;
    max_repeats    = count;

    buzzer_on = true;
    gpio_set_level((gpio_num_t)PIN_BUZZER, 1);
    tick_counter = custom ? custom_step.on_ms : current_table[current_step].on_ms;

    device_set_state(BIT_WAIT_SIGNALE);
    esp_timer_start_periodic(sound_timer, TICK_PERIOD_MS * 1000);
}

void
start_single_signale(unsigned delay)
{
    start_sound(NULL, true, delay, 0, 1);
}

void
short_signale()
{
    start_sound(sig_short, false, 0, 0, 1);
}

void
long_signale()
{
    start_sound(sig_long, false, 0, 0, 1);
}

void
sound_off()
{
    esp_timer_stop(sound_timer);
    gpio_set_level((gpio_num_t)PIN_BUZZER, 0);
    device_clear_state(BIT_WAIT_SIGNALE);
}

void
start_signale_series(unsigned delay, unsigned count)
{
    if (!(device_get_state() & BIT_WAIT_SIGNALE) && delay) {
        start_sound(NULL, true, delay / 2, delay / 2, count);
    }
}
