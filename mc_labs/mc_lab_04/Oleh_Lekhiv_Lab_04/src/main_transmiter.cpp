#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <IRrecv.h>
#include <IRutils.h>
#include "env.h"

const char* ssid = SSID;
const char* password = PASSWORD;
const char* mqtt_server = MQTT_SERVER_IP;
const char* topic = "ir/data";

const uint16_t RECV_PIN = D2;
const int LED_PIN = LED_BUILTIN;

WiFiClient espClient;
PubSubClient client(espClient);
IRrecv irrecv(RECV_PIN);
decode_results results;

void setup_wifi() {
    pinMode(LED_PIN, OUTPUT);
    bool ledState = false;
    delay(10);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);
        delay(300);
    }
    digitalWrite(LED_PIN, HIGH);
}

void reconnect() {
    while (!client.connected()) {
        client.connect("IRTransmitter");
        delay(500);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    irrecv.enableIRIn();
}

unsigned long lastLedToggle = 0;
bool ledState = false;

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    if (WiFi.status() == WL_CONNECTED) {
        if (millis() - lastLedToggle > 20000) {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? LOW : HIGH);
            lastLedToggle = millis();
        }
    }

    if (irrecv.decode(&results)) {
        String irCode = resultToHexidecimal(&results);
        client.publish(topic, irCode.c_str());
        Serial.print("Published IR code: ");
        Serial.println(irCode);
        irrecv.resume();
    }
}