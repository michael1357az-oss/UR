#include "Arduino.h"
#include "WebServer.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "ESP32Servo.h"

// Motor Pin
const int PIN_IN1 = 12; 
const int PIN_IN2 = 13; 
const int PIN_IN3 = 2;
const int PIN_IN4 = 4;

// Servo Pin
const int PIN_SER_U = 14;
const int PIN_SER_L = 15;

//wifi setting
const char *ssid = "pppppp";
const char *password = "310110199701093724";

// Webserver
WebServer server(80);

// Servo
Servo upper_servo;
Servo lower_servo;
int current_upper_angle = 90,current_lower_angle = 90;

// Motor control
uint8_t current_speed = 180;
bool motor_break = 0;
int prev_action = 1;

const char *html_content = R"rawliteral(
<!DOCTYPE html>
<head>
  <title>ESP32</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { text-align: center; font-family: Arial, sans-serif; }
    table { margin: 0 auto; border-collapse: collapse; }
    td { padding: 5px; }
    button { width: 80px; height: 40px; font-size: 16px; }
    .stop-btn { background-color:#ff4d4d; width:165px; margin-top:10px; } 
  </style>
</head>
<body>
    <h1>RC Mode</h1>
    <table>
        <tr>
            <td></td>
            <td><a href="/action?dir=1"><button>UP</button></a></td>
            <td></td>
        </tr>
        <tr>
            <td><a href="/action?dir=2"><button>LEFT</button></a></td>
            <td></td> <td><a href="/action?dir=4"><button>RIGHT</button></a></td>
        </tr>
        <tr>
            <td></td>
            <td><a href="/action?dir=3"><button>DOWN</button></a></td>
            <td></td>
        </tr>
    </table>
    <div>
        <a href="/action?dir=0"><button class="stop-btn">BREAK/RESUME</button></a>
    </div>
    <div>
        <a href="/action?dir=-1"><button class="stop-btn">AUTO</button></a>
    </div>
    <hr>
    <div>Current Upper Servo angle: %CURRENT_UPPERSERVO%</div>
    <div>Current Lower Servo angle: %CURRENT_LOWERSERVO%</div>
    <div><form action="/servo" method="GET">
      (0-180):
      UPPER<input type="range" name="upper_angle" min="0" max="180" value="%CURRENT_UPPERSERVO%">
      LOWER<input type="range" name="lower_angle" min="0" max="180" value="%CURRENT_LOWERSERVO%">
      <input type="submit" value="Set angle">
    </form></div>
</body>
)rawliteral";

// Control Function
void motor_direction(int dir);
void automatic_mode(); 
//Web Server Function
void handle_root();
void handle_action();
void handle_servo();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Wifi
  Serial.printf("Connecting to %s with the password %s \n",ssid,password);
  WiFi.begin(ssid, password);
  Serial.printf("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf(".");
  }
  Serial.printf("\n");

  Serial.print("WIFI Connected,go to http://");
  Serial.println(WiFi.localIP());
  
  // pin setting
  pinMode(PIN_IN1, OUTPUT); pinMode(PIN_IN3, OUTPUT); 
  pinMode(PIN_IN2, OUTPUT); pinMode(PIN_IN4, OUTPUT);

  upper_servo.attach(PIN_SER_U);
  lower_servo.attach(PIN_SER_L);
  upper_servo.write(current_upper_angle); 
  lower_servo.write(current_lower_angle);

  // Stop motor
  Serial.println("Motor stopped");
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, LOW);
  digitalWrite(PIN_IN4, LOW);

  //Webserver
  server.on("/", handle_root);
  server.on("/action", handle_action);
  server.on("/servo",handle_servo);
  server.begin();

  Serial.println("init complete");
}


void loop() {
  server.handleClient();
  delay(10);
}


void motor_direction(int dir) {
  dir = constrain(dir, 0, 4);
  
  // when dir = 0, stop
  switch (dir) {
  case 0:
    if(!motor_break){
      Serial.println("Motor break");
      digitalWrite(PIN_IN1, LOW);
      digitalWrite(PIN_IN2, LOW);
      digitalWrite(PIN_IN3, LOW);
      digitalWrite(PIN_IN4, LOW);
      motor_break = 1;
      return;
    }else{
      Serial.println("Motor resume");
      motor_break = 0;
      motor_direction(prev_action); 
      return;
    }
  case 1:
    Serial.println("Motor move forward");
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, HIGH);
    digitalWrite(PIN_IN4, LOW);
    break;
  case 2:
    Serial.println("Motor move left");
    digitalWrite(PIN_IN1, LOW); // Clockwise 
    digitalWrite(PIN_IN2, HIGH);
    digitalWrite(PIN_IN3, HIGH); // Anti-clockwise 
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
    digitalWrite(PIN_IN1, HIGH); // Anti-clockwise
    digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, LOW); // Clockwise
    digitalWrite(PIN_IN4, HIGH);
    break;
  }
  motor_break = 0;
  prev_action = dir;
  /*
     * 1
    4     2
     * 3
  */
}


void automatic_mode(){
  //TODO  
}


void handle_root() {
  String page = html_content;
  page.replace("%CURRENT_SPEED%", String(current_speed));
  page.replace("%CURRENT_UPPERSERVO%",String(current_upper_angle));
  page.replace("%CURRENT_LOWERSERVO%",String(current_lower_angle));
  server.send(200, "text/html", page); 
}

void handle_action() {
  if (server.hasArg("dir")) {
    int dir = server.arg("dir").toInt();
    if(dir == -1) automatic_mode();
    else motor_direction(dir);
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handle_servo(){
  if(server.hasArg("upper_angle")){
    int ang = server.arg("upper_angle").toInt();
    ang = constrain(ang,0,180);
    upper_servo.write(ang);
    current_upper_angle = ang;
    Serial.printf("Upper Servo Angle: %d\n",current_upper_angle);
  }
  if(server.hasArg("lower_angle")){
    int ang = server.arg("lower_angle").toInt();
    ang = constrain(ang,0,180);
    lower_servo.write(ang);
    current_lower_angle = ang;
    Serial.printf("Lower Servo Angle: %d\n",current_lower_angle);
  }
  server.sendHeader("Location","/",true);
  server.send(302,"text/plain","");
}