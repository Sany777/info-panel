set(srcs "epdif.c" "epdpaint.c" "epd2.9.c" "font8.c" "font12.c" "font16.c" "font20.c" "font24.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_common driver device_common esp_driver_gpio esp_driver_spi)