#include <WiFi.h>
#include <esp_wifi.h>
#include "types.h"
#include "web_interface.h"
#include "deauth.h"
#include "definitions.h"
#include "applejuice.h"
int curr_channel = 1;

void setup() {
// #ifdef SERIAL_DEBUG
//   Serial.begin(115200);
// #endif
// #ifdef LED
//   pinMode(LED, OUTPUT);
// #endif
  applejuice_init();
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
   // 设置WiFi功率为最大
  esp_wifi_set_max_tx_power(82);
  start_web_interface();
}

void loop() {
  if (deauth_type == DEAUTH_TYPE_ALL) {
    if (curr_channel > CHANNEL_MAX) curr_channel = 1;
    esp_wifi_set_channel(curr_channel, WIFI_SECOND_CHAN_NONE);
    curr_channel++;
    // web_interface_handle_client();
    delay(10);
  } else {
    web_interface_handle_client();
    web_interface_handle_client();
  }
}