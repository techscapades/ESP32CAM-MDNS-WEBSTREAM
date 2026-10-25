/*
https://github.com/techscapades
@techscapades
*/

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// ===== WiFi Settings =====
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// ===== mDNS Hostname =====
String hostname = "espcam2";

// ===== OV2640 Camera Pins =====
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM 15
#define XCLK_GPIO_NUM  27
#define SIOD_GPIO_NUM  25
#define SIOC_GPIO_NUM  23

#define Y9_GPIO_NUM    19
#define Y8_GPIO_NUM    36
#define Y7_GPIO_NUM    18
#define Y6_GPIO_NUM    39
#define Y5_GPIO_NUM    5
#define Y4_GPIO_NUM    34
#define Y3_GPIO_NUM    35
#define Y2_GPIO_NUM    17

#define VSYNC_GPIO_NUM 22
#define HREF_GPIO_NUM  26
#define PCLK_GPIO_NUM  21

// ===== HTTP Server =====
WebServer server(81);
bool cameraReady = false;

// ===== MJPEG Stream Handler =====
void handleStream() {
  if (!cameraReady) {
    server.send(503, "text/plain", "Camera is not initialized");
    return;
  }

  WiFiClient client = server.client();

  client.print(
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
    "Cache-Control: no-cache\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Connection: close\r\n\r\n"
  );

  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();

    if (fb == nullptr) {
      Serial.println("Camera capture failed");
      break;
    }

    if (fb->format == PIXFORMAT_JPEG) {
      client.printf(
        "--frame\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: %u\r\n\r\n",
        (unsigned int)fb->len
      );

      size_t written = client.write(fb->buf, fb->len);
      client.print("\r\n");

      esp_camera_fb_return(fb);

      if (written == 0) {
        break;
      }
    } else {
      esp_camera_fb_return(fb);
      Serial.println("Unexpected camera format");
      break;
    }

    delay(50);
  }

  client.stop();
  Serial.println("Stream client disconnected");
}

// ===== Start HTTP Server =====
void startCameraServer() {
  server.on("/stream", HTTP_GET, handleStream);

  server.on("/", HTTP_GET, []() {
    server.send(
      200,
      "text/html",
      "<!DOCTYPE html>"
      "<html><head>"
      "<meta name='viewport' content='width=device-width, initial-scale=1'>"
      "<title>ESP32 Camera</title>"
      "</head><body>"
      "<h2>ESP32-CAM OV2640</h2>"
      "<img src='/stream' style='width:100%;max-width:640px;'>"
      "</body></html>"
    );
  });

  server.begin();
  Serial.println("HTTP server started on port 81");
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Starting ESP32-CAM...");

  // ----- Camera Configuration -----
  camera_config_t config = {};

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk  = XCLK_GPIO_NUM;
  config.pin_pclk  = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href  = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn  = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_QVGA;  // 160 x 120
  config.jpeg_quality = 10; //LOWER IS BETTER
  config.fb_count     = 1;
  config.fb_location  = CAMERA_FB_IN_DRAM;
  config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

  Serial.printf(
    "SDA=%d SCL=%d RESET=%d XCLK=%d\n",
    config.pin_sccb_sda,
    config.pin_sccb_scl,
    config.pin_reset,
    config.pin_xclk
  );

  // ----- Initialize Camera -----
  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    Serial.println("Check camera wiring and sensor.");
    return;
  }

  cameraReady = true;
  Serial.println("OV2640 camera initialized successfully");

  // ----- Connect to WiFi -----
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");

  Serial.print("IP address: http://");
  Serial.println(WiFi.localIP());

  // ----- Start mDNS -----
  if (MDNS.begin(hostname.c_str())) {
    Serial.println("mDNS started successfully");

    // Advertise the HTTP service on port 81
    MDNS.addService("http", "tcp", 81);

    Serial.print("Hostname: http://");
    Serial.print(hostname);
    Serial.println(".local");

    Serial.print("Stream URL: http://");
    Serial.print(hostname);
    Serial.println(".local:81/stream");
  } else {
    Serial.println("ERROR: Failed to start mDNS");
    Serial.println("Use the IP address shown above instead.");
  }

  // ----- Start Web Server -----
  startCameraServer();
}

// ===== Main Loop =====
void loop() {
  if (cameraReady) {
    server.handleClient();
  }

  delay(1);
}
