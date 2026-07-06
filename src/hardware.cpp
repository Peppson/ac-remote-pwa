#include <WiFi.h>
#include "config/settings.h"
#include "hardware.h"

void Hardware::init()
{   
    #if APP_DEBUG
        initSerial();
    #endif

    pinMode(ERROR_LED_PIN, OUTPUT);
    digitalWrite(ERROR_LED_PIN, LOW);

    pinMode(OTA_LED_PIN, OUTPUT);
    digitalWrite(OTA_LED_PIN, LOW);

    pinMode(OTA_ENABLE_PIN, INPUT_PULLDOWN);

    connectWifi();
}

void Hardware::connectWifi() 
{   
    IPAddress local_IP(192, 168, 1, 210);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);

    if (!WiFi.config(local_IP, gateway, subnet)) 
        FatalError("Static IP config failed");

    WiFi.begin(SSID, PASSWORD);

    auto now = millis();
    while (WiFi.status() != WL_CONNECTED) 
    { 
        delay(250);
        if (millis() - now > 4000)
            Error("Failed to connect to WiFi");
    }
    WiFi.setSleep(false);

    log("Connected to WiFi: %s\n", WiFi.SSID().c_str());
    log("IP: %s\n", WiFi.localIP().toString().c_str());
    digitalWrite(ERROR_LED_PIN, LOW);
}

void Hardware::initSerial() 
{
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial && millis() < 3*1000) { } // give it 3s 
}

void Hardware::bootSuccessful()
{   
    log("\n\n----------- STARTED -----------\n\n");
    
    for (size_t i = 0; i < 3; i++)
    {
        digitalWrite(OTA_LED_PIN, HIGH);
        digitalWrite(ERROR_LED_PIN, HIGH);
        delay(75);
        digitalWrite(OTA_LED_PIN, LOW);
        digitalWrite(ERROR_LED_PIN, LOW);
        delay(75);
    }
}

void Hardware::Error(const char* message)
{
    log("ERROR: %s\n", message);
    digitalWrite(ERROR_LED_PIN, HIGH);
}

void Hardware::FatalError(const char* message)
{
    log("FATAL ERROR: %s\n", message);
    digitalWrite(ERROR_LED_PIN, HIGH);
    
    while (1) {};
}
