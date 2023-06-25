#include <esp_task_wdt.h>
#include "soc/rtc_wdt.h"
#include "SmarterConfig.h"
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <LITTLEFS.h>

namespace Configure
{
#define CONFIG_SERVICE "97F87C3E-F965-45EF-87DC-E1F0FF8C695A"
#define CHARACTERISTIC_COMMAND "FFFC70B6-4A21-4661-A2DB-9E0378902787"
#define CHARACTERISTIC_RESPONSE "74121C41-5433-428F-B6F0-9B8479251785"
#define CHARACTERISTIC_WIFI_STATUS "F802156A-FBE0-11ED-BE56-0242AC120002"

    class ServerCallbacks : public NimBLEServerCallbacks
    {
        void onConnect(NimBLEServer *pServer)
        {
            SmarterConfig::extendPowerOff();
            Serial.println("Client connected");
            Serial.println("Multi-connect support: start advertising");
            NimBLEDevice::startAdvertising();
        };
        /** Alternative onConnect() method to extract details of the connection.
         *  See: src/ble_gap.h for the details of the ble_gap_conn_desc struct.
         */
        void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc)
        {
            SmarterConfig::extendPowerOff();
            Serial.print("Client address: ");
            Serial.println(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
            /** We can use the connection handle here to ask for different connection parameters.
             *  Args: connection handle, min connection interval, max connection interval
             *  latency, supervision timeout.
             *  Units; Min/Max Intervals: 1.25 millisecond increments.
             *  Latency: number of intervals allowed to skip.
             *  Timeout: 10 millisecond increments, try for 5x interval time for best results.
             */
            pServer->updateConnParams(desc->conn_handle, 8, 8, 0, 60);
        };
        void onDisconnect(NimBLEServer *pServer)
        {
            SmarterConfig::extendPowerOff();
            Serial.println("Client disconnected - start advertising");
            NimBLEDevice::startAdvertising();
        };
        void onMTUChange(uint16_t MTU, ble_gap_conn_desc *desc)
        {
            SmarterConfig::extendPowerOff();
            Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, desc->conn_handle);
        };

        /********************* Security handled here **********************
        ****** Note: these are the same return values as defaults ********/
        uint32_t onPassKeyRequest()
        {
            Serial.println("Server Passkey Request");
            /** This should return a random 6 digit number for security
             *  or make your own static passkey as done here.
             */
            return 123456;
        };

        bool onConfirmPIN(uint32_t pass_key)
        {
            Serial.print("The passkey YES/NO number: ");
            Serial.println(pass_key);
            /** Return false if passkeys don't match. */
            return true;
        };

