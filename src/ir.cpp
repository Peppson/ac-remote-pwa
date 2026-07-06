#include <IRremote.hpp>  
#include <utility.h>
#include "hardware.h"
#include "ir.h"

IR* IR::_instance = nullptr;
QueueHandle_t IR::_sendQueue = nullptr;
TaskHandle_t IR::_sendTaskHandle = nullptr;

void IR::init() 
{   
    _instance = this;
    IrSender.begin(IR_OUT_PIN);
    
    #if RECEIVE_IR_ENABLED
        IrReceiver.begin(IR_IN_PIN);
    #endif

    setupSendQueue();
    setupSendTask();
}

void IR::setupSendQueue()
{
    if (_sendQueue == nullptr) _sendQueue = xQueueCreate(1, sizeof(AcState));
    if (_sendQueue == nullptr) Hardware::FatalError("Failed to create IR send queue");
}

void IR::setupSendTask()
{
    if (_sendTaskHandle != nullptr) return;
    
    BaseType_t ok = xTaskCreatePinnedToCore(
        sendTask,
        "ir_send_task",
        4096,
        nullptr,
        1,
        &_sendTaskHandle,
        1
    );

    if (ok != pdPASS) Hardware::FatalError("Failed to create IR send task");
}

void IR::sendTask(void* parameter)
{   
    (void)parameter; // compiler bitching
    AcState pending{};

    while (true)
    {
        if (xQueueReceive(_sendQueue, &pending, portMAX_DELAY) == pdTRUE && _instance != nullptr)
            _instance->sendNow(pending);
    }
}

void IR::send(const AcState& acState)
{
    if (_sendQueue == nullptr) 
    {
        sendNow(acState);
        return;
    }

    xQueueOverwrite(_sendQueue, &acState);
}

void IR::sendNow(const AcState& acState)
{   
    #if RECEIVE_IR_ENABLED
        IrReceiver.stop();
    #endif

    auto frame = buildFrame(acState);
    
    // Send with both checksum values (tailbit 0 and 1)
    for (size_t tailBit = 0; tailBit < 2; tailBit++)
    {
        auto checksum = calculateChecksum(frame, tailBit);
        auto rawTimings = GetRawTimings(frame, checksum, tailBit);
        sendRawTimings(rawTimings, acState);
    }

    #if RECEIVE_IR_ENABLED
        IrReceiver.start();
    #endif
}

Frame IR::buildFrame(const AcState& acState)
{
    Frame frame{};

    // headers
    frame[0] = 0xC1;
    frame[1] = 0x60;

    // AcState
    frame[2] =  frame2(acState);                    // fanspeed + lid 
    frame[3] =  frame3(acState);                    // primary mode + temp 
    frame[4] =  frame4(acState);                    // timer + auto Mode
    frame[5] =  acState.turbo ? 0x09 : 0x00;        // turbo
    frame[6] =  0x80;                               // -
    frame[7] =  0x80;                               // -
    frame[8] =  0x00;                               // -
    frame[9] =  0x00;                               // -
    frame[10] = 0x00;                               // -
    frame[11] = 0x00;                               // -
    frame[12] = 0x00;                               // -

    // checksum
    frame[13] = frame13(acState, frame);            

    return frame;
}

uint8_t IR::frame2(const AcState& acState) 
{   
    if (acState.power) return 0x60; 
    if (acState.sleep) return 0xD0;

    uint8_t frame = 0x00;

    // fan speed high on turbo
    if (acState.turbo) frame |= 0x80;
    else switch (acState.fan)
    {
        case FanSpeed::High:   frame |= 0x80; break;
        case FanSpeed::Medium: frame |= 0x40; break;
        case FanSpeed::Low:    frame |= 0xC0; break;
        case FanSpeed::Silent: frame |= 0x00; break;
    }

    if (acState.lid) frame |= 0x0D;    

    return frame;
}

uint8_t IR::frame3(const AcState& acState) 
{   
    uint8_t modeNibble = 0x00;

    // Jet, sleep or mode
    if (acState.turbo) modeNibble = 0x40;
    else if (acState.sleep) modeNibble = 0x40; 
    else switch (acState.mode)
    {
        case Mode::Cool: modeNibble = 0x40; break;
        case Mode::Dry:  modeNibble = 0xC0; break;
        case Mode::Fan:  modeNibble = 0x20; break;
        case Mode::Auto: modeNibble = 0x80; break;
    }

    // Set mode as timer 
    if (acState.timer > 0) modeNibble = 0x50;

    // Temp nibble. Offset from 18, then bit-reversed
    uint8_t temp = acState.temp;
    if (temp < 18) temp = 18;
    if (temp > 32) temp = 32;

    // Jet mode
    if (acState.turbo) temp = 18; 

    uint8_t offset = (uint8_t)(temp - 18);
    uint8_t tempNibble = reverse4(offset);

    return (uint8_t)(modeNibble | tempNibble);
}

