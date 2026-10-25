# ESP32CAM-OV2640-MDNS-WEBSTREAM
This code configures an ESP32-CAM with an OV2640 camera, connects it to Wi-Fi, and hosts a web server on port 81. It streams live JPEG video via MJPEG and provides a simple webpage to view the feed using an IP address or the hostname espcam2.local.

It uses this board: ESP32 CAM Development Board BT WiFi Camera Module with Battery Interface 2.4 GHz DVP 24Pin for OV2640 2MP TYPE-C IP5306

https://www.aliexpress.com/item/1005008810097240.html

Arduino IDE config:

BOARD: ESP32 DEV MODULE

PARTITION SCHEME: HUGE APP (3MB NO OTA)

PSRAM: DISABLED

![ESP32CAM](BOARD.PNG)