        void onAuthenticationComplete(ble_gap_conn_desc *desc)
        {
            /** Check that encryption was successful, if not we disconnect the client */
            if (!desc->sec_state.encrypted)
            {
                NimBLEDevice::getServer()->disconnect(desc->conn_handle);
                Serial.println("Encrypt connection failed - disconnecting client");
                return;
            }
            Serial.println("Starting BLE work!");
        };
    };

    String commandResponse = "";
    class CommandCallback : public NimBLECharacteristicCallbacks
    {
        void onWrite(NimBLECharacteristic *pCharacteristic)
        {
            SmarterConfig::extendPowerOff();
            Serial.print("Command Parameter received: ");
            Serial.println(pCharacteristic->getValue().c_str());

            std::string incomingJson = pCharacteristic->getValue();
            StaticJsonDocument<512> doc;
            DeserializationError err = deserializeJson(doc, incomingJson);
            
            if(err){
                StaticJsonDocument<512> errJson;
                auto errorMsg = String(err.c_str());
                Serial.print("Error reading command ");
                Serial.print(incomingJson.c_str());
                Serial.print(" --- ");
                Serial.println(errorMsg);
                errJson["error"] = String(errorMsg);
                serializeJson(errJson, commandResponse);
                return;
            }

            String cmd = doc["c"];
            if (cmd == "scan")
            {
                Serial.println("Scan command");
                SmarterConfig::regenerateWiFiJson = true;
            }
            else if (cmd == "connect")
            {
                Serial.println("Storing settings...");
                commandResponse = "{\"s\": \"connecting\"}";
                String ssidTmp = doc["ssid"];
                String passTmp = doc["password"];
                SmarterConfig::ssid = ssidTmp;
                SmarterConfig::password = passTmp;
            }
            else
            {
                commandResponse = "unknown command";
            }
            // BUG: sending the long string here, NimBLE loses control of BLE partways through.
            // Need to send in the main loop
            SmarterConfig::extendPowerOff();
        };

        void onRead(NimBLECharacteristic *pCharacteristic)
        {
            SmarterConfig::extendPowerOff();
            Serial.print(pCharacteristic->getUUID().toString().c_str());
            Serial.print(": onRead(), value: ");
            Serial.println(pCharacteristic->getValue().c_str());
        };

        void onNotify(NimBLECharacteristic *pCharacteristic)
        {
            Serial.print("BLE notifying: ");
            Serial.println(pCharacteristic->getValue().c_str());
        };

        /** The status returned in status is defined in NimBLECharacteristic.h.
         *  The value returned in code is the NimBLE host return code.
         */
        void onStatus(NimBLECharacteristic *pCharacteristic, Status status, int code)
        {
            SmarterConfig::extendPowerOff();
            String str = ("Notification/Indication status code: ");
            str += status;
            str += ", return code: ";
            str += code;
            str += ", ";
            str += NimBLEUtils::returnCodeToString(code);
            Serial.println(str);
        };

        void onSubscribe(NimBLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc, uint16_t subValue)
        {
            SmarterConfig::extendPowerOff();
            String str = "Client ID: ";
            str += desc->conn_handle;
            str += " Address: ";
            str += std::string(NimBLEAddress(desc->peer_ota_addr)).c_str();
            if (subValue == 0)
            {
                str += " Unsubscribed to ";
            }
            else if (subValue == 1)
            {
                str += " Subscribed to notfications for ";

                for (int i = 0; i < 10; i++)
                {
                    String s = "Count ";
                    s += i;
                    pCharacteristic->notify((std::string)s.c_str());

                    delay(2000);
                }
            }
            else if (subValue == 2)
            {
                str += " Subscribed to indications for ";
            }
            else if (subValue == 3)
            {
                str += " Subscribed to notifications and indications for ";
            }
            str += std::string(pCharacteristic->getUUID()).c_str();

            Serial.println(str);
        };
    };
    static CommandCallback commandCallbacks;

    static NimBLEServer *pServer;
    NimBLECharacteristic *characteristicResponse;
    NimBLECharacteristic *characteristicWifiStatus;
}

long SmarterConfig::shutdownMs = millis() + 5 * 60 * 1000;
bool SmarterConfig::awaitingConfig = true;
bool SmarterConfig::regenerateWiFiJson = false;
bool SmarterConfig::updateWiFiStatus = false;
String SmarterConfig::wifiStatusJson = "{\"status\": \"disconnected\"}";

