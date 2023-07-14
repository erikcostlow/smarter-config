#ifndef SMARTER_CONFIG
#define SMARTER_CONFIG
#include <Arduino.h>

class SmarterConfig{
    public:

    SmarterConfig();
        /**
         * @brief Initializes BLE
         * 
         */
        static void start();
        /**
         * @brief 
         * 
         * @return true If not configured yet
         * @return false If wifi has been validated.
         */
        static bool isAwaitingConfig();
        /**
         * @brief Stops BLE
         * 
         */
        static void stop();
        static void extendPowerOff();
        static bool regenerateWiFiJson;

        static String ssid;
        static String password;
        static String name = "bletest";

    private:
        static bool awaitingConfig;
        static long shutdownMs;
        static bool updateWiFiStatus;
        static String wifiStatusStr;

        static void extendPowerOffMs(int ms);
        static void bleLoop();
        static String scanWiFi();
        static void onConnectCommand();
        static String wiFiStatus();
        static void startWiFiHooks();
        static void initializeFS();
        static bool validateWiFi();
        static void saveWiFi();
        static void deepSleep();
};

#endif