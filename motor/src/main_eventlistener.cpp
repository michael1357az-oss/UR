#include "Arduino.h"
#include "WebServer.h"
#include "WiFi.h"
#include "ESP32Servo.h"
#include "esp_wifi.h"

#define UPPER_SERVO false 
#define LOWER_SERVO true
#define L_MOTOR true
#define R_MOTOR true

// Motor Pin
#if L_MOTOR
const int PIN_IN1 = 14; 
const int PIN_IN2 = 15;
#endif

#if R_MOTOR 
const int PIN_IN3 = 13;
const int PIN_IN4 = 12;
#endif

// Servo Pin
#if UPPER_SERVO
const int PIN_SER_U = 3; 
#endif
#if LOWER_SERVO
const int PIN_SER_L = 2;
#endif
//wifi setting
const char *ssid = "UR-FallTrain";
const char *password = "falltrain";

// Webserver
WebServer server(80);

// Servo
Servo upper_servo;
Servo lower_servo;
int current_upper_angle = 90,current_lower_angle = 90;

// Motor control
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
    button { width: 160px; height: 120px; font-size: 16px; }
    .stop-btn { background-color:#ff4d4d; width:225px; margin-top:10px; } 
  </style>
</head>
<body>
    <h1>RC Mode</h1>
    <table>
        <tr>
            <td></td>
            <td><button onclick="handle_dir(1)">UP</button></td>
            <td></td>
        </tr>
        <tr>
            <td><button onclick="handle_dir(2)">LEFT</button></td>
            <td></td> <td><button onclick="handle_dir(4)">RIGHT</button></td>
        </tr>
        <tr>
            <td></td>
            <td><button onclick="handle_dir(3)">DOWN</button></td>
            <td></td>
        </tr>
    </table>
    <div>
        <button class="stop-btn" id=stop_btn onclick="handle_dir(0)">BREAK/RESUME</button>
    </div>
    <hr>
    <div>Current Lower Servo angle:<strong id="lower_angle_display">%CURRENT_LOWERSERVO%</strong></div>
    <div>
        Lower Servo:
        <input type="range" id="lower_slider" name="lower_angle" min="0" max="180" value="%CURRENT_LOWERSERVO">
    </div>
    <div>
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

      function handle_dir(dir){
        clearTimeout(deboune_timer);
        deboune_timer = setTimeout(()=>{
            fetch(`/action?dir=${dir}`) 
            .then(res => {
                if(res.ok) console.log(`set dir to ${dir}`);
                else console.error("server response error");
            }).catch(error => console.error(`Command update dir to ${dir}`,error));
        })
      }
      
      // Event Listener for servo control, remove if RAM issue
      document.getElementById('lower_slider').addEventListener('input', (event) => { update_servo(event.target);});
      document.getElementById('stop_btn').addEventListener('keypress', (event) => { if(event.key == " ") handle_dir(0); });
    </script>
</body>
</html>
)rawliteral";

// Control Function
void motor_direction(int dir);
//Web Server Function
void handle_root();
void handle_action();
void handle_servo();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Wifi
  WiFi.begin(ssid, password);
  Serial.printf("Connecting to %s with the password ",ssid);
  for(int i=0;i<strlen(password);++i)  Serial.printf("*");
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
  pinMode(PIN_IN1, OUTPUT); pinMode(PIN_IN2, OUTPUT);  
  #endif
  #if R_MOTOR
  pinMode(PIN_IN3, OUTPUT); pinMode(PIN_IN4, OUTPUT);  
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
  digitalWrite(PIN_IN1, LOW); digitalWrite(PIN_IN2, LOW);
  #endif
  #if R_MOTOR
  digitalWrite(PIN_IN3, LOW); digitalWrite(PIN_IN4, LOW);
  #endif
  
  // Status output
  Serial.printf("Current Lower Servo angle %d\n",current_lower_angle);
  // Serial.printf("Current Lower Servo angle %d\nCurrent Upper Servo angle %d\n",current_lower_angle,current_upper_angle);
  //Webserver
  server.on("/", handle_root);
  server.on("/action", handle_action);
  server.on("/servo",handle_servo);
  server.begin(); 
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");

  Serial.println("init complete");
}


void loop() {
  server.handleClient();
  delay(5);
}


void motor_direction(int dir) {
  dir = constrain(dir, 0, 4);
  // when dir = 0, stop
  switch (dir) {
  case 0:
    if(!motor_break){
      Serial.println("Motor Break!");
      #if L_MOTOR
      digitalWrite(PIN_IN1, LOW); digitalWrite(PIN_IN2, LOW);
      #endif
      #if R_MOTOR
      digitalWrite(PIN_IN3, LOW); digitalWrite(PIN_IN4, LOW);
      #endif
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
    #if L_MOTOR
    digitalWrite(PIN_IN1, HIGH); digitalWrite(PIN_IN2, LOW);
    #endif
    #if R_MOTOR
    digitalWrite(PIN_IN3, HIGH); digitalWrite(PIN_IN4, LOW);
    #endif
    break;
  case 2:
    Serial.println("Motor move left");
    #if L_MOTOR
    digitalWrite(PIN_IN1, HIGH); digitalWrite(PIN_IN2, LOW);
    #endif
    #if R_MOTOR
    digitalWrite(PIN_IN3, LOW); digitalWrite(PIN_IN4, LOW);
    #endif
    break;
  case 3:
    Serial.println("Motor move backward");
    #if L_MOTOR
    digitalWrite(PIN_IN1, LOW); digitalWrite(PIN_IN2, HIGH);
    #endif
    #if R_MOTOR
    digitalWrite(PIN_IN3, LOW); digitalWrite(PIN_IN4, HIGH);
    #endif
    break;
  case 4:
    Serial.println("Motor move right");
    #if L_MOTOR
    digitalWrite(PIN_IN1, LOW); digitalWrite(PIN_IN2, LOW);
    #endif
    #if R_MOTOR
    digitalWrite(PIN_IN3, HIGH); digitalWrite(PIN_IN4, LOW);
    #endif
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
  //page.replace("%CURRENT_UPPERSERVO%",String(current_upper_angle));
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