void SmarterConfig::startWiFiHooks()
{
    WiFi.onEvent([](WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
                 {
                    updateWiFiStatus=true;
                    wifiStatusJson = "{\"s\": \"connected\"}"; },
                 ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent([](WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
                 {
                    updateWiFiStatus=true;
                    wifiStatusJson = "{\"s\": \"disconnected\"}"; },
                 ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    // WiFi.onEvent(wifiEvent, ARDUINO_EVENT_WIFI_AP_STACONNECTED);
    // WiFi.onEvent(wifiEvent, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
    WiFi.onEvent([](WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
                 {
                    wifiStatusJson = "{\"s\": \"scanned\"}";
                    updateWiFiStatus=true; },
                 ARDUINO_EVENT_WIFI_SCAN_DONE);
    // WiFi.onEvent(wifiReady, ARDUINO_EVENT_WIFI_READY);
    WiFi.onEvent([](WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
                 {
    String ip = WiFi.localIP().toString();
    String json = "{\"status\": \"connected\", \"ip\": ";
    json += ip;
    json += "\"}";
    wifiStatusJson = json;
    updateWiFiStatus = true; },
                 ARDUINO_EVENT_WIFI_STA_GOT_IP);
}

String SmarterConfig::wiFiStatus()
{
    int status = WiFi.status();
    switch (status)
    {
    case WL_IDLE_STATUS:
        return "WL_IDLE_STATUS";
    case WL_SCAN_COMPLETED:
        return "WL_SCAN_COMPLETED";
    case WL_NO_SSID_AVAIL:
        return "WL_NO_SSID_AVAIL";
    case WL_CONNECT_FAILED:
        return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST:
        return "WL_CONNECTION_LOST";
    case WL_CONNECTED:
        return "WL_CONNECTED";
    case WL_DISCONNECTED:
        return "WL_DISCONNECTED";
    }
    return "unknown_wifi";
}

void SmarterConfig::initializeFS()
{
    Serial.println("Setting up filesystem");
    LittleFS.begin(true);
    LittleFS.format();
    Serial.println("Done setting up filesystem");
}

bool SmarterConfig::validateWiFi()
{
    File config = LittleFS.open("/wifi.json");
    if (!config)
    {
        // No config
        return false;
    }
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, config);
    if (error)
    {
        Serial.print("Error reading wifi config: ");
        Serial.println(error.c_str());
        return false;
    }

    String ssid = doc["ssid"];
    String passwordStr = doc["password"];
    const char *password;
    if (passwordStr.length() == 0)
    {
        password = NULL;
    }
    else
    {
        password = passwordStr.c_str();
    }

    WiFi.begin(ssid.c_str(), password);

    for (long stopAfter = millis() + 60 * 1000;
             millis() < stopAfter && WiFi.status() != WL_CONNECTED && WiFi.status() != WL_CONNECT_FAILED;)
        {
            //ledGreen(millis() % 1000 > 500);
            yield();
            rtc_wdt_feed();
        }
    
    if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("Connected to wifi");
            Serial.print(WiFi.localIP());
            Serial.print(" - RSSI ");
            Serial.println(WiFi.RSSI());
            //ledGreen(true);
            WiFi.setAutoReconnect(true);
            delay(50);
            awaitingConfig=false;
            return true;
        }
        else
        {
            Serial.println("Unable to connect to wifi");
        }

    return false;
}

void SmarterConfig::start()
{
    initializeFS();

    NimBLEDevice::init("bletest");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
    NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
    Configure::pServer = NimBLEDevice::createServer();
    Configure::pServer->setCallbacks(new Configure::ServerCallbacks());

    if (validateWiFi())
    {
        Serial.println("WiFi validated");
        awaitingConfig = false;
        return;
    }

    shutdownMs = millis() + 5 * 60 * 1000;

    NimBLEService *costlowService = Configure::pServer->createService(CONFIG_SERVICE);
    NimBLECharacteristic *characteristicCommand = costlowService->createCharacteristic(CHARACTERISTIC_COMMAND,
                                                                                       NIMBLE_PROPERTY::WRITE);
    characteristicCommand->setCallbacks(&Configure::commandCallbacks);
    Configure::characteristicResponse = costlowService->createCharacteristic(CHARACTERISTIC_RESPONSE,
                                                                             NIMBLE_PROPERTY::NOTIFY);
    Configure::characteristicWifiStatus = costlowService->createCharacteristic(CHARACTERISTIC_WIFI_STATUS,
                                                                               NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    Configure::characteristicWifiStatus->setValue(wiFiStatus());

    costlowService->start();
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(costlowService->getUUID());

    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    startWiFiHooks();

    Serial.println("SmarterConfig started.");
}

void SmarterConfig::extendPowerOff()
{
    extendPowerOffMs(5 * 60 * 1000);
}

void SmarterConfig::extendPowerOffMs(int ms)
{
    shutdownMs = millis() + ms;
}

bool SmarterConfig::isAwaitingConfig()
{
    bleLoop();
    return awaitingConfig;
}

void sendCommandResponseString()
{
    int len = Configure::commandResponse.length();
    int offset = 0;
    const int ble_length_mtu_forgiving = 20;
    while (offset < len)
    {
        int endGuess = offset + ble_length_mtu_forgiving;
        int end = endGuess >= len ? len : endGuess;
        Serial.print(millis());
        Serial.print(" - ");
        Serial.print("Sending BLE chunk at ");
        Serial.print(end);
        Serial.print(": ");
        String chunk = Configure::commandResponse.substring(offset, end);
        Serial.println(chunk);
        // c->setValue(chunk);
        Configure::characteristicResponse->notify(chunk);
        offset = end;
        delay(10);
    }
    Serial.print(millis());
    Serial.println("Sending completion notification");
    String completion = "";
    Configure::characteristicResponse->notify(completion);
    Configure::commandResponse = "";
};

String SmarterConfig::ssid = "";
String SmarterConfig::password = "";

void SmarterConfig::saveWiFi(){
    awaitingConfig=false;
    File out = LittleFS.open("/wifi.json", "w");
    StaticJsonDocument<512> doc;
    doc["ssid"] = ssid;
    doc["password"] = password;
    serializeJson(doc, out);
    out.close();
    Serial.println("WiFi saved.");
}

void SmarterConfig::deepSleep(){
    Serial.println("Deep sleep");
    awaitingConfig = false;
}

void SmarterConfig::bleLoop()
{
    esp_task_wdt_reset();
    yield();
    if (millis() > shutdownMs)
    {
        deepSleep();
    }
    else if (regenerateWiFiJson)
    {
        String json = scanWiFi();
        Configure::commandResponse = json;
        regenerateWiFiJson = false;
    }
    else if (updateWiFiStatus)
    {
        Serial.println("Updating wifi status");
        Serial.println(wifiStatusJson);
        Configure::characteristicWifiStatus->setValue(wifiStatusJson);
        String notification = "";
        Configure::characteristicWifiStatus->notify(notification);
        updateWiFiStatus = false;
    }
    else if (!ssid.isEmpty())
    {
        Serial.println("Connecting to wifi");
        WiFi.begin(ssid, password);
        long started = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - started < 10000)
        {
            yield();
            rtc_wdt_feed();
        }
        
        Serial.print("Wifi is ");
        Serial.println(wiFiStatus());
        
        if(WiFi.isConnected()){
            String notification = "{\"s\": \"configured\"}";
            Configure::characteristicWifiStatus->setValue(notification);
            Configure::characteristicWifiStatus->notify(notification);
            Configure::commandResponse = notification;
            
            sendCommandResponseString();
            delay(100);
            saveWiFi();
        }else{
            Serial.println("WiFi did not succeed");
        }
        ssid = "";
        password = "";
    }

    if (Configure::commandResponse.length() > 0)
    {
        sendCommandResponseString();
        extendPowerOff();
    }
}

void SmarterConfig::stop()
{
    awaitingConfig = false;
    Configure::pServer->stopAdvertising();
    WiFi.onEvent([](WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {

    });
    NimBLEDevice::deinit(true);
}

String SmarterConfig::scanWiFi()
{
    Serial.println("Scanning nearby APs");
    int networkCount = WiFi.scanNetworks();
    Serial.print(networkCount);
    Serial.println(" networks found:");
    StaticJsonDocument<2048> doc;
    doc["generated"] = millis();
    JsonArray data = doc.createNestedArray("nearby");
    for (int i = 0; i < networkCount; ++i)
    {
        JsonObject el = data.createNestedObject();
        // Print SSID and RSSI for each network found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        el["ssid"] = WiFi.SSID(i);
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        el["rssi"] = WiFi.RSSI(i);
        Serial.print(")");
        String authType;
        switch (WiFi.encryptionType(i))
        {
        case WIFI_AUTH_OPEN:
            authType = "open";
            break;
        case WIFI_AUTH_WEP:
            authType = "wep";
            break;
        case WIFI_AUTH_WPA2_PSK:
        case WIFI_AUTH_WPA2_ENTERPRISE:
        case WIFI_AUTH_WPA_WPA2_PSK:
            authType = "wpa2";
            break;
        case WIFI_AUTH_WPA_PSK:
            authType = "wpa1";
            break;
        default:
            authType = "unknown";
        }
        el["encryption"] = authType;
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
    }
    String wiFiJson = "";
    serializeJson(doc, wiFiJson);
    Serial.println("Done scanning networks");
    return wiFiJson;
}
