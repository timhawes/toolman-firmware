#ifndef CONFIG_H
#define CONFIG_H

// password used in setup mode for the self-hosted WiFi AP
#define SETUP_PASSWORD "password"

// filenames
#ifdef ESP8266
#define APP_JSON_FILENAME "app.json"
#define NET_JSON_FILENAME "net.json"
#define WIFI_JSON_FILENAME "wifi.json"
#define TOKENS_FILENAME "tokens.dat"
#else
#define APP_JSON_FILENAME "/app.json"
#define NET_JSON_FILENAME "/net.json"
#define WIFI_JSON_FILENAME "/wifi.json"
#define TOKENS_FILENAME "/tokens.dat"
#endif

#endif
