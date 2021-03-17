#include <ccs811.h>

#include <ArduinoJson.h>
#include <mh-z14a.h>

#include <Wire.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "defines.h"
#include "ccs811.h"
#define DEVID 2

// Address of TC74A7 temperature sensor (from datasheet)
#define TC74A7ADDR 0x4f
// Initialize the library with the numbers of the interface pins

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

#define MAX_TIMEOUT 20000

WiFiMulti WiFiMulti;
CCS811 ccs(10); // 10 = D34; arg1 = nWAKE
Z14A sensor;

RTC_DATA_ATTR bool initialized = false;

const int capacity = JSON_OBJECT_SIZE(10);
StaticJsonDocument<capacity> doc;

int temp;


void setup() {
  uint16_t eco2, etvoc, errstat, raw;
  String out;
  bool sent = false;
  bool failed = false;
  bool data_ready = false;
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  Serial.begin(115200);
  Wire.begin();                      // Setup iic/twi
  prepareDoc(doc);
  sensor.init();


  if (!initialized) {
    if (!ccs.begin()) {
      Serial.println("Connection to css811 failed");
      failed = true;
    }

    bool success_start = ccs.start(CCS811_MODE_1SEC);
    if (!success_start) failed = true;
    initialized = true;
    Serial.println("Setup Done");
  }

  int startTime = millis();
  while (errstat != CCS811_ERRSTAT_OK) {
    int diff = millis() - startTime;
    failed = (diff) > MAX_TIMEOUT;

    ccs.read(&eco2, &etvoc, &errstat, &raw);
    Serial.println("co2: " + String(eco2) + "; etVoc: " + String(etvoc));
    delay(1000);
  }

  bool success = false;
  bool temp_err = false;
  startTime = millis();
  //while(!success && !temp_err) {
  //  success = getTempTc(temp);
  //  temp_err = millis()-startTime > MAX_TIMEOUT;
  //}
  int co2_concentration = 0;

  if (sensor.getValue(co2_concentration) == Z14A_OK) {
    doc["measurements"]["realco2"] = co2_concentration;
  }


  if (failed) Serial.println("Reached MAX_TIMEOUT at read -- skipping");

  if (!failed) {
    doc["measurements"]["voc"] = etvoc;
    doc["measurements"]["co2"] = eco2;
    doc["measurements"]["temperature"] = temp_err ? 0 : temp;
    serializeJson(doc, out);
    connectWifi();
    sendData(out);
  }
  delay(1000);
  Serial.println("going to sleep");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {}


bool getTempTc(int& rtnval) {
  int temp = 0;
  Wire.beginTransmission(TC74A7ADDR);
  Wire.write(0x00);
  // request 1 byte from sensor
  Wire.requestFrom(TC74A7ADDR, 1);

  if (Wire.available()) {
    temp = Wire.read();
    if (temp > 127) {
      temp = 255 - temp + 1;
      temp = temp * -1;
    }
    Wire.endTransmission();
    rtnval = temp;
    return true;
  } else {
    return false;
  }
}

void prepareDoc(StaticJsonDocument<capacity>& doc) {
  doc["key"] = KEY;
  doc["devid"] = DEVID;
  doc["measurements"]["voc"] = 0.0;
  doc["measurements"]["co2"] = 0.0;
  doc["measurements"]["temperature"] = 0.0;
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(WLAN_SSID, WLAN_PASSWD);
  bool conn_err = false;
  int start_time = millis();
  while ((WiFiMulti.run() != WL_CONNECTED) || !conn_err) {
    Serial.print(".");
    conn_err = millis()-start_time > MAX_TIMEOUT;
  }
  if(conn_err) {
    // error connecting to wifi. resetting
    // starting sleeping prematurely
    esp_deep_sleep_start();
  }
}


void sendData(String data) {
  Serial.println("Starting transmission");
  HTTPClient http;
  http.begin("https://yar:4445/submit", rootCACertificate);
  int httpCode = http.POST(data);
  Serial.println("Sent Post");

  if (httpCode > 0) { //Check for the returning code
    Serial.println("HTTP code: " + httpCode);
  }
  else {
    Serial.println("Error on HTTP request: " + httpCode);
  }
  Serial.println("Sent");
  http.end();
  return;
}
