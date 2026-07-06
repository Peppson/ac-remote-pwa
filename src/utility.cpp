#include "utility.h" 
#include "config/irDebugCodes.h"

void IRAM_ATTR Utility::debug_benchmark(const char* name, uint32_t threshold_us) 
{
    static bool begin = true;
    static uint32_t counter = 0;
    static uint32_t startTime_us = 0;

    // Start timer
	if (begin) 
    {
        begin = false;
		startTime_us = micros();
        return;
	}
    
    // Stop timer
    uint32_t endTime_us = micros() - startTime_us;

    // filter below x us
    if (threshold_us > 0 && endTime_us <= threshold_us)
    {
        begin = true;
        startTime_us = 0;
        return;
    }

    log("#%u: ", counter++);

    // name (if any)
    if (name != nullptr) log("%s: ", name);

    // microseconds
    if (endTime_us < 1000U) 
    {
        log("%luus\n", (unsigned long)endTime_us);
    } 
    // milliseconds
    else if (endTime_us < 1000 * 1000U)  
    {
        float end_ms = (float)endTime_us / 1000;
        log("%.3fms\n", end_ms);
    }
    else 
    {
        log("%lums\n", (unsigned long)(endTime_us / 1000U));
    }

    // Reset
    begin = true;
    startTime_us = 0;	
}

void Utility::debug_printKeyboardInfo() 
{
    log("\n");
    for (size_t i = 0; i < _numOfDebugCodes; i++)
    {
        log("%c = %s\n", irDebugCodes[i].debugKey, irDebugCodes[i].name);
    } 
    log("\n"); 
}

void Utility::debug_keyboardAction(IR& ir)
{    
    if (Serial.available() == 0) return;
    
    const char key = Serial.read();   
    bool sent = false;

    for (size_t i = 0; i < _numOfDebugCodes; i++) 
    {
        if (key == irDebugCodes[i].debugKey) 
        {   
            ir.send(irDebugCodes[i].acState);
            sent = true;
            break;
        }
    }

    if (!sent) debug_printKeyboardInfo();

    Serial.flush();
}

void Utility::debug_printAcState(const AcState& acState)
{
    static bool hasPrevAcState = false;
    static AcState prevAcState{};
    static int printCounter = 0;

    const char* modeTxt = "Cool";
    switch (acState.mode)
    {
        case Mode::Cool:  modeTxt = "Cool";  break;
        case Mode::Dry:   modeTxt = "Dry";   break;
        case Mode::Fan:   modeTxt = "Fan";   break;
        case Mode::Auto:  modeTxt = "Auto";  break;
    }

    const char* fanTxt = "Silent";
    switch (acState.fan)
    {
        case FanSpeed::Silent: fanTxt = "Silent"; break;
        case FanSpeed::Low:    fanTxt = "Low";    break;
        case FanSpeed::Medium: fanTxt = "Medium"; break;
        case FanSpeed::High:   fanTxt = "High";   break;
    }

    const char* markPower = (hasPrevAcState && prevAcState.power != acState.power) ? "  <<<<<<<<<<<<" : "";
    const char* markLid = (hasPrevAcState && prevAcState.lid != acState.lid) ? "  <<<<<<<<<<<<" : "";
    const char* markTurbo = (hasPrevAcState && prevAcState.turbo != acState.turbo) ? "  <<<<<<<<<<<<" : "";
    const char* markSleep = (hasPrevAcState && prevAcState.sleep != acState.sleep) ? "  <<<<<<<<<<<<" : "";
    const char* markTemp = (hasPrevAcState && prevAcState.temp != acState.temp) ? "  <<<<<<<<<<<<" : "";
    const char* markTimer = (hasPrevAcState && prevAcState.timer != acState.timer) ? "  <<<<<<<<<<<<" : "";
    const char* markMode = (hasPrevAcState && prevAcState.mode != acState.mode) ? "  <<<<<<<<<<<<" : "";
    const char* markFan = (hasPrevAcState && prevAcState.fan != acState.fan) ? "  <<<<<<<<<<<<" : "";

    log(
        "\nAcState [%d] {\n  power: %u%s\n  lid: %u%s\n  turbo: %u%s\n  sleep: %u%s\n  temp: %u%s\n  timer: %u%s\n  mode: %s%s\n  fan: %s%s\n};\n",
        printCounter++,
        acState.power ? 1u : 0u,
        markPower,
        acState.lid ? 1u : 0u,
        markLid,
        acState.turbo ? 1u : 0u,
        markTurbo,
        acState.sleep ? 1u : 0u,
        markSleep,
        acState.temp,
        markTemp,
        acState.timer,
        markTimer,
        modeTxt,
        markMode,
        fanTxt,
        markFan
    );

    prevAcState = acState;
    hasPrevAcState = true;
}

void Utility::debug_scanI2CBus()
{
    uint8_t foundCount = 0;

    log("I2C scan: ");
    for (uint8_t address = 1; address < 127; ++address)
    {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
            log("0x%02X ", address);
            ++foundCount;
        }
    }

    if (foundCount == 0)
        log("no devices found");

    log("\n");
}
