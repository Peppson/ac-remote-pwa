#pragma once

class Hardware
{
    public:
        static void init();
        static void bootSuccessful();
        static void Error(const char* message = "");
        static void FatalError(const char* message = "");

    private:
        static void initSerial();
        static void connectWifi();
};
