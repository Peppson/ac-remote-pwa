#pragma once
#include "../models/acState.h"
#include "../models/irDebugCode.h"
#include "settings.h"

constexpr IrDebugCode irDebugCodes[] =
{
    {
        "Power toggle",
        '1',
        {
            .power = true,
            .lid = false,
            .turbo = false,
            .sleep = false,
            .temp = 26,
            .timer = 0,
            .mode = Mode::Cool,
            .fan = FanSpeed::Low
        }
    },
    {
        "Jet on",
        '2',
        {
            .power = false,
            .lid = false,
            .turbo = true,
            .sleep = false,
            .temp = 26,
            .timer = 0,
            .mode = Mode::Cool,
            .fan = FanSpeed::Low
        }
    },
    {
        "Jet off",
        '3',
        {
            .power = false,
            .lid = false,
            .turbo = false,
            .sleep = false,
            .temp = 26,
            .timer = 0,
            .mode = Mode::Cool,
            .fan = FanSpeed::Low
        }
    },
    {
        "Mode cool",
        '4',
        {
            .power = false,
            .lid = false,
            .turbo = false,
            .sleep = false,
            .temp = 26,
            .timer = 0,
            .mode = Mode::Cool,
            .fan = FanSpeed::Low
        }
    },
    {
        "Toggle lid",
        '5',
        {
            .power = false,
            .lid = true,
            .turbo = false,
            .sleep = false,
            .temp = 26,
            .timer = 0,
            .mode = Mode::Cool,
            .fan = FanSpeed::Low
        }
    },
    {
        "Timer 10h",
        '6',
        {
            .power = false,
            .lid = false,
            .turbo = false,
            .sleep = false,
            .temp = 26,
            .timer = 10,
            .mode = Mode::Cool,
            .fan = FanSpeed::Low
        }
    },
    {
        "Mode fan",
        '7',
        {
            .power = false,
            .lid = false,
            .turbo = false,
            .sleep = false,
            .temp = 32,
            .timer = 0,
            .mode = Mode::Fan,
            .fan = FanSpeed::Low
        }
    },
};
