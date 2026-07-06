#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "../models/acState.h"
#include "config/settings.h"

using Frame = std::array<uint8_t, FRAME_SIZE>;
using RawTimings = std::array<uint16_t, RAW_TIMING_SIZE>;

class IR
{
    public:
        void init();
        void send(const AcState& acState);

        #if APP_DEBUG
            void debug_captureIR();
            void debug_printBinary(const char* txt, const uint8_t frame[FRAME_SIZE], uint8_t tail);
            void debug_printBinary(const char* txt, const RawTimings& timings);
            void debug_printChecksum(const char* txt, const uint8_t frame[FRAME_SIZE], uint8_t tail);
            void debug_printRawTimings(const RawTimings& timings);
        #endif

    private:
        static IR* _instance;
        static QueueHandle_t _sendQueue;
        static TaskHandle_t _sendTaskHandle;

        void setupSendQueue();
        void setupSendTask();
        static void sendTask(void* parameter);
        void sendNow(const AcState& acState);
        void sendRawTimings(RawTimings& rawTimings, const AcState& acState);
        Frame buildFrame(const AcState& acState);
        uint8_t frame2(const AcState& acState); 
        uint8_t frame3(const AcState& acState); 
        uint8_t frame4(const AcState& acState);
        uint8_t frame13(const AcState& acState, const Frame& frame);
        RawTimings GetRawTimings(Frame frame, uint8_t checksum, uint8_t tailBit);
        uint8_t calculateChecksum(Frame frame, uint8_t tailBit);
        uint16_t generateCrc(Frame frame, uint8_t tailBit);
        uint16_t generateCrcGeneric(
            Frame frame,
            uint8_t tail,
            uint8_t width,
            uint16_t poly,
            uint16_t init,
            uint16_t xorout,
            bool refin,
            bool refout,
            bool reverseBytes,
            bool invertBytes,
            bool includeTail
        );
        uint8_t reverse4(uint8_t x);
        uint8_t reverse8(uint8_t v);
        uint16_t reflectN(uint16_t x, uint8_t width); 
};
