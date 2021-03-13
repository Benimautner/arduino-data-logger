#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#include <ccs811.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

#include "defines.h"
#include "ccs811.h"
#define DEVID 3
#define MAX_TIMEOUT 10000
#define BME_ADDR 0x76
#define SEA_LEVEL_PRESSURE_HPA (1013.25)


typedef struct {
  bool err;
  bool initialized;
  uint8_t cnt;
} rtc_mem_t;

rtc_mem_t rtc_mem;

const int capacity = JSON_OBJECT_SIZE(10);
StaticJsonDocument<capacity> doc;

int temp;

LiquidCrystal_I2C lcd(0x27, 20, 4);

Adafruit_BME280 bme; // I2C


void setup() {
  ESP.rtcUserMemoryRead(0, (uint32_t*) &rtc_mem, sizeof(rtc_mem));
  Serial.begin(115200);
  //Serial.setTimeout(2000);
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin();
  lcd.backlight(); //enabling it before initializing to avoid flashing
  lcd.init();

  rtc_mem.err = rtc_mem.err ? true : false;
  rtc_mem.initialized = false;


  String out;


  Wire.begin();                      // Setup iic/twi
  prepareDoc(doc);


  if (!bme.begin(BME_ADDR)) {
    Serial.println("Connection to css811 failed");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR BME280");
    delay(2000);
    rtc_mem.err = true;
  } else {
    Serial.println("Successfully initialized");
    rtc_mem.err = false;
  }
  rtc_mem.initialized = true;
  Serial.println("Setup Done");


  float temperature = 0;
  float pressure = 0;
  float humidity = 0;

  if (!rtc_mem.err) {
    temperature = bme.readTemperature();
    pressure = bme.readPressure() / 100.0F;
    humidity = bme.readHumidity();
  }


  doc["measurements"]["temperature"] = temperature;
  doc["measurements"]["pressure"] = pressure;
  doc["measurements"]["humidity"] = humidity;

  Serial.print("read temp: ");
  Serial.println((float)doc["measurements"]["temperature"]);


  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:" + String(temperature) + "C");
  lcd.setCursor(0, 1);
  lcd.print("Pres:" + String(pressure) + "hPa");


  serializeJson(doc, out);
  digitalWrite(LED_BUILTIN, 1);
  connectWifi();
  if(!rtc_mem.err) {
    sendData(out);
  }
  digitalWrite(LED_BUILTIN, 0);

  delay(1000);
  Serial.println("going to sleep");
  Serial.flush();

  ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtc_mem, sizeof(rtc_mem));

  ESP.deepSleep(10e6);
}

void loop() {}


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
