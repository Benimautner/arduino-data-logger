#include <ccs811.h>

#include <ArduinoJson.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "defines.h"
#include "ccs811.h"
#define DEVID 3
#define RTCMEMORYSTART 65
#define MAX_TIMEOUT 10000
#define BME_ADDR 0x76
#define SEALEVELPRESSURE_HPA (1013.25)


struct {
  bool initialized;
  bool err;
} rtcMem;

const int capacity = JSON_OBJECT_SIZE(10);
StaticJsonDocument<capacity> doc;

int temp;

X509List cert(rootCACertificate);

Adafruit_BME280 bme; // I2C


void setup() {
  //Serial.begin(115200);
  //Serial.setTimeout(2000);
  //delay(500);
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin();
  
  uint16_t eco2, etvoc, errstat, raw;
  String out;
  bool sent = false;
  bool failed = false;
  bool data_ready = false;

  Wire.begin();                      // Setup iic/twi
  prepareDoc(doc);


  if (!rtcMem.initialized) {
    if (!bme.begin(BME_ADDR)) {
      Serial.println("Connection to css811 failed");
      rtcMem.err = true;
    }


    rtcMem.initialized = true;
    Serial.println("Setup Done");
  }


  doc["measurements"]["temperature"] = bme.readTemperature();
  doc["measurements"]["pressure"] = bme.readPressure() / 100.0F;
  doc["measurements"]["humidity"] = bme.readHumidity();

  Serial.print("read temp: ");
  Serial.println((float)doc["measurements"]["temperature"]);

  serializeJson(doc, out);
  digitalWrite(LED_BUILTIN, 1);
  connectWifi();
  sendData(out);
  digitalWrite(LED_BUILTIN, 0);

  delay(1000);
  Serial.println("going to sleep");
  Serial.flush();
  ESP.deepSleep(10e6);
}

void loop() {}


/*
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
*/

void prepareDoc(StaticJsonDocument<capacity>& doc) {
  doc["key"] = KEY;
  doc["devid"] = DEVID;
  doc["measurements"]["pressure"] = 0.0;
  doc["measurements"]["temperature"] = 0.0;
  doc["measurements"]["humidity"] = 0.0;
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASSWD);
  while ((WiFi.status() != WL_CONNECTED)) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wifi connected");
}


void sendData(String data) {
  Serial.println("Starting transmission");
  WiFiClientSecure client;
  client.setInsecure();
  //client.setFingerprint("B5:CC:2C:98:ED:47:60:3F:64:ED:4B:BB:61:D2:38:70:FC:8C:0F:2C");

  if (!client.connect("yar", 4445)) {
    Serial.println("Didn't connect");
    Serial.println(client.getLastSSLError());
    return;
  }
  client.println("POST /submit HTTP/1.1");
  client.println("Host: yar");
  client.println("User-Agent: esp8266/1.0");
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(data.length());
  client.println();
  client.println(data);

  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.println(line);
  }

  return;
}
