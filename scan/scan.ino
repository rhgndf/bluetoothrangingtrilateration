
/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

#define MQTT_MAX_PACKET_SIZE 8192

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <BluetoothSerial.h>

#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

#include <ArduinoJson.h>

#include <TFT_eSPI.h>

#include <bits/stdc++.h>
using namespace std;

#define DEVICE_INDEX 3
#define DEVICE_ROOM "default"

#define WIFI_SSID "SUTD_LAB"
#define WIFI_PASSWORD ""

#define HTTP_SERVER "https://165.22.249.102:8000/"
#define MQTT_SERVER "165.22.249.102"
#define MQTT_PORT 1883

#define STR_INDIR(x) #x
#define STR(x) STR_INDIR(x)

int scanTime = 5; // In seconds
BLEScan *pBLEScan;
BluetoothSerial SerialBT;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
int mqtt_failures = 0;

#define TFT_W 135
#define TFT_H 240
TFT_eSPI tft = TFT_eSPI(TFT_W, TFT_H);

vector<tuple<string, string, int>> devices;

int scantype = 0;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    devices.emplace_back(advertisedDevice.getAddress().toString(), advertisedDevice.getName(), advertisedDevice.getRSSI());
    Serial.printf("Advertised Device: %s %s %d\n", advertisedDevice.getAddress().toString().c_str(), advertisedDevice.getName().c_str(), advertisedDevice.getRSSI());
  }
};

void onWiFiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("WiFi disconnect, restarting");
  ESP.restart();
}
void print_tft_header() {
  /*tft.fillScreen(0);
  tft.setCursor(0,0);
  String ip = WiFi.localIP().toString();
  tft.printf("Connected\n%s\n",ip.c_str());*/
}
void setup()
{
  Serial.begin(115200);
  Serial.println("Device name: " DEVICE_ROOM "_" STR(DEVICE_INDEX));
  
  /*tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.printf("WiFi: " WIFI_SSID " ");*/

  //WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    tft.print('.');
    delay(1000);
    retry++;
    if (retry % 5 == 0) {
      WiFi.reconnect();
    }
    if (retry > 30) {
      ESP.restart();
    }
  }
  print_tft_header();
  WiFi.onEvent(onWiFiDisconnect, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  Serial.println(WiFi.localIP());
  mqttClient.setBufferSize(8192);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  Serial.println("MQTT connected.");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value

  SerialBT.begin("");

}

int send_to_mqtt(bool type) {
  // Make JSON
  DynamicJsonDocument doc(4096);
  doc["index"] = DEVICE_INDEX;
  doc["room"] = DEVICE_ROOM;
  doc.createNestedArray("devices");
  for (auto &devices : devices)
  {
    string mac, name;
    int rssi;
    tie(mac, name, rssi) = devices;
    JsonObject device = doc["devices"].createNestedObject();
    device["mac"] = mac;
    device["name"] = name;
    device["rssi"] = rssi;
  }
  doc["type"] = type ? "BLE": "BtClassic";
  int jsonlen = measureJson(doc);
  char jsonstr[jsonlen + 1];
  serializeJson(doc, jsonstr, jsonlen + 1);

  if(!mqttClient.connected()) return 0;
  bool published = mqttClient.publish("room/bt", jsonstr);
  if(published) {
    mqtt_failures = 0;
  } else {
    mqtt_failures++;
    if (mqtt_failures > 2) {
      ESP.restart();
    }
  }
  print_tft_header();
  Serial.println("Scan done!");
  Serial.printf("Devices found: %d\n", devices.size());
  Serial.printf("JSON: %d %s\n", jsonlen, jsonstr);
  Serial.printf("MQTT response: %d\n", published);
  /*tft.printf("%s scan done!\n", type ? "BLE": "BtClassic");
  tft.printf("Devices found: %d\n", devices.size());
  tft.printf("JSON length: %d\n", jsonlen);
  tft.printf("MQTT response: %d\n", published);*/

  return published;
}

void ble_scan()
{
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
}
void btclassic_scan() {
  BTScanResults *pResults = SerialBT.discover(scanTime * 1000);
  int results = pResults->getCount();
  for(int i = 0;i < results;i++) {
    BTAdvertisedDevice* pDevice = pResults->getDevice(i);
    devices.emplace_back(pDevice->getAddress().toString(), pDevice->getName(), pDevice->getRSSI());
  }
  /*if(SerialBT.discoverAsync(btAdvertisedDeviceFound)) {
    delay(5000);
    SerialBT.discoverAsyncStop();
  }*/
}
void loop()
{
  while (!mqttClient.connected()) {
    Serial.println("MQTT disconnect, reconnecting");
    print_tft_header();
    //tft.printf("MQTT disconnect, reconnecting\n");
    mqttClient.connect(DEVICE_ROOM "_" STR(DEVICE_INDEX));
    delay(5000);
  }
  devices.clear();
  if(scantype) {
    ble_scan();
  } else {
    btclassic_scan();
  }
  send_to_mqtt(scantype);
  scantype = !scantype;
}
