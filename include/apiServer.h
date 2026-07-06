#pragma once
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "../models/acState.h"
#include "tofSensor.h"
#include "ir.h"

class APiServer
{
    public:
        void init(AcState* acState, TofSensor* tof, IR* ir);
        void handle();
        void NotifyAcStateChanged(AcState& acState);
        
    private:
        WebServer webServer{80};
        WebSocketsServer webSocket{81};

        static APiServer* _instance;
        static AcState* _acState;
        static AcState _lastIrAcState;
        static TofSensor* _tof;
        static IR* _ir;
        static bool _irACStateInit;

        void setupEndpoints();
        void sendCacheControlHeader();
        AcState buildIrAcState(const AcState& requestedAcState);
        static void wsHandleEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
        void wsEventConnected(uint8_t num);
        void wsEventInData(uint8_t num, uint8_t* payload, size_t length);
        void wsNotifyAcStateChanged(AcState& acState);
        void wsSendError(uint8_t num, const String& error = "");
        bool validateAndGetAcState(const String& data, String& outError, AcState& outAcState);
        String stringFromPayload(uint8_t* payload, size_t length);
        String AcStateToJson(const AcState& acState);
        static bool tryGetJsonField(const String& data, const char* key, String& outValue);
        static bool parseBool(const String& value, bool& outValue);
        static bool parseUInt8(const String& value, uint8_t& outValue);
        static Mode parseMode(const String& s);
        static FanSpeed parseFan(const String& s);
        static const char* modeToString(Mode mode);
        static const char* fanToString(FanSpeed fan);
};
