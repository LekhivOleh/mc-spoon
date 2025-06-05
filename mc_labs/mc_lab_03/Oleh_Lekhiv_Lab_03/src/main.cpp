#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <time.h>
#include "env.h"

// Replace with your values
const char* ssid = SSID;
const char* password = PASSWORD;
const char* nasa_api_key = NASA_API_KEY;
const char* opencage_api_key = OPENCAGE_API_KEY;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

const int buttonPin = D3;
int screenMode = 0;
unsigned long lastButtonPress = 0;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 5000;
const unsigned long debounceDelay = 300;

String currentLat = "N/A";
String currentLon = "N/A";
String currentCountry = "N/A";
String apodDate = "N/A";
String apodTitle = "N/A";
String apodAuthor = "Unknown";

int scrollOffset = 0;
unsigned long lastScrollTime = 0;
const unsigned int scrollSpeed = 200;
const int scrollStep = 2;
unsigned long scrollPauseStart = 0;
const unsigned int scrollPauseTime = 1200;

long contrast = 127;

void updateDisplay() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    if (screenMode == 0) {
        u8g2.drawStr(0, 10, "ISS Tracker:");
        u8g2.drawStr(0, 25, ("Lat: " + currentLat).c_str());
        u8g2.drawStr(0, 40, ("Lon: " + currentLon).c_str());
        String countryText = "Over: " + currentCountry;
        int textWidth = u8g2.getStrWidth(countryText.c_str());
        int displayWidth = u8g2.getDisplayWidth();
        int maxScrollOffset = textWidth - displayWidth + 1;
        if (textWidth > displayWidth) {
            u8g2.drawStr(0 - scrollOffset, 55, countryText.c_str());
            if (scrollOffset < maxScrollOffset) {
                if (millis() - lastScrollTime > scrollSpeed) {
                    int nextScrollOffset = scrollOffset + scrollStep;
                    if (nextScrollOffset >= maxScrollOffset) {
                        scrollOffset = maxScrollOffset;
                        scrollPauseStart = millis();
                    } else {
                        scrollOffset = nextScrollOffset;
                    }
                    lastScrollTime = millis();
                }
            } else {
                if (millis() - scrollPauseStart > scrollPauseTime) {
                    scrollOffset = 0;
                    lastScrollTime = millis();
                    scrollPauseStart = 0;
                }
            }
        } else {
            u8g2.drawStr(0, 55, countryText.c_str());
            scrollOffset = 0;
            scrollPauseStart = 0;
        }
    } else if (screenMode == 1) {
        u8g2.drawStr(2, 12, ("Date: " + apodDate).c_str());
        u8g2.drawStr(2, 30, "Title:");
        String titleText = apodTitle;
        int titleTextWidth = u8g2.getStrWidth(titleText.c_str());
        int titleDisplayWidth = u8g2.getDisplayWidth() - 2;
        int titleMaxScrollOffset = titleTextWidth - titleDisplayWidth + 1;
        if (titleTextWidth > titleDisplayWidth) {
            u8g2.drawStr(2 - scrollOffset, 40, titleText.c_str());
            if (scrollOffset < titleMaxScrollOffset) {
                if (millis() - lastScrollTime > scrollSpeed) {
                    int nextScrollOffset = scrollOffset + scrollStep;
                    if (nextScrollOffset >= titleMaxScrollOffset) {
                        scrollOffset = titleMaxScrollOffset;
                        scrollPauseStart = millis();
                    } else {
                        scrollOffset = nextScrollOffset;
                    }
                    lastScrollTime = millis();
                }
            } else {
                if (millis() - scrollPauseStart > scrollPauseTime) {
                    scrollOffset = 0;
                    lastScrollTime = millis();
                    scrollPauseStart = 0;
                }
            }
        } else {
            u8g2.drawStr(2, 40, titleText.c_str());
            scrollOffset = 0;
            scrollPauseStart = 0;
        }
        String authorText = "By: " + apodAuthor;
        int authorTextWidth = u8g2.getStrWidth(authorText.c_str());
        int authorDisplayWidth = u8g2.getDisplayWidth() - 2;
        int authorMaxScrollOffset = authorTextWidth - authorDisplayWidth + 1;
        if (authorTextWidth > authorDisplayWidth) {
            u8g2.drawStr(2 - scrollOffset, 56, authorText.c_str());
        } else {
            u8g2.drawStr(2, 56, authorText.c_str());
        }
    } else if (screenMode == 2) {
        u8g2.drawStr(10, 15, "Brightness:");
        u8g2.drawFrame(10, 30, 108, 15);
        int fillWidth = map(contrast, 0, 255, 0, 106);
        u8g2.drawBox(11, 31, fillWidth, 13);
    }
    u8g2.sendBuffer();
}

