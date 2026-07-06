#include "config/settings.h"
#include "OtaUpload.h"
#include "apiServer.h"
#include "tofSensor.h"
#include "hardware.h"
#include "utility.h"
#include "ir.h"

static AcState acState;             // current state of the AC
static Hardware hardware;           // hardware pins + wifi
static OtaUpload otaUpload;         // over-the-air program upload
static APiServer server;            // web and websocket server
static TofSensor tof;               // time-of-flight sensor
static IR ir;                       // infrared RX and TX

void setup()
{
    acState.setDefaults();
    
    hardware.init();   
    ir.init();
    tof.init(&acState);
    server.init(&acState, &tof, &ir);

    hardware.bootSuccessful();
}

void loop()
{   
    otaUpload.handle();
    server.handle();
    tof.handle();

    if (tof.hasChanged())
    {
        server.NotifyAcStateChanged(acState);
    }

    #if RECEIVE_IR_ENABLED
        ir.debug_captureIR();
    #endif

    #if KEYBOARD_DEBUG
        hardware.debug_keyboardAction(ir);
    #endif
}
