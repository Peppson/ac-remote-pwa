#pragma once
#include <Wire.h>
#include "ir.h"
#include "config/irDebugCodes.h"
#include "config/settings.h"

class Utility
{
    public:
        static void debug_printKeyboardInfo();
        static void debug_printAcState(const AcState& acState);
        static void debug_keyboardAction(IR& ir);
        static void debug_scanI2CBus();
        static void IRAM_ATTR debug_benchmark(const char* name = nullptr, uint32_t threshold_us = 0);

    private:
        static constexpr size_t _numOfDebugCodes = sizeof(irDebugCodes) / sizeof(irDebugCodes[0]);
};

#if APP_DEBUG

#define SCOPED_TIMER_IMPL(line) ScopedTimer scoped_timer_##line(__func__)
#define SCOPED_TIMER() SCOPED_TIMER_IMPL(__LINE__)

class ScopedTimer
{
    public:
        explicit ScopedTimer(const char* name) : _name(name)
        {
            Utility::debug_benchmark();
        }

        ~ScopedTimer()
        {
            Utility::debug_benchmark(_name);
        }

    private:
        const char* _name;
};

#endif
