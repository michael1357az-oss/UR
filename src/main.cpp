#include "Arduino.h"
#include "WebServer.h"
#include "WiFi.h"
#include "bits/stdc++.h"
#include "esp32-hal-ledc.h"
#include "esp_camera.h"
#include "Servo.h"

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
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Motor Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body{font-family:Arial,sans-serif;margin:0;background-color:#282c34;color:white;display:flex;min-height:100vh}
        .sidebar{width:100px;background-color:#3c4049;padding:20px 0;display:flex;flex-direction:column;align-items:center;justify-content:center;box-shadow:2px 0 5px rgba(0,0,0,0.2);position:sticky;top:0;height:100vh}
        .sidebar.right{box-shadow:-2px 0 5px rgba(0,0,0,0.2)}
        .main-content{flex-grow:1;text-align:center;padding:20px}
        h1{color:#61dafb;margin-top:0}
        .btn-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;max-width:300px;margin:20px auto}
        .btn{padding:20px;font-size:1.2em;cursor:pointer;border:none;border-radius:5px;background-color:#61dafb;color:#282c34;text-decoration:none;display:block}
        .btn:active{background-color:#4fa8c8}
        .stop{grid-column:2;background-color:#ff5555}
        .placeholder{visibility:hidden}
        .speed-control{display:flex;flex-direction:column;align-items:center;height:100%;justify-content:center;position:relative}
        label{font-size:1.1em;transform:rotate(-90deg);white-space:nowrap;position:absolute;top:50%;transform-origin:0 0;z-index:1}
        .sidebar.left label{left:-35px}
        .sidebar.right label{left:-40px}
        input[type="range"]{-webkit-appearance:none;appearance:none;width:150px;height:15px;background:#555;outline:none;border-radius:5px;transform:rotate(-90deg);margin:0;position:relative;z-index:0}
        input[type="range"]::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:30px;height:30px;border-radius:50%;background:#61dafb;cursor:pointer}
        input[type="range"]::-moz-range-thumb{width:30px;height:30px;border-radius:50%;background:#61dafb;cursor:pointer}
        .speed-value-display{margin-top:20px;font-size:1.2em;font-weight:bold}
    </style>
</head>
<body>
    <aside class="sidebar left">
        <div class="speed-control">
            <label for="speedLeft">L Speed</label>
            <input type="range" id="speedLeft" min="0" max="255" value="%CURRENT_SPEED_LEFT%" oninput="updateSpeed('left', this.value)">
            <div class="speed-value-display"><span id="speedValueLeft">%CURRENT_SPEED_LEFT%</span></div>
        </div>
    </aside>
    <main class="main-content">
        <h1>Motor Control</h1>
        <div class="btn-grid">
            <div class="placeholder"></div><a href="/action?dir=1"><button class="btn">▲</button></a><div class="placeholder"></div>
            <a href="/action?dir=2"><button class="btn">◄</button></a><a href="/action?dir=0"><button class="btn stop">STOP</button></a><a href="/action?dir=4"><button class="btn">►</button></a>
            <div class="placeholder"></div><a href="/action?dir=3"><button class="btn">▼</button></a><div class="placeholder"></div>
        </div>
    </main>
    <aside class="sidebar right">
        <div class="speed-control">
            <label for="speedRight">R Speed</label>
            <input type="range" id="speedRight" min="0" max="255" value="%CURRENT_SPEED_RIGHT%" oninput="updateSpeed('right', this.value)">
            <div class="speed-value-display"><span id="speedValueRight">%CURRENT_SPEED_RIGHT%</span></div>
        </div>
    </aside>
    <script>
        function updateSpeed(motor, value){
            document.getElementById('speedValue'+(motor.charAt(0).toUpperCase()+motor.slice(1))).innerText=value;
            fetch('/speed?motor='+motor+'&value='+value);
        }
        document.addEventListener('DOMContentLoaded',function(){
            const initialSpeedLeft=%CURRENT_SPEED_LEFT%;
            document.getElementById('speedLeft').value=initialSpeedLeft;
            document.getElementById('speedValueLeft').innerText=initialSpeedLeft;
            const initialSpeedRight=%CURRENT_SPEED_RIGHT%;
            document.getElementById('speedRight').value=initialSpeedRight;
            document.getElementById('speedValueRight').innerText=initialSpeedRight;
        });
    </script>
</body>
</html>
)rawliteral";

// wifi setting
const char *ssid = "pppppp";
const char *password = "310110199701093724";

// Webserver
WebServer server(80);

void motor_direction(int dir);

void handle_root() {
  String page = html_content;
  // Insert the current speed values for both motors into the HTML
  page.replace("%CURRENT_SPEED_LEFT%", String(currentSpeedLeft));
  page.replace("%CURRENT_SPEED_RIGHT%", String(currentSpeedRight));
  server.send(200, "text/html", page);
}

void handle_action() {
  if (server.hasArg("dir")) {
    int dir = server.arg("dir").toInt();
    motor_direction(dir);
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// Updated handler to control individual motor speed
void handle_speed() {
  if (server.hasArg("motor") && server.hasArg("value")) {
    String motor = server.arg("motor");
    int speed = server.arg("value").toInt();
    speed = constrain(speed, 0, 255);

    if (motor == "left") {
      currentSpeedLeft = speed;
      ledcWrite(MOTOR_CHA_A, currentSpeedLeft);
      Serial.print("Left motor speed set to: ");
      Serial.println(currentSpeedLeft);
    } else if (motor == "right") {
      currentSpeedRight = speed;
      ledcWrite(MOTOR_CHA_B, currentSpeedRight);
      Serial.print("Right motor speed set to: ");
      Serial.println(currentSpeedRight);
    }
  }
  server.send(200, "text/plain", "OK");
}

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

  server.on("/", handle_root);
  server.on("/action", handle_action);
  server.on("/speed", handle_speed);

  Serial.println("init complete");
}

void loop() {
  server.handleClient();
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
