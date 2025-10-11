#include "Arduino.h"
#include "WebServer.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "ESP32Servo.h"
#include "esp_wifi.h"

#define UPPER_SERVO false
#define LOWER_SERVO true
#define L_MOTOR true
#define R_MOTOR true

// Motor Pin
#if L_MOTOR
const int PIN_IN1 = 12; 
const int PIN_IN2 = 13;
const int PIN_ENA = 2;
#endif

#if R_MOTOR 
const int PIN_IN3 = 0;
const int PIN_IN4 = 0;
const int PIN_ENB = 0; 
#endif

// Servo Pin
#if UPPER_SERVO
const int PIN_SER_U = 14; // 14
#endif
#if LOWER_SERVO
const int PIN_SER_L = 15; // 15 
#endif
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
uint8_t current_speed_l = 0,current_speed_r = 0;
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
    td { padding: 5px; `}
    button { width: 160px; height: 120px; font-size: 16px; }
    .stop-btn { background-color:#ff4d4d; width:225px; margin-top:10px; } 
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
    <hr>
    <div>Current Upper Servo angle:<strong id="upper_angle_display">%CURRENT_UPPERSERVO%</strong></div>
    <div>Current Lower Servo angle:<strong id="lower_angle_display">%CURRENT_LOWERSERVO%</strong></div>
    <div>Current L Motor Speed:<strong id="l_motor_display">%CURRENT_L_MOTOR_SPEED%</strong></div>
    <div>Current R Motor Speed:<strong id="r_motor_display">%CURRENT_R_MOTOR_SPEED%</strong></div>
    <div>
        Upper Servo:
        <input type="range" id="upper_slider" name="upper_angle" min="0" max="180" value="%CURRENT_UPPERSERVO">
    </div>
    <div>
        Lower Servo:
        <input type="range" id="lower_slider" name="lower_angle" min="0" max="180" value="%CURRENT_LOWERSERVO">
    </div>
    <div>
        L Motor:
        <input type="range" id="l_motor_slider" name="l_motor" min="0" max="255" value="%CURRENT_L_MOTOR_SPEED">
    </div>
    <div>
        R Motor:
        <input type="range" id="r_motor_slider" name="r_motor" in="0" max="255" value="%CURRENT_R_MOTOR_SPEED">
    </div>
    <script>
      // For better manual control, remove if RAM issue
      "use strict";
      let deboune_timer;

      function update_servo(slider){
          const name = slider.name;
          const value = slider.value;
          
          // Update current angle display
          document.getElementById(name+"_display").innerText = value;
          // Handle Time out
          clearTimeout(deboune_timer);
          deboune_timer = setTimeout(() => {
              fetch(`/servo?${name}=${value}`)
              .then(res =>{
                  if(res.ok) console.log(`set ${name}=${value}`);
                  else console.error("server response error");
              }).catch(error => console.error(`Command update_servo ${name}=${value} error`,error));
          },50);
      }
      
      function update_motor(slider){
        const name = slider.name;
        const value = slider.value;
        
        // Update current speed display
        document.getElementById(name+"_display").innerText = value;
        // Handle Time out
        clearTimeout(deboune_timer);
        deboune_timer = setTimeout(() => {
            fetch(`/speed?${name}=${value}`)
            .then(res =>{
                if(res.ok) console.log(`set ${name}=${value}`);
                else console.error("server response error");
            }).catch(error => console.error(`Command update_servo ${name}=${value} error`,error));
        },50);
      }

      // Event Listener for servo control, remove if RAM issue
      document.getElementById('upper_slider').addEventListener('input', (event) => { update_servo(event.target);});
      document.getElementById('lower_slider').addEventListener('input', (event) => { update_servo(event.target);});
      // Event Listener for speed control
      document.getElementById('l_motor_slider').addEventListener('input', (event) => { update_motor(event.target);});
      document.getElementById('r_motor_slider').addEventListener('input', (event) => { update_motor(event.target);});
    </script>
</body>
</html>
)rawliteral";

// Control Function
void motor_direction(int dir);
void automatic_mode(); 
//Web Server Function
void handle_root();
void handle_action();
void handle_servo();
void handle_speed();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Wifi
  WiFi.begin(ssid, password);
  Serial.printf("Connecting to %s with the password ",ssid);
  for(int i=0;i<strlen(password);++i){
    Serial.printf("*");
  }
  Serial.println();
  
  
  Serial.printf("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf(".");
  }
  Serial.printf("\n");

  Serial.print("WIFI Connected,go to http://");
  Serial.println(WiFi.localIP());
  
  // pin setting
  #if L_MOTOR
  pinMode(PIN_IN1, OUTPUT); pinMode(PIN_IN3, OUTPUT);  pinMode(PIN_ENA, OUTPUT);
  #endif
  #if R_MOTOR
  pinMode(PIN_IN2, OUTPUT); pinMode(PIN_IN4, OUTPUT); pinMode(PIN_ENB,OUTPUT); 
  #endif

  #if UPPER_SERVO
  upper_servo.attach(PIN_SER_U);
  upper_servo.write(current_upper_angle); 
  #endif
  #if LOWER_SERVO
  lower_servo.attach(PIN_SER_L);
  lower_servo.write(current_lower_angle);
  #endif
  // Stop motor
  Serial.println("Stopping motor...");
  
  #if L_MOTOR
  analogWrite(PIN_ENA,0); 
  #endif

  #if R_MOTOR
  analogWrite(PIN_ENB,0);
  #endif
  
  // Status output
  Serial.printf("Current L Motor Speed %d\nCurrent R Motor Speed %d\n",current_speed_l,current_speed_r);
  Serial.printf("Current Lower Servo angle %d\nCurrent Upper Servo angle %d\n",current_lower_angle,current_upper_angle);
  //Webserver
  server.on("/", handle_root);
  server.on("/action", handle_action);
  server.on("/servo",handle_servo);
  server.on("/speed",handle_speed);
  server.begin(); 
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");

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
      Serial.println("Motor Break!");
      #if L_MOTOR
      analogWrite(PIN_ENA,0); 
      #endif
      #if R_MOTOR
      analogWrite(PIN_ENB,0);
      #endif
      motor_break = 1;
      return;
    }else{
      Serial.println("Motor resume");
      motor_break = 0;
      analogWrite(PIN_ENA,current_speed_l); analogWrite(PIN_ENB,current_speed_r);
      Serial.printf("Motor L Speed: %d Motor R Speed: %d\n",current_speed_l,current_speed_r);
      motor_direction(prev_action); 
      return;
    }
  case 1:
    Serial.println("Motor move forward");
    digitalWrite(PIN_IN1, HIGH); digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, HIGH); digitalWrite(PIN_IN4, LOW);
    break;
  case 2:
    Serial.println("Motor move left");
    digitalWrite(PIN_IN1, LOW); digitalWrite(PIN_IN2, HIGH);
    digitalWrite(PIN_IN3, HIGH); digitalWrite(PIN_IN4, LOW);
    break;
  case 3:
    Serial.println("Motor move backward");
    digitalWrite(PIN_IN1, LOW); digitalWrite(PIN_IN2, HIGH);
    digitalWrite(PIN_IN3, LOW); digitalWrite(PIN_IN4, HIGH);
    break;
  case 4:
    Serial.println("Motor move right");
    digitalWrite(PIN_IN1, HIGH); digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, LOW); digitalWrite(PIN_IN4, HIGH);
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

void handle_root() {
  String page = html_content;
  page.replace("%CURRENT_L_MOTOR_SPEED%",String(current_speed_l));
  page.replace("%CURRENT_R_MOTOR_SPEED%",String(current_speed_r));
  page.replace("%CURRENT_UPPERSERVO%",String(current_upper_angle));
  page.replace("%CURRENT_LOWERSERVO%",String(current_lower_angle));
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

void handle_speed(){
  #if L_MOTOR
  if(server.hasArg("l_motor")){
    int speed = server.arg("l_motor").toInt();
    speed = constrain(speed,0,255);
    analogWrite(PIN_ENA,speed);
    current_speed_l = speed;
    Serial.printf("Motor L Speed: %d\n",current_speed_l);
  }
  #endif
  #if R_MOTOR
  if(server.hasArg("r_motor")){
    int speed = server.arg("r_motor").toInt();
    speed = constrain(speed,0,255);
    analogWrite(PIN_ENB,speed);
    current_speed_r = speed;
    Serial.printf("Motor R Speed: %d\n",current_speed_r);    
  }
  #endif
  server.sendHeader("Location","/",true);
  server.send(302,"text/plain","");
}

void handle_servo(){
  #if UPPER_SERVO
  if(server.hasArg("upper_angle")){
    int ang = server.arg("upper_angle").toInt();
    ang = constrain(ang,0,180);
    upper_servo.write(ang);
    current_upper_angle = ang;
    Serial.printf("Upper Servo Angle: %d\n",current_upper_angle);
  }
  #endif
  #if LOWER_SERVO
  if(server.hasArg("lower_angle")){
    int ang = server.arg("lower_angle").toInt();
    ang = constrain(ang,0,180);
    lower_servo.write(ang);
    current_lower_angle = ang;
    Serial.printf("Lower Servo Angle: %d\n",current_lower_angle);
  }
  #endif
  server.sendHeader("Location","/",true);
  server.send(302,"text/plain","");
}