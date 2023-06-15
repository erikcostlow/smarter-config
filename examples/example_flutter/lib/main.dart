import 'package:f_logs/f_logs.dart';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:go_router/go_router.dart';
import 'package:smarter_config_example/ble_device_screen.dart';

import 'list_ble_devices_screen.dart';

void main() {
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  MyApp({super.key});

  final _navigatorKey = GlobalKey<NavigatorState>();
  final _scaffoldKey = GlobalKey<ScaffoldMessengerState>();

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    var r = MaterialApp.router(
      routerConfig: _router,
      scaffoldMessengerKey: _scaffoldKey,
    );
    return r;
  }

  final _router = GoRouter(routes: [
    GoRoute(
      path: '/',
      builder: (context, state) => ListBLEDevicesScreen(),
    ),
    GoRoute(
        path: '/device',
        builder: (context, state) {
          var deviceNullable = state.extra;
          if (deviceNullable == null) {
            FLog.error(text: "Navigating to device control without device.");
          }
          BluetoothDevice device = deviceNullable! as BluetoothDevice;
          return BleDeviceScreen(device: device);
        }),
  ]);
}
