#pragma once
#include <Wire.h>
#include <VL53L1X.h>
#include <deque>
#include "../models/acState.h"
#include "config/settings.h"

class TofSensor
{
    public:
        void init(AcState* acState);
        void handle();
        void notifyAcStateChanged(const AcState& requestedAcState);
        bool hasChanged();
    
    private:
        VL53L1X vl53l1x;
        
        struct LidHistory {
            uint16_t distance;
            uint32_t timestamp;
        };

        void updatePowerState(uint16_t distance_mm);
        void updateLidState(uint16_t distance_mm);
        uint16_t getDistanceSpan();
        void setLidState(uint16_t span);
        
        static AcState* _acState;
        std::deque<LidHistory> lidHistory;

        bool _lastPowerState = false;
        bool _lastLidState = false;
        bool _hasChanged = false;
        bool _currentPowerState = false;
        uint32_t _ignorePowerUntilMs = millis() + 3000;
        uint32_t _ignoreLidUntilMs = millis() + 3000;
};