uint8_t IR::frame4(const AcState& acState) 
{   
    uint8_t hours = acState.timer;
    if (hours > 10) hours = 10;

    uint8_t lower3 = 0x00;
    if (acState.mode == Mode::Auto) lower3 |= 0x01;

    if (hours == 0) return lower3;

    // reverse the byte / 2
    // keep upper 5 timer bits + 3 lower 3 automode bits
    uint8_t timerEncoded = reverse8(hours) >> 1;
    uint8_t frame = timerEncoded & 0xF8;
    frame |= lower3 & 0x07;
 
    return frame;
}

uint8_t IR::frame13(const AcState& acState, const Frame& frame) 
{
    uint8_t upper = 
        ((frame[2] >> 5) & 0x07) ^ 
        ((frame[3] >> 5) & 0x07) ^ 
        ((frame[4] >> 5) & 0x07);
    
    return (uint8_t)(upper << 4);
}

RawTimings IR::GetRawTimings(Frame frame, uint8_t checksum, uint8_t tailBit)
{
    RawTimings timings{};
    int index = 0;

    // insert checksum
    frame[FRAME_SIZE - 1] = checksum;

    // header
    timings[index++] = HEADER_MARK;
    timings[index++] = HEADER_SPACE;

    // data
    for (int i = 0; i < FRAME_SIZE; i++)
    {
        for (int b = 7; b >= 0; b--)
        {
            timings[index++] = BIT_MARK;
            timings[index++] = ((frame[i] >> b) & 1) ? BIT_SPACE1 : BIT_SPACE0;
        }
    }

    timings[index++] = BIT_MARK;
    timings[index++] = tailBit ? BIT_SPACE1 : BIT_SPACE0;
    timings[index++] = BIT_SPACE0;
    timings[99] = BIT_GAP;

    return timings;
}

void IR::sendRawTimings(RawTimings& rawTimings, const AcState& acState) 
{   
    //debug_printBinary("Binary: ", rawTimings);
    //debug_printRawTimings(rawTimings);

    #if !SEND_IR_ENABLED
        return;
    #endif

    IrSender.sendRaw(rawTimings.data(), RAW_TIMING_SIZE, FREQUENCY);
    delay(100);
}

uint8_t IR::calculateChecksum(Frame frame, uint8_t tailBit) 
{   
    uint16_t crc = generateCrc(frame, tailBit);
    uint8_t got = frame[FRAME_SIZE - 1];

    return (uint8_t)((got & 0xF0) | (crc & 0x0F));
}

uint16_t IR::generateCrc(Frame frame, uint8_t tailBit) 
{
    constexpr uint8_t width = 8;
    constexpr uint16_t poly = 0x7F;
    constexpr uint16_t init = 0x00;
    constexpr uint16_t xorout = 0x00;
    constexpr uint16_t mask = (uint16_t)((1u << width) - 1u);

    uint16_t crc = (uint16_t)(init & mask);

    for (int i = 0; i < 13; i++) 
    {
        uint8_t b = frame[i];
        for (int k = 0; k < 8; k++) 
        {
            uint8_t bit = (uint8_t)((b >> k) & 1u);
            uint8_t fb = (uint8_t)((crc & 1u) ^ bit);
            crc >>= 1;
            if (fb) crc ^= poly;
            crc &= mask;
        }
    }

    uint8_t bit = (uint8_t)(tailBit & 1u);
    uint8_t fb = (uint8_t)((crc & 1u) ^ bit);

    crc >>= 1;
    if (fb) crc ^= poly;
    crc &= mask;
    crc ^= xorout;
    crc &= mask;

    return crc;
}

uint8_t IR::reverse4(uint8_t x)
{
    x &= 0x0F;
    return ((x & 0x01) << 3) | ((x & 0x02) << 1) | ((x & 0x04) >> 1) | ((x & 0x08) >> 3);
}

uint8_t IR::reverse8(uint8_t v) 
{
    v = (uint8_t)(((v & 0xF0) >> 4) | ((v & 0x0F) << 4));
    v = (uint8_t)(((v & 0xCC) >> 2) | ((v & 0x33) << 2));
    v = (uint8_t)(((v & 0xAA) >> 1) | ((v & 0x55) << 1));

    return v;
}

