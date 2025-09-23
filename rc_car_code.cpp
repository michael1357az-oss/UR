#include "esp_camera.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Servo.h>

const char* ssid = "RC-Car";
const char* password = "12345678"

#define left_motor_fwd 14  // corr.pin is IN1
#define left_motor_rev 15  // corr.pin is IN2
#define right_motor_fwd 13 // corr.pin is IN3 
#define right_motor_rev 12 // corr.pin is IN4
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
  pinMode(MOTOR_ENA, OUTPUT);
  pinMode(MOTOR_ENB, OUTPUT);
  stopMotors();

  // Motor control functions
void forward() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, speed);
  analogWrite(MOTOR_ENB, speed);
}

void back() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, HIGH);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, HIGH);
  analogWrite(MOTOR_ENA, speed);
  analogWrite(MOTOR_ENB, speed);
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

