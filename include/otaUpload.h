#pragma once

class OtaUpload
{
    public:
        static void handle();
    
    private:
        static void setupOta();
        static void uploadLoop();
};
