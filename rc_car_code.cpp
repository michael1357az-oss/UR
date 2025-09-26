#include "esp_camera.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h> // Required for ESPAsyncWebServer
#include <ESP32Servo.h> // Use ESP32Servo for compatibility (install if needed)

// WiFi credentials
const char* ssid = "RC-Car";
const char* password = "12345678";

// Pin definitions
#define left_motor_fwd 14  // IN1
#define left_motor_rev 15  // IN2
#define right_motor_fwd 13 // IN3
#define right_motor_rev 12 // IN4
#define MOTOR_ENA 27       // PWM for left motor
#define MOTOR_ENB 33       // PWM for right motor
#define lower_servo 2      // Steering servo
#define upper_servo 16     // Optional upper servo (e.g., for camera tilt)
#define MOTOR_IN1 left_motor_fwd
#define MOTOR_IN2 left_motor_rev
#define MOTOR_IN3 right_motor_fwd
#define MOTOR_IN4 right_motor_rev

AsyncWebServer server(80);
Servo steeringServo;
int motorSpeed = 200; // Default speed (0-255)
int steerAngle = 90;  // Center position for servo
bool motorStopped = true;

// Camera configuration for ESP32-CAM
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

// Forward declarations
void forward();
void back();
void left();
void right();
void stopMotors();
void handleForward(AsyncWebServerRequest *request);
void handleBack(AsyncWebServerRequest *request);
void handleLeft(AsyncWebServerRequest *request);
void handleRight(AsyncWebServerRequest *request);
void handleStop(AsyncWebServerRequest *request);

void setup() {
  // Initialize Serial
  Serial.begin(115200);

  // Motor pin setup
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);
  pinMode(MOTOR_ENB, OUTPUT);
  stopMotors();

  // Servo setup
  steeringServo.attach(lower_servo);
  steeringServo.write(90); // Center position

  // Camera initialization
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA; // 640x480
  config.jpeg_quality = 10; // 0-63, lower means higher quality
  config.fb_count = 1;

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // WiFi setup
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html",
      "<html><body><h1>RC Car Control</h1>"
      "<img src='/stream' style='width:100%;'><br>"
      "<button onclick='fetch(\"/forward\")'>Forward</button>"
      "<button onclick='fetch(\"/left\")'>Left</button>"
      "<button onclick='fetch(\"/stop\")'>Stop</button>"
      "<button onclick='fetch(\"/right\")'>Right</button>"
      "<button onclick='fetch(\"/back\")'>Back</button>"
      "</body></html>");
  });

  server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginChunkedResponse("multipart/x-mixed-replace; boundary=frame", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) return 0;
      size_t len = min(fb->len, maxLen);
      memcpy(buffer, fb->buf, len);
      esp_camera_fb_return(fb);
      return len;
    });
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  // Control endpoints
  server.on("/forward", HTTP_GET, handleForward);
  server.on("/back", HTTP_GET, handleBack);
  server.on("/left", HTTP_GET, handleLeft);
  server.on("/right", HTTP_GET, handleRight);
  server.on("/stop", HTTP_GET, handleStop);

  server.begin();
}

void loop() {
  delay(100); // Idle loop
}

// Motor control functions
void forward() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, motorSpeed);
  analogWrite(MOTOR_ENB, motorSpeed);
  steeringServo.write(90); // Center steering
  motorStopped = false;
}

void back() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, HIGH);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, HIGH);
  analogWrite(MOTOR_ENA, motorSpeed);
  analogWrite(MOTOR_ENB, motorSpeed);
  steeringServo.write(90); // Center steering
  motorStopped = false;
}

void left() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, motorSpeed / 2); // Slow left motor
  analogWrite(MOTOR_ENB, motorSpeed);
  steeringServo.write(60); // Steer left
  motorStopped = false;
}

void right() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, motorSpeed);
  analogWrite(MOTOR_ENB, motorSpeed / 2); // Slow right motor
  steeringServo.write(120); // Steer right
  motorStopped = false;
}

void stopMotors() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, 0);
  analogWrite(MOTOR_ENB, 0);
  steeringServo.write(90); // Center steering
  motorStopped = true;
}

// HTTP handlers
void handleForward(AsyncWebServerRequest *request) {
  forward();
  request->send(200, "text/plain", "Forward");
}

void handleBack(AsyncWebServerRequest *request) {
  back();
  request->send(200, "text/plain", "Back");
}

void handleLeft(AsyncWebServerRequest *request) {
  left();
  request->send(200, "text/plain", "Left");
}

void handleRight(AsyncWebServerRequest *request) {
  right();
  request->send(200, "text/plain", "Right");
}

void handleStop(AsyncWebServerRequest *request) {
  stopMotors();
  request->send(200, "text/plain", "Stop");
}
