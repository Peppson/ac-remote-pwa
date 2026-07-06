#include "apiServer.h"
#include "config/secrets.h"
#include "config/settings.h"
#include "hardware.h"
#include "embeddedWeb/embedded.h"
#include "utility.h"

AcState* APiServer::_acState = nullptr;
AcState APiServer::_lastIrAcState{};
APiServer* APiServer::_instance = nullptr;
TofSensor* APiServer::_tof = nullptr;
IR* APiServer::_ir = nullptr;
bool APiServer::_irACStateInit = false;


void APiServer::init(AcState* acState, TofSensor* tof, IR* ir)
{   
    _acState = acState;
    _tof = tof;
    _ir = ir;
    _instance = this;
    
    setupEndpoints();
    
    webSocket.begin();
    webSocket.onEvent(wsHandleEvent);
    webServer.begin();
}

void APiServer::setupEndpoints()
{
    webServer.on("/", HTTP_GET, [this]() 
    {
        sendCacheControlHeader();
        webServer.send(200, "text/html; charset=utf-8", EMBEDDED_WEBPAGE);
        log("200: /index.html\n");
    });

    webServer.on("/apple-touch-icon.png", HTTP_GET, [this]() 
    {
        sendCacheControlHeader();
        webServer.send_P(200, "image/png", reinterpret_cast<PGM_P>(EMBEDDED_DARK_512X512_PNG), EMBEDDED_DARK_512X512_PNG_LEN);
        log("200: /apple-touch-icon.png\n");
    });

    webServer.on("/icon-192x192.png", HTTP_GET, [this]() 
    {
        sendCacheControlHeader();
        webServer.send_P(200, "image/png", reinterpret_cast<PGM_P>(EMBEDDED_DARK_512X512_PNG), EMBEDDED_DARK_512X512_PNG_LEN);
        log("200: /icon-192x192.png\n");
    });

    webServer.onNotFound([this]() 
    {
        webServer.send(404, "text/plain; charset=utf-8", "404 Not Found");
        log("404 Not Found: %s\n", webServer.uri().c_str());
    });
} 

void APiServer::handle()
{
    webSocket.loop();
    webServer.handleClient();
}

void APiServer::wsHandleEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length)
{   
    if (_instance == nullptr) return;

    if (type == WStype_CONNECTED) _instance->wsEventConnected(num);
    else if (type == WStype_TEXT) _instance->wsEventInData(num, payload, length);
}

void APiServer::wsEventConnected(uint8_t num)
{   
    auto json = AcStateToJson(*_acState);
    webSocket.sendTXT(num, json.c_str(), json.length());
}

void APiServer::wsEventInData(uint8_t num, uint8_t* payload, size_t length)
{   
    String error;

    if (length == 0 || length > 512 || payload == nullptr) 
    {
        error = "Invalid payload: length = " + String(length);
        wsSendError(num, error);
        return;
    }

    auto request = stringFromPayload(payload, length);

    if (request[0] != '{')
    {
        error = "Invalid payload: not JSON";
        wsSendError(num, error);
        return;
    } 

    AcState requestAcState;
    if (!validateAndGetAcState(request, error, requestAcState)) 
    {
        log("\nWebsocket error:\n%s \nerror: %s\n", request.c_str(), error.c_str());
        wsSendError(num, error);
        return;
    }

    requestAcState.timer = 0; // not used
    *_acState = requestAcState;

    #if LOG_INCOMMING_PAYLOAD
        Utility::debug_printAcState(*_acState);
    #endif

    // Update states and send
    this->wsNotifyAcStateChanged(*_acState);    // send state to all clients via WS
    _tof->notifyAcStateChanged(*_acState);      // paus tof sensor if needed
    _ir->send(buildIrAcState(*_acState));       // send IR command to AC
}

void APiServer::wsNotifyAcStateChanged(AcState& acState)
{
    auto json = AcStateToJson(acState);
    webSocket.broadcastTXT(json.c_str(), json.length());
}

void APiServer::NotifyAcStateChanged(AcState& acState)
{
    auto json = AcStateToJson(acState);
    webSocket.broadcastTXT(json.c_str(), json.length());
    _lastIrAcState = acState;
}

void APiServer::wsSendError(uint8_t num, const String& error)
{   
    log("Websocket error: %s\n", error.c_str());

    String safeError = error;
    safeError.replace("\\", "\\\\");
    safeError.replace("\"", "\\\"");
    String errorJson = "{\"error\":\"" + safeError + "\"}";

    webSocket.sendTXT(num, errorJson.c_str(), errorJson.length());
}

void APiServer::sendCacheControlHeader()
{   
    #if CACHE_WEBPAGE
        webServer.sendHeader("Cache-Control", "public, max-age=604800"); // 1 week
    #else
        webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        webServer.sendHeader("Pragma", "no-cache");
        webServer.sendHeader("Expires", "0");
    #endif
}

AcState APiServer::buildIrAcState(const AcState& requestedAcState)
{
    AcState irAcState = requestedAcState;

    if (!_irACStateInit)
    {
        _lastIrAcState = requestedAcState;
        _irACStateInit = true;
        irAcState.power = false;
        irAcState.lid = false;
        return irAcState;
    }

    irAcState.power = (requestedAcState.power != _lastIrAcState.power);
    irAcState.lid = (requestedAcState.lid != _lastIrAcState.lid);

    _lastIrAcState = requestedAcState;
    return irAcState;
}

String APiServer::stringFromPayload(uint8_t* payload, size_t length)
{    
    String result;
    result.reserve(length);

    for (size_t i = 0; i < length; i++) 
        result += (char)payload[i];

    return result; 
}

