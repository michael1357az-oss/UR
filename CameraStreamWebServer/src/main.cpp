#include <Arduino.h>
int onboardled = 33;

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
//const char *ssid = "KiraIphone";   // WiFi SSID (network name)
//const char *password = "kira1114"; // WiFi password
const char *ssid = "UR-FallTrain";   // WiFi SSID (network name)
const char *password = "falltrain"; // WiFi password

// ==================
// Camera Pin Settings for AI-Thinker Module
// ==================
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
// ==================

void startCameraServer();
void motor_direction(int);
WebServer server(80);



// Stream JPG images as MJPEG over HTTP
void handle_jpg_stream(void)
{
  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);

  unsigned long lastTime = millis();
  int frameCount = 0;
  unsigned long startTime = millis();

  while (client.connected())
  {
    unsigned long captureStart = micros(); // Start timing capture
    camera_fb_t *fb = esp_camera_fb_get();
    unsigned long captureTime = micros() - captureStart; // Time taken for capture

    if (!fb)
    {
      Serial.println("Camera capture failed");
      return;
    }

    response = "--frame\r\n";
    response += "Content-Type: image/jpeg\r\n\r\n";
    server.sendContent(response);
    client.write(fb->buf, fb->len);
    server.sendContent("\r\n");
    esp_camera_fb_return(fb);

    frameCount++;
    unsigned long currentTime = millis();
    if (currentTime - lastTime >= 3000)
    { // Every second, calculate FPS
      float fps = frameCount * 1000.0 / (currentTime - lastTime);
      Serial.printf("FPS: %.2f, Capture time: %lu us, Frame size: %u bytes\n", fps, captureTime, fb->len);
      frameCount = 0;
      lastTime = currentTime;
    }
  }
}


// Main page handler
void handle_root()
{
  String html = R"(<!DOCTYPE html>
<html>
<body>
<h1>ESP32-CAM Stream</h1>
<img src='/stream'/>
<form action='/action' method='get'>
    <button type='submit' name='value' value=1>Forward</button>
    <button type='submit' name='value' value=3>Backward</button>
    <button type='submit' name='value' value=2>Left</button>
    <button type='submit' name='value' value=4>Right</button>
</form>
</body>
</html>)";
  server.send(200, "text/html", html);
}

// Start the streaming web server
void startCameraServer()
{
  server.on("/", HTTP_GET, handle_root);
  server.on("/stream", HTTP_GET, handle_jpg_stream);
  server.begin();
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 24000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // ===== Frame size and quality configuration =====
  // To adjust resolution:
  //   FRAMESIZE_QQVGA   160x120
  //   FRAMESIZE_QVGA    320x240
  //   FRAMESIZE_VGA     640x480
  //   FRAMESIZE_SVGA    800x600
  //   FRAMESIZE_XGA     1024x768
  //   FRAMESIZE_SXGA    1280x1024
  //   FRAMESIZE_UXGA    1600x1200
  // To adjust JPEG quality: 0 (best) to 63 (worst), lower value = higher quality

  config.frame_size = FRAMESIZE_QVGA; // Resolution: 1024x768. Change to FRAMESIZE_VGA (640x480) or others as needed.
  config.jpeg_quality = 12;           // JPEG quality. Lower = better image, but slower FPS.
  config.fb_count = 5;

  // ===============================================

  // Camera initialization
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  sensor_t *s = esp_camera_sensor_get();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.print(WiFi.localIP());
  // Start web server
  startCameraServer();
}

void loop()
{
  server.handleClient();
}
