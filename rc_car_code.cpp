#include "esp_camera.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Servo.h>

const char* ssid = "RC-Car";
const char* password = "12345678"

// PINS
#define left_motor_fwd 14  // corr.pin is IN1
#define left_motor_rev 15  // corr.pin is IN2
#define right_motor_fwd 13  // corr.pin is IN3 
#define right_motor_rev 12  // corr.pin is IN4
#define lower_servo 2  
#define upper_servo 16

AsyncWebServer server(80);
Servo steeringServo;
int motorSpeed = 0;
int steerAngle = 0;

// Motor setup
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);
  // no pin yet for spped control pinMode(MOTOR_ENA, OUTPUT);
  //pinMode(MOTOR_ENB, OUTPUT);
  stopMotors();

// Servo setup
  steeringServo.attach(SERVO_PIN);
  steeringServo.write(90);


// Wifi setup
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
      camera_fb_t * fb = esp_camera_fb_get();
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
  delay(100);  // Idle loop
}
// Motor control functions
void forward() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, speed);
  analogWrite(MOTOR_ENB, speed);
  motorStopped = false;
}

void back() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, HIGH);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, HIGH);
  analogWrite(MOTOR_ENA, speed);
  analogWrite(MOTOR_ENB, speed);
  motorStopped = false;
}

void left() {
  // Differential steering: slow left motor
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, speed / 2);
  analogWrite(MOTOR_ENB, speed);
  motorStopped = false;
}

void right() {
  // Differential + servo
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, speed);
  analogWrite(MOTOR_ENB, speed / 2);
  motorStopped = false;
}

void stopMotors() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, 0);
  analogWrite(MOTOR_ENB, 0);
  motorStopped = true;
}

// HTTP handlers
void handleForward(AsyncWebServerRequest *request) { forward(); request->send(200, "text/plain", "Forward"); }
void handleBack(AsyncWebServerRequest *request) { back(); request->send(200, "text/plain", "Back"); }
void handleLeft(AsyncWebServerRequest *request) { left(); request->send(200, "text/plain", "Left"); }
void handleRight(AsyncWebServerRequest *request) { right(); request->send(200, "text/plain", "Right"); }
void handleStop(AsyncWebServerRequest *request) { stopMotors(); request->send(200, "text/plain", "Stop"); }
