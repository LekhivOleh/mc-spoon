#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <U8g2lib.h>
#include <map>
#include <functional>
#include "env.h"

const char* ssid = SSID;
const char* password = PASSWORD;
const char* mqtt_server = MQTT_SERVER_IP;
const char* topic = "ir/data";

const int LED_PIN = LED_BUILTIN;

WiFiClient espClient;
PubSubClient client(espClient);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

std::map<String, String> irCodeToCommand = {
  {"0xC", "POWER"},
  {"0x1000C", "POWER_OFF"},
  {"0x10", "VOL_UP"},
  {"0x10010", "VOL_UP"},
  {"0x11", "VOL_DOWN"},
  {"0x10011", "VOL_DOWN"}
};

bool isOn = false;
int volumeLevel = 5;
const int maxVolume = 10;
unsigned long lastCommandTime = 0;
const unsigned long debounceDelay = 700;

void drawVolumeBar() {
  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, 128, 64);
  int barWidth = map(volumeLevel, 0, maxVolume, 0, 124);
  u8g2.drawBox(2, 30, barWidth, 10);
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10, 20, "Volume");
  u8g2.sendBuffer();
}

void handlePower() {
  isOn = true;
  u8g2.setPowerSave(false);
  drawVolumeBar();
}

void handlePowerOff() {
  isOn = false;
  u8g2.setPowerSave(true);
}

void handleVolUp() {
  if (!isOn) return;
  if (volumeLevel < maxVolume) volumeLevel++;
  drawVolumeBar();
}

void handleVolDown() {
  if (!isOn) return;
  if (volumeLevel > 0) volumeLevel--;
  drawVolumeBar();
}

std::map<String, std::function<void()>> commandHandlers = {
  {"POWER", handlePower},
  {"POWER_OFF", handlePowerOff},
  {"VOL_UP", handleVolUp},
  {"VOL_DOWN", handleVolDown}
};

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  unsigned long now = millis();
  if (now - lastCommandTime < debounceDelay) return;
  lastCommandTime = now;

  String rawMsg = "";
  for (unsigned int i = 0; i < length; i++) {
    rawMsg += (char)payload[i];
  }
  
  String cmd = irCodeToCommand.count(rawMsg) ? irCodeToCommand[rawMsg] : rawMsg;

  if (commandHandlers.count(cmd)) {
    commandHandlers[cmd]();
  }
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "IRReceiver-" + String(ESP.getChipId());
    if (client.connect(clientId.c_str())) {
      client.subscribe(topic);
    } else {
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  u8g2.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
