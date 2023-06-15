import 'package:flutter/material.dart';

class RssiHelper {
  static Icon rssiToIcon(int rssi) {
    final IconData data;
    final Color color;
    if (rssi > -70) {
      data = Icons.network_wifi;
      color = Colors.green;
    } else if (rssi > -85) {
      data = Icons.network_wifi_3_bar;
      color = Colors.greenAccent;
    } else if (rssi > -100) {
      data = Icons.network_wifi_2_bar;
      color = Colors.yellowAccent;
    } else if (rssi > -125) {
      data = Icons.network_wifi_1_bar;
      color = Colors.red[300]!;
    } else {
      data = Icons.wifi_off;
      color = Colors.red;
    }

    return Icon(
      data,
      color: color,
    );
  }
}
