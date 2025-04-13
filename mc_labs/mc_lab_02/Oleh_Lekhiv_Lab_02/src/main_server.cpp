#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

const char* SSID = "esp1318";
const char* PASSWORD = "14882284";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

enum class Color { RED, GREEN, BLUE };

typedef struct led_s {
    unsigned short pin;
    unsigned short status;
    Color color;
    led_s* nextLed;
} led_t;

typedef struct button_s {
    unsigned short pin;
    unsigned short status;
    unsigned short previousStatus;
    void (*callback)();
} button_t;

led_t* currentLed = nullptr;
uint32_t lastStepTime = 0;
unsigned short stepTime = 1000;
bool isPaused = false;
uint32_t pauseEndTime = 0;
bool wifiConnected = false;
bool buttonPressed = false;

led_t ledRed = {D7, LOW, Color::RED, nullptr};
led_t ledGreen = {D4, LOW, Color::GREEN, nullptr};
led_t ledBlue = {D5, LOW, Color::BLUE, nullptr};

void pauseLEDs() {
    if (!isPaused) {
        isPaused = true;
        pauseEndTime = millis() + 2000;
    }
}

button_t button = {D0, HIGH, HIGH, pauseLEDs};

void ledSetup() {
    ledRed.nextLed = &ledGreen;
    ledGreen.nextLed = &ledBlue;
    ledBlue.nextLed = &ledRed;
    currentLed = &ledBlue;
}

void pinSetup() {
    pinMode(ledRed.pin, OUTPUT);
    pinMode(ledGreen.pin, OUTPUT);
    pinMode(ledBlue.pin, OUTPUT);
    pinMode(button.pin, INPUT_PULLUP);
}



void SendLEDColorToWeb() {
    if (currentLed == nullptr) return;

    switch (currentLed->color) {
        case Color::RED:
            ws.textAll("red");
            break;
        case Color::GREEN:
            ws.textAll("green");
            break;
        case Color::BLUE:
            ws.textAll("blue");
            break;
    }
}

void lightLED() {
    uint32_t currentTime = millis();
    if (!isPaused && (currentTime - lastStepTime >= stepTime)) {
        lastStepTime = currentTime;
        digitalWrite(currentLed->pin, LOW);
        currentLed = currentLed->nextLed;
        digitalWrite(currentLed->pin, HIGH);
        SendLEDColorToWeb();
    }

    if (isPaused && currentTime >= pauseEndTime) {
        isPaused = false;
        lastStepTime = millis();
    }
}

void checkButton() {
    button.status = digitalRead(button.pin);

    if (button.status == HIGH && button.previousStatus == LOW && !buttonPressed) {
        buttonPressed = true;
        if (button.callback != nullptr) {
            button.callback();
        }
    }

    if (button.status == LOW) {
        buttonPressed = false;
    }

    button.previousStatus = button.status;
}


void checkSerial() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input == "pause") {
            pauseLEDs();
        }
    }
}

void releaseEndpoint(AsyncWebServerRequest *request) {
    pauseLEDs();
    request->send(200, "text/html", "ok");
}

void releaseAnotherEndpoint(AsyncWebServerRequest *request) {
    Serial.println("pause");
    request->send(200, "text/html", "ok");
}

void wifiSetup() {
    WiFi.softAP(SSID, PASSWORD);
    IPAddress IP = WiFi.softAPIP();
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.on("/release", HTTP_GET, releaseEndpoint);
    server.on("/releaseAnother", HTTP_GET, releaseAnotherEndpoint);
    server.addHandler(&ws);
    server.begin();
}


void setup() {
    Serial.begin(115200);
    delay(9000);
    if (!LittleFS.begin()) {
        return;
    }
    pinSetup();
    ledSetup();
    wifiSetup();
}

void loop() {
    checkButton();
    checkSerial();
    lightLED();
    ws.cleanupClients();
}