void showISSLocation() {
    if (WiFi.status() != WL_CONNECTED) {
        updateDisplay();
        return;
    }

    WiFiClient client;
    HTTPClient http;
    String tempLat = currentLat;
    String tempLon = currentLon;
    String tempCountry = currentCountry;

    http.begin(client, "http://api.open-notify.org/iss-now.json");
    int httpCode = http.GET();
    if (httpCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(192);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            tempLat = doc["iss_position"]["latitude"].as<String>();
            tempLon = doc["iss_position"]["longitude"].as<String>();
        }
    }
    http.end();

    WiFiClientSecure geoClient;
    geoClient.setInsecure();
    HTTPClient geoHttp;

    String geoUrl = String("https://api.opencagedata.com/geocode/v1/json?q=") + tempLat + "+" + tempLon + "&key=" + opencage_api_key;
    geoHttp.begin(geoClient, geoUrl);
    geoHttp.setUserAgent("ESP8266");
    int geoCode = geoHttp.GET();
    if (geoCode == 200) {
        String geoPayload = geoHttp.getString();
        StaticJsonDocument<256> filter;
        filter["results"][0]["components"]["country"] = true;
        filter["results"][0]["components"]["continent"] = true;
        filter["results"][0]["components"]["body_of_water"] = true;

        DynamicJsonDocument geoDoc(1024);
        DeserializationError geoErr = deserializeJson(geoDoc, geoPayload, DeserializationOption::Filter(filter));
        if (!geoErr) {
            JsonArray results = geoDoc["results"];
            if (results.size() > 0) {
                JsonObject components = results[0]["components"];
                if (components.containsKey("country")) {
                    tempCountry = components["country"].as<String>();
                } else if (components.containsKey("body_of_water")) {
                    tempCountry = components["body_of_water"].as<String>();
                } else if (components.containsKey("continent")) {
                    tempCountry = components["continent"].as<String>();
                } else {
                    tempCountry = "Unknown Area";
                }
            } else {
                tempCountry = "No Geodata";
            }
        }
    }
    geoHttp.end();

    currentLat = tempLat;
    currentLon = tempLon;
    currentCountry = tempCountry;
    updateDisplay();
}

void showAPODInfo() {
    if (WiFi.status() != WL_CONNECTED) {
        updateDisplay();
        return;
    }

    struct tm timeinfo{};
    if (!getLocalTime(&timeinfo)) {
        apodDate = "Time Error";
        apodTitle = "N/A";
        apodAuthor = "N/A";
        updateDisplay();
        return;
    }

    char dateStr[11];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url = "https://api.nasa.gov/planetary/apod?api_key=" + String(nasa_api_key) + "&date=" + String(dateStr);
    http.begin(client, url);
    int code = http.GET();

    String tempApodDate = apodDate;
    String tempApodTitle = apodTitle;
    String tempApodAuthor = apodAuthor;

    if (code == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            tempApodDate = doc["date"].as<String>();
            tempApodTitle = doc["title"].as<String>();
            if (doc.containsKey("copyright")) {
                tempApodAuthor = doc["copyright"].as<String>();
                tempApodAuthor.trim();
            } else {
                tempApodAuthor = "Unknown";
            }
        }
    }
    http.end();

    apodDate = tempApodDate;
    apodTitle = tempApodTitle;
    apodAuthor = tempApodAuthor;
    updateDisplay();
}

void performUpdate() {
    if (WiFi.status() == WL_CONNECTED) {
        if (screenMode == 0) showISSLocation();
        else if (screenMode == 1) showAPODInfo();
    } else {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(0, 20, "Connecting WiFi...");
        u8g2.sendBuffer();
    }
    lastUpdate = millis();
}

void setup() {
    Serial.begin(115200);
    pinMode(buttonPin, INPUT_PULLUP);
    u8g2.begin();
    u8g2.setContrast(contrast);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 20, "Connecting WiFi...");
    u8g2.sendBuffer();

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    u8g2.clearBuffer();
    u8g2.drawStr(0, 20, "WiFi connected");
    u8g2.sendBuffer();
    delay(1000);

    performUpdate();
}

void loop() {
    static bool buttonPressed = false;

    if (digitalRead(buttonPin) == LOW) {
        if (!buttonPressed && millis() - lastButtonPress > debounceDelay) {
            buttonPressed = true;
            lastButtonPress = millis();
            screenMode = (screenMode + 1) % 3;
            scrollOffset = 0;
            lastScrollTime = millis();
            scrollPauseStart = 0;
            if (screenMode != 2) performUpdate();
            else updateDisplay();
        }
    } else {
        buttonPressed = false;
    }

    if (screenMode == 2) {
        long raw = analogRead(A0);
        long newContrast = constrain(map(raw, 0, 1023, 0, 255), 0, 255);
        if (newContrast != contrast) {
            contrast = newContrast;
            u8g2.setContrast(contrast);
            updateDisplay();
        }
    }

    if (millis() - lastUpdate > updateInterval && screenMode != 2) {
        performUpdate();
    }

    if (millis() - lastScrollTime > scrollSpeed && screenMode != 2) {
        updateDisplay();
    }
}