uint16_t IR::reflectN(uint16_t x, uint8_t width) 
{
    uint16_t r = 0;
    for (uint8_t i = 0; i < width; i++) 
    {
        if (x & (1u << i)) r |= (1u << (width - 1 - i));
    }

    return r;
}

uint16_t IR::generateCrcGeneric(
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
) {
    const uint16_t mask = (uint16_t)((1u << width) - 1u);
    uint16_t crc = (uint16_t)(init & mask);

    for (int i = 0; i < 13; i++) 
    {
        uint8_t b = frame[i];
        if (reverseBytes) b = reverse8(b);
        if (invertBytes) b = (uint8_t)~b;

        if (refin) 
        {
            for (int k = 0; k < 8; k++) 
            {
                uint8_t bit = (uint8_t)((b >> k) & 1u);
                uint8_t fb = (uint8_t)((crc & 1u) ^ bit);
                crc >>= 1;
                if (fb) crc ^= poly;
                crc &= mask;
            }
        } 
        else 
        {
            for (int k = 0; k < 8; k++) 
            {
                uint8_t bit = (uint8_t)((b >> (7 - k)) & 1u);
                uint8_t fb = (uint8_t)(((crc >> (width - 1)) & 1u) ^ bit);
                crc = (uint16_t)((crc << 1) & mask);
                if (fb) crc ^= poly;
                crc &= mask;
            }
        }
    }

    if (includeTail) 
    {
        uint8_t bit = (uint8_t)(tail & 1u);
        if (refin) 
        {
            uint8_t fb = (uint8_t)((crc & 1u) ^ bit);
            crc >>= 1;
            if (fb) crc ^= poly;
            crc &= mask;
        } 
        else 
        {
            uint8_t fb = (uint8_t)(((crc >> (width - 1)) & 1u) ^ bit);
            crc = (uint16_t)((crc << 1) & mask);
            if (fb) crc ^= poly;
            crc &= mask;
        }
    }

    if (refout) crc = reflectN(crc, width);
    crc ^= xorout;
    crc &= mask;

    return crc;
}

#if APP_DEBUG

void IR::debug_captureIR() 
{   
    static int counter = 0;

    if (IrReceiver.decode()) 
    {
        log("\n-------- %d --------\n\n", counter++);
        //IrReceiver.printIRResultAsCArray(&Serial);

        uint8_t rawTimings[229];
        IrReceiver.compensateAndStoreIRResultInArray(rawTimings);

        std::vector<uint8_t> bits;
        bits.reserve(150);

        for (size_t i = 2; i + 1 < 229; i += 2) 
        {
            uint16_t space = rawTimings[i + 1];
            uint8_t bit = (space > 20) ? 1 : 0;
            bits.push_back(bit);
        }

        log("Bit count: %zu\n", bits.size());

        for (size_t i = 0; i < bits.size(); i++)
        {
            log("%u", bits[i]);
            if (i % 8 == 7) log(" ");
        }
        log("\n");        

        IrReceiver.resume();
    }
}

void IR::debug_printChecksum(const char* txt, const uint8_t frame[FRAME_SIZE], uint8_t tail)
{
    log("%s", txt);
    for (int i = 13; i < 14; i++) 
    {
        for (int b = 7; b >= 0; b--) log("%u", (frame[i] >> b) & 1u);
        log(" ");
    }

    log("%u\n", tail);
}

void IR::debug_printBinary(const char* txt, const uint8_t frame[FRAME_SIZE], uint8_t tail)
{
    log("%s", txt);
    for (int i = 0; i < 14; i++) 
    {
        for (int b = 7; b >= 0; b--) log("%u", (frame[i] >> b) & 1u);
        log(" ");
    }

    log("%u\n", tail);
}

void IR::debug_printBinary(const char* txt, const RawTimings& timings)
{
    std::vector<uint8_t> bits;
    bits.reserve(150);

    for (size_t i = 2; i + 1 < 229; i += 2) 
    {
        uint16_t space = timings[i + 1];
        uint8_t bit = (space > 1000) ? 1 : 0;
        bits.push_back(bit);
    }

    log("%s", txt);

    for (size_t i = 0; i < bits.size(); i++)
    {
        log("%u", bits[i]);
        if (i % 8 == 7) log(" ");
    }
    log("\n");
}

void IR::debug_printRawTimings(const RawTimings& timings)
{
    for (int i = 0; i < 229; i++) 
    {   
        if (i % 2 == 0) log("%u,", timings[i]);
        else log("%u, ", timings[i]);
    }
    log("\n");
}

#endif
