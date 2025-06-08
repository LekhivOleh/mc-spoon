#include <Arduino.h>
#include <U8g2lib.h>

#define TRIG_PIN D6
#define ECHO_PIN D7
#define BUZZER_PIN D5

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

uint32_t previousTriggerTime = 0;
const uint32_t triggerInterval = 100;

uint32_t previousBeepToggleTime = 0;
uint32_t currentBeepInterval = 0;
bool buzzerState = LOW;

uint16_t lastDistance = 0;

const uint8_t zoneCount = 4;
const uint8_t rectWidth = 100;
const uint8_t displayHeight = 64;
const uint8_t rectHeight = displayHeight / zoneCount;
const uint16_t maxZoneDistance = 40;
const uint16_t zoneSize = maxZoneDistance / zoneCount;

uint8_t getFilledZones(uint16_t distance) {
    if (distance > maxZoneDistance) return 0;
    uint8_t zone = zoneCount - (distance / zoneSize);
    if (zone < 0) return 0;
    if (zone > zoneCount) return zoneCount;
    return zone;
}

uint32_t getZoneBeepInterval(uint16_t distance) {
    if (distance <= 10) return 50;
    if (distance <= 20) return 150;
    if (distance <= 30) return 300;
    if (distance <= 40) return 600;
    return 0;
}

void drawZones(uint8_t filled) {
    u8g2.clearBuffer();
    uint8_t startX = (128 - rectWidth) / 2;
    for (uint8_t i = 0; i < zoneCount; ++i) {
        uint8_t y = (zoneCount - 1 - i) * rectHeight;
        if (i < filled) {
            u8g2.drawBox(startX, y, rectWidth, rectHeight);
        } else {
            u8g2.drawFrame(startX, y, rectWidth, rectHeight);
        }
    }
    u8g2.sendBuffer();
}

void setup() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    u8g2.begin();
}

void loop() {
    uint32_t currentTime = millis();

    drawZones(getFilledZones(lastDistance));

    if (currentTime - previousTriggerTime >= triggerInterval) {
        previousTriggerTime = currentTime;

        digitalWrite(TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);

        uint32_t duration = pulseIn(ECHO_PIN, HIGH, 30000);

        if (duration > 0) {
            lastDistance = (uint16_t)((duration * 343UL) / 20000UL);
            currentBeepInterval = getZoneBeepInterval(lastDistance);
            if (currentBeepInterval == 0) {
                digitalWrite(BUZZER_PIN, LOW);
                buzzerState = LOW;
            }
        } else {
            currentBeepInterval = 0;
            digitalWrite(BUZZER_PIN, LOW);
            buzzerState = LOW;
        }
    }

    if (currentBeepInterval > 0 && lastDistance <= maxZoneDistance) {
        if (currentTime - previousBeepToggleTime >= currentBeepInterval) {
            previousBeepToggleTime = currentTime;
            buzzerState = !buzzerState;
            digitalWrite(BUZZER_PIN, buzzerState);
        }
    } else {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerState = LOW;
    }
}
