#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <RFID.h>
#include "FirebaseESP8266.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

#define FIREBASE_HOST "rfidattendance-c6ac1-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "PHQxsOlNdVfhryHWzD2H2hMPKaJnrl2k7AQFQFxZ"
RFID rfid(D8, D0);
unsigned char str[MAX_LEN];
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 19800; //(UTC+5:30)
NTPClient timeClient(ntpUDP, "pool.ntp.org");
const char ssid[] = "V2030";
const char pass[] = "ashu8431";
String uidPath = "/";
FirebaseJson json;
FirebaseData firebaseData;
String device_id = "device11";
boolean checkIn = true;

void connect() {
  Serial.print("Checking WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected!");
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  SPI.begin();
  rfid.init();
  timeClient.begin();
  timeClient.setTimeOffset(utcOffsetInSeconds);
  connect();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
}

void checkAccess(String temp) {
  if (Firebase.getInt(firebaseData, uidPath + "/users/" + temp)) {
    if (firebaseData.intData() == 0) {
      json.add("time", String(timeClient.getFormattedTime()));
      json.add("id", device_id);
      json.add("uid", temp);
      json.add("status", 1);
      Firebase.setInt(firebaseData, uidPath + "/users/" + temp, 1);
      if (Firebase.pushJSON(firebaseData, uidPath + "/attendance", json)) {
        Serial.println(firebaseData.dataPath() + firebaseData.pushName());
      } else {
        Serial.println(firebaseData.errorReason());
      }
    } else if (firebaseData.intData() == 1) {
      Firebase.setInt(firebaseData, uidPath + "/users/" + temp, 0);
      json.add("time", String(timeClient.getFormattedTime()));
      json.add("id", device_id);
      json.add("uid", temp);
      json.add("status", 0);
      if (Firebase.pushJSON(firebaseData, uidPath + "/attendance", json)) {
        Serial.println(firebaseData.dataPath() + firebaseData.pushName());
      } else {
        Serial.println(firebaseData.errorReason());
      }
    }
  } else {
    Serial.println("FAILED");
    Serial.println("REASON: " + firebaseData.errorReason());
  }
}

void loop() {
  timeClient.update();
  if (rfid.findCard(PICC_REQIDL, str) == MI_OK) {
    Serial.println("Card found");
    String temp = "";
    if (rfid.anticoll(str) == MI_OK) {
      Serial.print("The card's ID number is : ");
      for (int i = 0; i < 4; i++) {
        temp = temp + (0x0F & (str[i] >> 4));
        temp = temp + (0x0F & str[i]);
      }
      Serial.println(temp);
      checkAccess(temp);
    }
    rfid.selectTag(str);
  }
  rfid.halt();
}
