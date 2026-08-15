#ifndef SOUND_GENERATOR_H_
#define SOUND_GENERATOR_H_

void sound_generator_init(void);
void start_single_signale(unsigned delay);
void start_signale_series(unsigned delay, unsigned count);
void sound_off();
void short_signale();
void long_signale();
void sig_disable();

#endif