String APiServer::AcStateToJson(const AcState& acState)
{
    String json = "{";
    json += "\"power\":" + String(acState.power ? 1 : 0);
    json += ",\"lid\":" + String(acState.lid ? 1 : 0);
    json += ",\"turbo\":" + String(acState.turbo ? 1 : 0);
    json += ",\"sleep\":" + String(acState.sleep ? 1 : 0);
    json += ",\"temp\":" + String(acState.temp);
    json += ",\"timer\":" + String(acState.timer);
    json += ",\"mode\":\"" + String(modeToString(acState.mode)) + "\"";
    json += ",\"fan\":\"" + String(fanToString(acState.fan)) + "\"";
    json += "}";
    return json;
}

bool APiServer::tryGetJsonField(const String& data, const char* key, String& outValue)
{
    // Build search pattern: "key":
    String pattern = "\"" + String(key) + "\":";
    int patternPos = data.indexOf(pattern);
    if (patternPos < 0) return false;

    int valueStart = patternPos + pattern.length();
    if (valueStart >= data.length()) return false;

    // Skip leading whitespace
    while (valueStart < data.length() && (data[valueStart] == ' ' || data[valueStart] == '\t')) {
        valueStart++;
    }
    if (valueStart >= data.length()) return false;

    // If quoted string, read until closing quote
    if (data[valueStart] == '"') {
        int endQuote = data.indexOf('"', valueStart + 1);
        if (endQuote < 0) return false;
        outValue = data.substring(valueStart + 1, endQuote);
        return true;
    }

    // Otherwise (number/bool), read until comma or closing brace
    int valueEnd = valueStart;
    while (valueEnd < data.length() && data[valueEnd] != ',' && data[valueEnd] != '}') {
        valueEnd++;
    }

    if (valueEnd <= valueStart) return false;

    outValue = data.substring(valueStart, valueEnd);
    outValue.trim();
    
    return outValue.length() > 0;
}

bool APiServer::validateAndGetAcState(const String& data, String& outError, AcState& outAcState) // sloppy
{
    String value;
    AcState result;

    if (!tryGetJsonField(data, "power", value) || !parseBool(value, result.power)) 
    {
        outError = "Invalid or missing field: power";
        return false;
    }

    if (!tryGetJsonField(data, "lid", value) || !parseBool(value, result.lid)) 
    {
        outError = "Invalid or missing field: lid";
        return false;
    }

    if (!tryGetJsonField(data, "turbo", value) || !parseBool(value, result.turbo)) 
    {
        outError = "Invalid or missing field: turbo";
        return false;
    }

    if (!tryGetJsonField(data, "sleep", value) || !parseBool(value, result.sleep)) 
    {
        outError = "Invalid or missing field: sleep";
        return false;
    }

    if (!tryGetJsonField(data, "temp", value) || !parseUInt8(value, result.temp)) 
    {
        outError = "Invalid or missing field: temp";
        return false;
    }

    if (result.temp < 18 || result.temp > 32) 
    {
        outError = "temp out of range (18-32)";
        return false;
    }

    if (!tryGetJsonField(data, "timer", value) || !parseUInt8(value, result.timer)) 
    {
        outError = "Invalid or missing field: timer";
        return false;
    }

    if (result.timer > 10) 
    {
        outError = "timer out of range (0-10)";
        return false;
    }

    if (!tryGetJsonField(data, "mode", value)) 
    {
        outError = "Invalid or missing field: mode";
        return false;
    }

    if (value != "Cool" && value != "Dry" && value != "Fan" && value != "Auto") 
    {
        outError = "Invalid mode";
        return false;
    }

    result.mode = parseMode(value);

    if (!tryGetJsonField(data, "fan", value)) 
    {
        outError = "Invalid or missing field: fan";
        return false;
    }

    if (value != "High" && value != "Medium" && value != "Low" && value != "Silent") 
    {
        outError = "Invalid fan";
        return false;
    }

    result.fan = parseFan(value);

    outAcState = result;
    return true;
}

bool APiServer::parseBool(const String& value, bool& outValue)
{
    if (value == "1" || value == "true") 
    {
        outValue = true;
        return true;
    }

    if (value == "0" || value == "false") 
    {
        outValue = false;
        return true;
    }

    return false;
}

bool APiServer::parseUInt8(const String& value, uint8_t& outValue)
{
    if (value.length() == 0) return false;

    for (size_t i = 0; i < value.length(); i++) 
    {
        if (value[i] < '0' || value[i] > '9') return false;
    }

    long parsed = value.toInt();
    if (parsed < 0 || parsed > 255) return false;

    outValue = (uint8_t) parsed;
    return true;
}

Mode APiServer::parseMode(const String& s)
{
    if (s == "Dry")  return Mode::Dry;
    if (s == "Fan")  return Mode::Fan;
    if (s == "Auto") return Mode::Auto;
    return Mode::Cool;
}

FanSpeed APiServer::parseFan(const String& s)
{
    if (s == "High")   return FanSpeed::High;
    if (s == "Medium") return FanSpeed::Medium;
    if (s == "Low")    return FanSpeed::Low;
    return FanSpeed::Silent;
}

const char* APiServer::modeToString(Mode mode)
{
    switch (mode)
    {
        case Mode::Cool: return "Cool";
        case Mode::Dry:  return "Dry";
        case Mode::Fan:  return "Fan";
        case Mode::Auto: return "Auto";
    }
    return "Cool";
}

const char* APiServer::fanToString(FanSpeed fan)
{
    switch (fan)
    {
        case FanSpeed::High:   return "High";
        case FanSpeed::Medium: return "Medium";
        case FanSpeed::Low:    return "Low";
        case FanSpeed::Silent: return "Silent";
    }
    return "Medium";
}
