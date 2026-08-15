#ifndef device_memory_H_
#define device_memory_H_

int
read_flash(const char *data_name, unsigned char *buf, unsigned data_size);
int
write_flash(const char *data_name, unsigned char *buf, unsigned data_size);

#endif