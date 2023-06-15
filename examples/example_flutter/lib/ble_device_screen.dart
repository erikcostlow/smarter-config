import 'dart:async';
import 'dart:convert';

import 'package:f_logs/f_logs.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'ble_uuids.dart' as BLE_UUIDs;

class BleDeviceScreen extends StatefulWidget {
  BluetoothDevice device;

  BleDeviceScreen({required this.device});

  @override
  State<BleDeviceScreen> createState() => _BleDeviceScreenState();
}

class _BleDeviceScreenState extends State<BleDeviceScreen> {
  late StreamSubscription<BluetoothDeviceState> connectedListener;
  BluetoothDeviceState connected = BluetoothDeviceState.disconnected;

  @override
  void initState() {
    super.initState();
    var w = widget;
    var device = w.device;
    var state = device.state;
    connectedListener = state.listen((event) => setState(() {
          connected = event;
          if (connected == BluetoothDeviceState.connected) {
            setupServices();
          }
        }));
    if (connected == BluetoothDeviceState.disconnected) {
      FLog.info(text: "Not connected, connecting.");
      device.connect().catchError((err) {
        FLog.info(text: "Connection error: ${err.toString()}");
      });
    }
  }

  @override
  void dispose() {
    super.dispose();
    //_commandValue.dispose();
    connectedListener.cancel();
    if (connected == BluetoothDeviceState.connected) {
      widget.device.disconnect();
    }
    commandResponseCallback!.cancel();
    wifiStatusCallback.cancel();
  }

  late BluetoothCharacteristic _commandCharacteristic;
  late BluetoothCharacteristic _commandResponseCharacteristic;
  late StreamSubscription<List<int>>? commandResponseCallback = null;
  final StringBuffer _commandResponseBuffer = StringBuffer("No command yet...");
  String _wifiStatus = "Unknown connection";

  late StreamSubscription<List<int>> wifiStatusCallback;

  void setupServices() async {
    FLog.info(text: "Setting up BLE callbacks");
    var device = widget.device;
    if (connected != BluetoothDeviceState.connected) {
      FLog.info(text: "Device is $connected so can't discover services.");
      return;
    }
    await device.discoverServices();

    List<BluetoothService> services = await device.discoverServices();
    BluetoothService? service = services.firstWhere(
      (element) =>
          element.uuid.toString().toUpperCase() == BLE_UUIDs.CONFIG_SERVICE,
    );
    var characteristicsMap = {
      for (var element in service.characteristics)
        element.uuid.toString().toUpperCase(): element
    };
    _commandCharacteristic =
        characteristicsMap[BLE_UUIDs.CHARACTERISTIC_COMMAND]!;
    _commandResponseCharacteristic =
        characteristicsMap[BLE_UUIDs.CHARACTERISTIC_RESPONSE]!;

    FLog.info(text: 'Listening for notify on CommandResponse');
    await _commandResponseCharacteristic!.setNotifyValue(true);
    if (commandResponseCallback != null) {
      commandResponseCallback!.cancel();
    }
    commandResponseCallback = _commandResponseCharacteristic.value
        .listen(onCommandResponseNotification);

    var wifiStatusCharacteristic =
        characteristicsMap[BLE_UUIDs.CHARACTERISTIC_WIFI_STATUS];
    if (wifiStatusCharacteristic == null) {
      FLog.info(text: 'wifi status not found');
    } else {
      FLog.info(text: 'Got wifi status characteristic');
      var newValueInts = await wifiStatusCharacteristic.read();
      String newValue = String.fromCharCodes(newValueInts);
      setState(() {
        _wifiStatus = newValue;
      });
      await wifiStatusCharacteristic.setNotifyValue(true);

      wifiStatusCallback =
          wifiStatusCharacteristic.onValueChangedStream.listen((event) async {
        FLog.info(text: "wifi status was notified");

        //READING THE VALUE CALLS THIS AGAIN. FIX INFINITE LOOP.
        String newValue = String.fromCharCodes(event);
        if (newValue == "") {
          //we're told to re-read the value
          var newValueInts = await wifiStatusCharacteristic.read();
          newValue = String.fromCharCodes(newValueInts);
          FLog.info(text: "  new val $newValue");
          if (newValue != _wifiStatus) {
            FLog.info(text: ' new value is $newValue');
            setState(() {
              _wifiStatus = newValue;
            });
          }
        }
      });
    }
  }

  void onCommandResponseNotification(List<int> event) {
    String incoming = String.fromCharCodes(event);
    //FLog.info(text: 'Incoming is $incoming');
    //messagesReceived++;
    setState(() {
      _commandResponseBuffer.write(incoming);
    });
    /*FLog.info(
        text:
            "Messages received: $messagesReceived with length ${_commandResponse.length}");*/
    if (incoming == "") {
      setState(() {
        FLog.info(text: "Finished receiving with: $_commandResponseBuffer");
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(),
      body: Column(children: [
        ElevatedButton(
            onPressed: () {
              FLog.info(text: "toggling connection");
              setState(() {
                if (connected == BluetoothDeviceState.connected) {
                  FLog.info(text: "Disconnecting");
                  widget.device.disconnect();
                } else {
                  FLog.info(text: "Connecting");
                  widget.device.connect();
                }
              });
            },
            child: Text(connected == BluetoothDeviceState.connected
                ? "Disconnect"
                : "Connect")),
        Text("Wifi Status: $_wifiStatus"),
        TextButton(
            onPressed: () {
              var value = utf8.encode("{\"c\": \"scan\"}");
              _commandCharacteristic.write(value);
            },
            child: Text("Scan WiFi")),
        Text(_commandResponseBuffer.toString())
      ]),
    );
  }
}
