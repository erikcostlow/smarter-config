import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:go_router/go_router.dart';

import 'rssi_helper.dart';

class BLEScanResult extends StatelessWidget {
  final int rssi;

  final BluetoothDevice device;

  const BLEScanResult({super.key, required this.rssi, required this.device});

  @override
  Widget build(BuildContext context) {
    return Container(
      width: double.infinity,
      //height: 35,
      child: Padding(
        padding: const EdgeInsets.only(left: 25),
        child: Column(
          children: [
            Row(
              children: [
                Text(device.name),
                RssiHelper.rssiToIcon(rssi),
                ElevatedButton(
                    onPressed: () {
                      context.push(
                          Uri(path: '/device', queryParameters: {
                            'deviceId': device.id.toString()
                          }).toString(),
                          extra: device);
                    },
                    child: const Text("Read")),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
