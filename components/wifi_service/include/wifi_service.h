#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

int
connect_sta(const char *ssid, const char *pwd);
int
start_ap();
int
wifi_init(void);
void
wifi_stop();

#endif