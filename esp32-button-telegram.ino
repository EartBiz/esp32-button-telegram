/*
 * ESP32: button press → Telegram message
 * 1. Copy secrets.h.example → secrets.h and fill Wi-Fi + bot token + chat id
 * 2. Board: ESP32 Dev Module
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "secrets.h"

#ifndef BUTTON_PIN
#define BUTTON_PIN 4
#endif
#ifndef LED_PIN
#define LED_PIN 2
#endif

const unsigned long DEBOUNCE_MS = 40;
const unsigned long COOLDOWN_MS = 1500;

bool lastStable = HIGH;
unsigned long lastChangeMs = 0;
unsigned long lastSendMs = 0;

bool sendTelegram(const String &text) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  String url = String("https://api.telegram.org/bot") + BOT_TOKEN + "/sendMessage";
  if (!https.begin(client, url)) {
    return false;
  }
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String body = "chat_id=" + String(CHAT_ID) + "&text=" + text;
  int code = https.POST(body);
  https.end();
  return code > 0 && code < 400;
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
  digitalWrite(LED_PIN, WiFi.status() == WL_CONNECTED ? HIGH : LOW);
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.begin(115200);
  delay(200);
  Serial.println("ESP32 button → Telegram");
  connectWifi();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    sendTelegram("ESP32 online: " + WiFi.localIP().toString());
  } else {
    Serial.println("Wi-Fi failed");
  }
}

void loop() {
  bool raw = digitalRead(BUTTON_PIN);
  unsigned long now = millis();
  if (raw != lastStable && (now - lastChangeMs) > DEBOUNCE_MS) {
    lastChangeMs = now;
    lastStable = raw;
    if (raw == LOW && (now - lastSendMs) > COOLDOWN_MS) {
      lastSendMs = now;
      digitalWrite(LED_PIN, LOW);
      bool ok = sendTelegram("Button pressed on device-01");
      Serial.println(ok ? "Telegram OK" : "Telegram FAIL");
      digitalWrite(LED_PIN, HIGH);
    }
  }
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }
}