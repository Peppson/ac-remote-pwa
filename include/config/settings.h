#pragma once
#include <Arduino.h>
#include <stdio.h>
#include "secrets.h"

// dev
#define APP_DEBUG 0
#define LOG_TOF_TO_PLOT 0
#define LOG_INCOMMING_PAYLOAD 0
#define KEYBOARD_DEBUG 0

// configs
#define CACHE_WEBPAGE 0
#define SEND_IR_ENABLED 1
#define RECEIVE_IR_ENABLED 0

// web
constexpr const char* SSID = SECRET_SSID;
constexpr const char* PASSWORD = SECRET_PASSWORD;

// IR timings
constexpr int HEADER_MARK = 9030;                       
constexpr int HEADER_SPACE = 4520;
constexpr int BIT_GAP = 7970;
constexpr int BIT_MARK = 630;
constexpr int BIT_SPACE0 = 480;
constexpr int BIT_SPACE1 = 1670;
constexpr int FRAME_SIZE = 14;
constexpr int FREQUENCY = 38; // kHz
constexpr int RAW_TIMING_SIZE = 229;
constexpr int RAW_TIMING_SIZE_NO_CHECKSUM = 229 - 18;

// tof sensor
constexpr uint POWER_ON_THRESHOLD_MM = 35;
constexpr uint LID_MEASUREMENT_TIME_MS = 4000;
constexpr uint LID_SPAN_THRESHOLD_MM = 20;
constexpr uint SYNC_POWER_ON_DELAY_MS = 16000;
constexpr uint SYNC_POWER_OFF_DELAY_MS = 40000;
constexpr uint SYNC_LID_DELAY_MS = 3500;

// I/O
constexpr uint SERIAL_BAUD_RATE = 115200;
constexpr uint WIRE_BAUD_RATE = 400000;

// pins
constexpr uint8_t IR_IN_PIN = 13;
constexpr uint8_t IR_OUT_PIN = 12; // just for prototype
constexpr uint8_t TOF_SDA_PIN = 21;
constexpr uint8_t TOF_SCL_PIN = 22;
constexpr uint8_t OTA_ENABLE_PIN = 26;
constexpr uint8_t OTA_LED_PIN = 14;
constexpr uint8_t ERROR_LED_PIN = 27;

// log
#if APP_DEBUG
    #define log(...) printf(__VA_ARGS__)
    #define STOP log("\n#####  Stop right there criminal scum!  ##### \n"); while (1) { delay(1); }
#else
    #define log(...)
    #define STOP while (1) { delay(1); }
#endif
