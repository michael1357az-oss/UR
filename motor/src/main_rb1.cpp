#include "Arduino.h"
#include "WebServer.h"
#include "WiFi.h"
#include "esp32-hal-ledc.h"
#include "esp_camera.h"
#include "ESP32Servo.h"

// Motor Pin
const int PIN_IN1 = 12;
const int PIN_IN2 = 13;
const int PIN_IN3 = 15;
const int PIN_IN4 = 16;
// pwm pin
const int PIN_ENA = 2;
const int PIN_ENB = 14;

// pwm
const int FREQ = 5000;         // PWN FREQ
const int RES = 8;             // 255
const uint8_t MOTOR_CHA_A = 0; // LERC CHA A
const uint8_t MOTOR_CHA_B = 1; // LERC CHA B

uint8_t currentSpeedLeft = 0, currentSpeedRight = 0;

const char *html_content = R"rawliteral(
 "<html><body><h1>RC Car Control</h1>"
<div class="placeholder"></div><a href="/action?dir=1"><button class="btn">▲</button></a><div class="placeholder"></div>
<a href="/action?dir=2"><button class="btn">◄</button></a><a href="/action?dir=0"><button class="btn stop">STOP</button></a><a href="/action?dir=4"><button class="btn">►</button></a>
<div class="placeholder"></div><a href="/action?dir=3"><button class="btn">▼</button></a><div class="placeholder"></div>  
 "</body></html>"
)rawliteral";

// wifi setting
const char *ssid = "pppppp";
const char *password = "310110199701093724";

// Webserver
WebServer server(80);

void motor_direction(int dir);

void handle_root() {
  String page = html_content;
  server.send(200, "text/html", page);
}

void handle_jpg_stream();

/*
void startCameraServer() {
//  server.on("/", HTTP_GET, handle_root);
  server.on("/stream", HTTP_GET, handle_jpg_stream);
  server.begin();
}

void handle_jpg_stream(void) {
  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);

  uint64_t lastTime = millis();
  int frameCount = 0;
  uint64_t startTime = millis();

  while (client.connected()) {
    uint64_t captureStart = micros(); // Start timing capture
    camera_fb_t *fb = esp_camera_fb_get();
    uint64_t captureTime = micros() - captureStart; // Time taken for capture

    if (!fb) {
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
    if (currentTime - lastTime >= 1000) { // Every second, calculate FPS
      float fps = frameCount * 1000.0 / (currentTime - lastTime);
      Serial.printf("FPS: %.2f, Capture time: %lu us, Frame size: %u bytes\n", fps, captureTime, fb->len);
      frameCount = 0;
      lastTime = currentTime;
    }
  }
}
*/

void handle_action() {
  if (server.hasArg("dir")) {
    int dir = server.arg("dir").toInt();
    motor_direction(dir);
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// Updated handler to control individual motor speed
// Auto mode for line tracking
void automatic_mode() {
  // TODO
}

// Motor Direction control

void setup() {
  Serial.begin(115200);
  delay(100);

  // Wifi
  Serial.print("Connecting to");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WIFI Connected");
  Serial.print("IP Address:");
  Serial.println(WiFi.localIP());
  
  // pin setting
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  ledcChangeFrequency(MOTOR_CHA_A, FREQ, RES);
  ledcChangeFrequency(MOTOR_CHA_B, FREQ, RES);
  ledcAttachPin(PIN_ENA, MOTOR_CHA_A);
  ledcAttachPin(PIN_ENB, MOTOR_CHA_B);
  motor_direction(0);

  //webserver
  server.on("/", handle_root);
  server.on("/action", handle_action);
  //startCameraServer();

  Serial.println("init complete");
}

void loop() {
  server.handleClient();
  Serial.println(WiFi.localIP());
  delay(10);
}

void motor_direction(int dir) {
  dir = constrain(dir, 0, 4);
  ledcWrite(MOTOR_CHA_A, currentSpeedLeft);
  ledcWrite(MOTOR_CHA_B, currentSpeedRight);
      // when dir = 0, stop
  switch (dir) {
  case 0:
    Serial.println("Motor stop");
    currentSpeedLeft = currentSpeedRight = 0u;
    break;
  case 1:
    Serial.println("Motor move forward");
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, HIGH);
    digitalWrite(PIN_IN4, LOW);
    break;
  case 2:
    Serial.println("Motor move left");
    digitalWrite(PIN_IN1, HIGH); // when motor1 at left side
    digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, LOW); // turn off
    digitalWrite(PIN_IN4, LOW);
    break;
  case 3:
    Serial.println("Motor move backward");
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, HIGH);
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, HIGH);
    break;
  case 4:
    Serial.println("Motor move right");
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, HIGH);
    break;
  }
  /*
     * 1
    4     2
     * 3
  */
}
