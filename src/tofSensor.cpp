#include "tofSensor.h"
#include "hardware.h"
#include "utility.h"

AcState* TofSensor::_acState = nullptr;


void TofSensor::init(AcState* acState)
{   
    _acState = acState;
    _currentPowerState = _acState->power;
    _lastPowerState = _acState->power;
    _lastLidState = _acState->lid;

    Wire.begin();
    Wire.setClock(WIRE_BAUD_RATE);
    
    vl53l1x.setTimeout(500);

    if (!vl53l1x.init()) 
        Hardware::FatalError("Failed to init tofSensor!");
 
    vl53l1x.setDistanceMode(VL53L1X::Medium);
    vl53l1x.setMeasurementTimingBudget(20000);
    vl53l1x.startContinuous(20);
}

void TofSensor::handle()
{
    if (!vl53l1x.dataReady()) return;
    
    auto distance_mm = vl53l1x.read(false);

    // bad reading
    if (distance_mm > 140) 
        distance_mm = POWER_ON_THRESHOLD_MM + 5;

    updatePowerState(distance_mm);
    updateLidState(distance_mm);

    #if APP_DEBUG && LOG_TOF_TO_PLOT
        log("> distance mm: %d,\n", distance_mm);
    #endif
}

void TofSensor::notifyAcStateChanged(const AcState& requestedAcState)
{   
    // power on 
    if (_lastPowerState != requestedAcState.power && requestedAcState.power)
    {
        _ignorePowerUntilMs = millis() + SYNC_POWER_ON_DELAY_MS;
        _ignoreLidUntilMs = millis() + SYNC_POWER_ON_DELAY_MS;
        log("Request Power: ON \n");
    }

    // power off
    else if (_lastPowerState != requestedAcState.power && !requestedAcState.power)
    {
        _ignorePowerUntilMs = millis() + SYNC_POWER_OFF_DELAY_MS;
        _ignoreLidUntilMs = millis() + SYNC_POWER_OFF_DELAY_MS;
        log("Request Power: OFF \n");
    }

    // lid toggle
    if (_lastLidState != requestedAcState.lid)
    {
        _ignoreLidUntilMs = millis() + SYNC_LID_DELAY_MS;
        log("Request Lid: Toggle\n");
    }

    _currentPowerState = requestedAcState.power;
    _lastPowerState = requestedAcState.power;
    _lastLidState = requestedAcState.lid;

    lidHistory.clear();
    _hasChanged = false;
}

bool TofSensor::hasChanged() 
{   
    if (_hasChanged) 
    {
        _hasChanged = false;
        return true;
    }

    return false;
}

void TofSensor::updatePowerState(uint16_t distance_mm) 
{   
    if (millis() < _ignorePowerUntilMs) return;

    const uint buffer_mm = 4;

    bool powerOn = (distance_mm > POWER_ON_THRESHOLD_MM + buffer_mm);
    bool powerOff = (distance_mm < POWER_ON_THRESHOLD_MM - buffer_mm);

    if (powerOn && !_lastPowerState) 
    {
        log("TOF Power: ON\n");
        _hasChanged = true;
        _acState->power = true;
        _currentPowerState = true;
        _lastPowerState = true;
        _ignoreLidUntilMs = millis() + SYNC_POWER_ON_DELAY_MS; // ignore lid changes for a while after power on
    } 
    else if (powerOff && _lastPowerState) 
    {
        log("TOF Power: OFF\n");
        _hasChanged = true;
        _acState->power = false;
        _currentPowerState = false;
        _lastPowerState = false;
    }
}

void TofSensor::updateLidState(uint16_t distance_mm) 
{   
    // OFF
    if (!_currentPowerState)
    {
        lidHistory.clear();
        return;
    }

    // Keep only a recent window of samples
    lidHistory.push_back({distance_mm, millis()});
    while (!lidHistory.empty() && millis() - lidHistory.front().timestamp > LID_MEASUREMENT_TIME_MS)
    {
        lidHistory.pop_front();
    }

    if (lidHistory.size() < 2) return;
    if (millis() < _ignoreLidUntilMs) return;

    uint16_t span = getDistanceSpan();
    setLidState(span);
}

uint16_t TofSensor::getDistanceSpan() 
{
    uint16_t minDistance = UINT16_MAX;
    uint16_t maxDistance = 0;

    for (const auto& sample : lidHistory)
    {
        if (sample.distance < minDistance) minDistance = sample.distance;
        if (sample.distance > maxDistance) maxDistance = sample.distance;
    }

    return maxDistance - minDistance;
}

void TofSensor::setLidState(uint16_t span) 
{
    bool lidState = (span > LID_SPAN_THRESHOLD_MM);

    if (lidState && !_lastLidState) 
    {
        log("TOF Lid: ON\n");
        _hasChanged = true;
        _acState->lid = true;
        _lastLidState = true;
    } 
    else if (!lidState && _lastLidState) 
    {
        log("TOF Lid: OFF\n");
        _hasChanged = true;
        _acState->lid = false;
        _lastLidState = false;
    }
}
