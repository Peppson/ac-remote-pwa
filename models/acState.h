#pragma once
#include <Arduino.h>

enum class Mode : uint8_t
{
    Cool,
    Dry,
    Fan,
    Auto
};

enum class FanSpeed : uint8_t
{
    Silent,
    Low,
    Medium,
    High
};

struct AcState {
    bool power;
    bool lid;
    bool turbo;
    bool sleep;
    uint8_t temp;
    uint8_t timer;
    Mode mode;
    FanSpeed fan;

    void setDefaults()
    {
        power = false;
        lid = false;
        turbo = false;
        sleep = false;
        temp = 21;
        timer = 0;
        mode = Mode::Cool;
        fan = FanSpeed::High;
    }
};
