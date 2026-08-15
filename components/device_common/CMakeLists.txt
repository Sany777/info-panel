idf_component_register(SRC_DIRS .
                        INCLUDE_DIRS "include"
                        REQUIRES device_macro
                        PRIV_REQUIRES 
                            driver
                            esp_driver_gpio 
                            esp_driver_touch_sens
                            device_memory 
                            wifi_service 
                            adc_reader 
                            sound_generator   
                            esp_timer 
                            clock_module
                    ) 