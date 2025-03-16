#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

const char* SSID = "yomayo";
const char* PASSWORD = "14882284";

AsyncWebServer server(80);

typedef struct led_s {
    unsigned short pin;
    unsigned short status;
    led_s* nextLed;
} led_t;

typedef struct button_s {
    unsigned short pin;
    unsigned short status;
    unsigned short previousStatus;
} button_t;

led_t ledRed = {D7, LOW, nullptr};
led_t ledGreen = {D4, LOW, nullptr};
led_t ledBlue = {D5, LOW, nullptr};
button_t button = {D0, HIGH, HIGH};

led_t* currentLed = nullptr;
uint32_t lastStepTime = 0;
unsigned short stepTime = 1000;
bool isPaused = false;
uint32_t pauseEndTime = 0;
bool wifiConnected = false;

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

void pauseLEDs() {
    if (!isPaused) {
        isPaused = true;
        pauseEndTime = millis() + 2000;
    }
}

void lightLED() {
    uint32_t currentTime = millis();
    if (!isPaused && (currentTime - lastStepTime >= stepTime)) {
        lastStepTime = currentTime;
        digitalWrite(currentLed->pin, LOW);
        currentLed = currentLed->nextLed;
        digitalWrite(currentLed->pin, HIGH);
    }

    if (isPaused && currentTime >= pauseEndTime) {
        isPaused = false;
        lastStepTime = millis();
    }
}

void checkButton() {
    static bool buttonPressed = false;
    button.status = digitalRead(button.pin);

    if (button.status == HIGH && button.previousStatus == LOW && !buttonPressed) {
        buttonPressed = true;
        pauseLEDs();
    }

    if (button.status == LOW) {
        buttonPressed = false;
    }

    button.previousStatus = button.status;
}

void releaseEndpoint(AsyncWebServerRequest *request) {
    pauseLEDs();
    request->send(200, "text/html", "ok");
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP Web Server</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; }
        h1 { color: #000; }
        .button { background-color: red; color: white; padding: 12px 20px; border: none; border-radius: 4px; cursor: pointer; }
    </style>
</head>
<body>
    <h1>Release button</h1>
    <button class="button" onclick="fetch('/release').then(() => console.log('Paused')).catch(err => console.error(err));">Pause</button>
</body>
</html>
)rawliteral";

void wifiSetup() {
    WiFi.begin(SSID, PASSWORD);
}

void checkWiFi() {
    if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
        wifiConnected = true;
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send_P(200, "text/html", index_html);
        });

        server.on("/release", HTTP_GET, releaseEndpoint);
        server.begin();
    }
}

void setup() {
    Serial.begin(115200);
    pinSetup();
    ledSetup();
    wifiSetup();
}

void loop() {
    checkWiFi();
    checkButton();
    lightLED();
}
