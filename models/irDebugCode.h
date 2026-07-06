#pragma once
#include <Arduino.h>
#include "acState.h"

struct IrDebugCode
{
    const char* name;
    char debugKey;
    AcState acState;
};
