# SmarterConfig

A utility for ESP32 device provisioning to WiFi.

A challenge of ESP32 devices is getting them their first WiFi credentials and reconfiguring them in the event of a failure or device move.

SmarterConfig is BLE-based configuration for communication between an app and a single device.

Specifically the reason for SmarterConfig is several factors:

1. The ability to configure a single device, in the event that multiple ESP32s are nearby.
1. A status coming back to an app, showing success or failure of connecting to the wifi network.
1. Storing credentials on the device via LittleFS rather than direct EEPROM.

## Comparison to other approaches
1. [SmartConfig](https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiSmartConfig/WiFiSmartConfig.ino) - unlike SmartConfig, the network BSSID is not necessary. The ESP32 controls its own WiFi so the app can be connected to 5ghz or a different network.
1. [WiFiProv](https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFiProv/examples/WiFiProv/WiFiProv.ino) - WiFiProv did not work.
1. Custom device web-app - end user configuration is a bit harder, as they need to connect to your custom SSID and use the app.

## Requirements
SmarterConfig requires several libraries. A sample platformio.ini
```ini
[env]
monitor_speed = 115200

[env:esp32dev]
platform = espressif32
board = esp32dev
board_build.filesystem = littlefs
framework =
	arduino
#	espidf
platform_packages = framework-arduinoespressif32 @ https://github.com/espressif/arduino-esp32.git#2.0.9
build_type = debug
monitor_filters = esp32_exception_decoder
lib_deps = 
	bblanchon/ArduinoJson@^6.19.2
	djgrrr/Int64String@^1.1.1
	h2zero/NimBLE-Arduino@^1.4.1
board_build.partitions = customPartition.csv
build_flags =
	-DARDUINO_ARCH_ESP32
```

Custom Partitions, because BLE libraries are large.
```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x1C0000,
app1,     app,  ota_1,   0x1D0000,0x1C0000,
spiffs,   data, spiffs,  0x390000,0x70000,
```

## Examples

See examples/provision.cpp for a sample device app.

See examples/flutter for a sample Flutter connecting app.

## BLE Service and Commands

- Configuration Service 97F87C3E-F965-45EF-87DC-E1F0FF8C695A
  - Command to write FFFC70B6-4A21-4661-A2DB-9E0378902787
  - Command's Response to Notify 74121C41-5433-428F-B6F0-9B8479251785
  - WiFi Status to Read. Notify will be blank, informing the app to read new value. F802156A-FBE0-11ED-BE56-0242AC120002

## Commands
### Scan
The ESP will scan nearby WiFi APs and provide a list.
```json
{"c": "scan"}
```
```json
{"generated":87840," nearby":[{"ssid":"network1","rssi":-34,"encryption":"wpa2"},{"ssid":"Network Number 2","rssi":-89,"encryption":"wpa2"}]}
```

### Connect
The ESP will attempt connection to this network. If credentials work, the device will save them across future reboots.
```json
{"c": "connect", "ssid": "network name", "pass": "P@ssword"}
```
In the event of a success, the device will notify on the WiFi status characteristic and wait a short time before rebooting.

In the event of a failure, the device will respond on the Command Response characteristic and notify on the WiFi status characteristic, which can be read for the failure status.
```json
{"status": "failed"}
```
## Flow
```mermaid
graph TD;
    initializeFS[Initialize LittleFS]
    checkFile[Check WiFi credentials file]
    initializeFS-->checkFile
    tryConfig[Try Connection]
    checkFile--Exists-->tryConfig
    bleSetupStart[BLE Setup Start]
    checkFile--Doesn't exist-->bleSetupStart
    tryConfig--Fail-->bleSetupStart

    exit[Normal App]
    tryConfig--Succeeds-->exit

    startTimer[Start powerdown timer]
    bleSetupStart-->startTimer

    bleLoop[BLE Loop]
    startTimer-->bleLoop
    timerExceeded{Timer Exceeded?}
    bleLoop-->timerExceeded

    powerOff[Deep Sleep]
    timerExceeded--Yes-->powerOff

    receiveCommand[Receive Command]
    receiveCommand-.No Command.->bleLoop
    timerExceeded--No-->receiveCommand
    updateTimer[Update Timer]
    receiveCommand-->scan
    scan[Scan]
    connect[Connect]
    receiveCommand-->connect
    scan-->updateTimer

    tryNewCreds[Try new credentials]
    connect-->tryNewCreds
    tryNewCreds--Fail-->updateTimer
    saveAndReboot[Save and Reboot]
    tryNewCreds--Succeeds-->saveAndReboot
    updateTimer.->bleLoop
```