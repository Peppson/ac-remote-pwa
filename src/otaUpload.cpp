#include <ArduinoOTA.h>
#include "config/settings.h"
#include "OTAUpload.h"
#include "hardware.h"
#include "config/secrets.h"

void OtaUpload::handle()
{
    if (digitalRead(OTA_ENABLE_PIN) == HIGH) 
    {
        log("OTA upload mode enabled\n");
        setupOta();
        uploadLoop();
    }
}

void OtaUpload::setupOta()
{   
    ArduinoOTA.setHostname("ir-ac-remote");
    ArduinoOTA.setPassword(SECRET_OTA_PASSWORD);

    ArduinoOTA.onError([](ota_error_t error)
    {
        uploadLoop(); // retry on error
    });

    digitalWrite(OTA_LED_PIN, HIGH);
    delay(500);
    ArduinoOTA.begin();
}

void OtaUpload::uploadLoop()
{   
    // reboots after successful upload
    while (true) 
    {
        ArduinoOTA.handle();
        
        if (digitalRead(OTA_ENABLE_PIN) == HIGH) 
        {   
            delay(100);
            ESP.restart();
        }
    }
}
