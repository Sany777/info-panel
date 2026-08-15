# Info Panel (ESP32 E-Paper)

## Platform
- **Microcontroller**: ESP32
- **Framework**: ESP-IDF v6.0.1
- **Display**: 2.9" WeAct Studio E-Paper (3-color)
- **Power**: Rechargeable battery (voltage monitoring via ADC)

## Connectivity & Controls
- **Left Touch Button**: Wake up + start AP settings server. The screen displays the SSID, password, and IP (usually 192.168.4.1). The server automatically shuts down after 2 minutes of inactivity.
- **Right Touch Button**: Wake up + force weather data update (OpenWeatherMap) and screen refresh.
- **Operating Mode**: The device operates in deep/light sleep mode, waking up periodically via a timer to update data, or via touch button events.

## OpenWeatherMap Setup

To display weather data, the device requires an active OpenWeatherMap API key:
1. Register at [OpenWeatherMap](https://openweathermap.org/) and generate a free API key.
2. Touch the **Touch Button** on the device to wake it up and start the Settings Server (AP mode).
3. Connect your phone or PC to the device's Wi-Fi network. The SSID, password, and IP address will be displayed on the e-paper screen.
4. Open a web browser and navigate to the IP address shown on the screen (`http://192.168.4.1`).
5. In the web interface, enter:
   - Your local Wi-Fi SSID and Password.
   - Your OpenWeatherMap API Key.
   - Your City name.
6. Save the settings. The device will automatically connect to your Wi-Fi and fetch the  weather forecast.

## Modules

- **adc_reader** - reads the battery voltage level via ADC.
- **clock_module** - synchronizes time via SNTP.
- **device_common** - manages the global device state.
- **device_macro** - global constants and macros.
- **device_memory** - handles non-volatile storage (NVS) for saving settings.
- **device_task** - core event logic, sleep management, and touch button handling.
- **display_icons** - icon bitmaps for screen rendering.
- **epaper_adapter** / **esp-epaper-display** - driver and adapter for the e-paper display via SPI.
- **forecast** - HTTP client for OpenWeatherMap requests and response parsing (via cJSON).
- **setting_server** - HTTP server for the Web settings interface (serves HTML/CSS/JS, processes API endpoints).
- **sound_generator** - controls the audio buzzer (via PWM).
- **toolbox** - general helper utilities.
- **wifi_service** - Wi-Fi management (connects as an STA client or starts an AP).
