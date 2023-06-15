import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'ble_scan_result.dart';
import 'ble_uuids.dart' as BLE_UUIDs;

class ListBLEDevicesScreen extends StatefulWidget {
  @override
  State<ListBLEDevicesScreen> createState() => _ListBLEDevicesScreenState();
}

class _ListBLEDevicesScreenState extends State<ListBLEDevicesScreen> {
  static final FlutterBluePlus _flutterBlue = FlutterBluePlus.instance;
  static final List<Guid> _serviceFilter = [Guid(BLE_UUIDs.CONFIG_SERVICE)];

  late StreamSubscription<bool> scanListener;

  bool scanning = false;

  _ListBLEDevicesScreenState() {
    {
      scanListener = _flutterBlue.isScanning
          .listen((event) => setState(() => scanning = event));
    }
  }

  @override
  void dispose() {
    super.dispose();
    scanListener.cancel();
  }

  Map<String, ScanResult> devicesByName = {};

  void scan() {
    if (!scanning) {
      setState(() {
        devicesByName.clear();
      });
      //Do the scan
      FlutterBluePlus.instance
          .scan(
              withServices: _serviceFilter, timeout: const Duration(seconds: 4))
          .forEach((element) =>
              setState(() => devicesByName[element.device.name] = element));
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("Nearby Devices"),
      ),
      body: Column(
        children: [
          TextButton(
            onPressed: () {
              if (!scanning) {
                scan();
              }
            },
            child: Text(scanning ? "Please Wait" : "Scan"),
          ),
          ...devicesByName.values
              .map((sr) => BLEScanResult(rssi: sr.rssi, device: sr.device))
        ],
      ),
    );
  }
}
