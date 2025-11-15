//könyvtárak
#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include <Wire.h>
#include <esp32-hal-ledc.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>

void WebTask(void* pvParameters);
void startTask1(void* parameter);
void startTask2(void* parameter);
void startTask3(void* parameter);
void startTask4(void* parameter);
void startTask5(void* parameter);
void startTask6(void* parameter);
void startTask7(void* parameter);

// MCP23017 objektum
Adafruit_MCP23X17 mcp; // I²C portbővítő objektum
//sensor, motorok

// PWM-beállítások
const uint32_t freq       = 1000;  // Hz
const uint8_t  resolution = 8;      // bitek (1–14)
const int      pwmChannel_0 = 0;      //csatornák
const int      pwmChannel_1 = 1;
const int      pwmChannel_2 = 2;
const int      pwmChannel_3 = 3;
// Motor1 ajtó
const int motor1_in1 = 12;
const int motor1_in2 = 13;
//Motor2 futószalag 
const int motor2_enable=19;
const int motor2_in1 = 32;
const int motor2_in2 = 33;
//Motor3 csapóajtó
const int motor3_enable=17;
const int motor3_in1 = 16;
const int motor3_in2 = 4;
//Motor4 lift
const int motor4_enable = 14;
const int motor4_in1 = 27;
const int motor4_in2 = 26;
//Motor5 toló
const int motor5_enable = 18;
const int motor5_in1 = 2;
const int motor5_in2 = 15;
//ventillátor
const int motor6_in1=23;
const int motor6_in2=25;
//Temperature Sensor
const int Temperature_sensor=35;

//MCP23017:

//Leds
const int start_leds = 7; 
const int stop_leds = 12;
//sensor
const int sensor_door = 10;
const int sensor_trapdoor = 11;
const int sensor_szalag = 13;
const int sensor_lift_lent = 14;
const int sensor_lift_fent = 15;
// Globális szenzor állapotok
volatile bool value_door = LOW;
volatile bool value_trapdoor = LOW;
volatile bool value_szalag = LOW;
volatile bool value_lift_lent = LOW;
volatile bool value_lift_fent = LOW;
//Press button start
const int Press_button_start = 8; 
int Press_button_start_state=0;
//Press button stop
const int Press_button_stop=9;
int Press_button_stop_state=0;

//számláló
const int A = 0;
const int B = 1;
const int C = 2;
const int D = 3;
const int E = 4;
const int F = 5;
const int G = 6;
volatile int counterValue = 0;
bool prev_sensor_state = LOW;

//Triggerelés
volatile bool start = false;
volatile bool sensor_szalag_Triggered = false;
volatile bool stop = false;
volatile bool sensor_lift_Triggered = false;
volatile bool sensor_trapdoor_Triggered = false;
volatile bool START =false;
volatile bool STOP = false;
volatile bool state_of_buttons=false;

volatile bool PUSHER =false;  
volatile bool DOOR =true;
volatile bool TRAPDOOR =false;
volatile bool CONVEYOR =false;
volatile bool LIFT =false;
volatile bool PUSHER_motor =false;  
volatile bool DOOR_motor =false;
volatile bool TRAPDOOR_motor =false;
volatile bool CONVEYOR_motor =false;
volatile bool LIFT_motor =false;
volatile float Temperature = 0.0;
volatile bool Reset_motors=false;
volatile bool ventilator_state=false;
//WiFi connection
const char* ssid     = "Family Network";
const char* password = "opel4ever";

WebServer server(80);
// --- HTTP kezelők ---
void handleRoot() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="hu">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Szakdolgozat</title>
  <style>
    body {
      height:100vh;
      margin:0;
      background:linear-gradient(to bottom, #784901c1 25%, #fff 10%);
      font-family: Arial, sans-serif;
      text-align:center;
    }
    .container { margin-top:40px; }
    .container h1 { font-size:70px; margin-bottom:20px; }
    .container hr { width:60%; border:none; height:13px; background:black; margin:20px auto; }
    .buttons { margin-top:30px; }
    .button {
      font-size:60px; border-radius:20px; border:none;
      color:white; padding:20px 40px; cursor:pointer; margin:20px;
      transition:all 0.1s ease-in-out; box-shadow:0 6px #999;
    }
    .button:active {transform:scale(0.95); box-shadow:0 3px #666;}
    .node-btn {fill:#fff; stroke:#0d2630; stroke-width:3; cursor:pointer; transition: all 0.2s ease-in-out;}
    .node-btn:hover{fill:#d4edff;}
    .node-label{font-size:16px; fill:#0d2630; font-weight:600; cursor:pointer; user-select:none;}
    .node-btn.active {fill:red;}
    .node-btn.sensor-active {fill: yellow;}
    .node-btn.active.sensor-active {fill: url(#redYellowHalf);} /* piros-sárga felezett */
    #startBtn { background-color:green; }
    #stopBtn { background-color:rgba(192,2,2,0.893); }
    #startBtn.active { background-color:rgb(54,243,54)!important; }
    #stopBtn.active { background-color:rgb(251,101,101)!important; }
    .counter-wrap {display:flex; justify-content:center; align-items:center; gap:40px; margin-top:40px;}
    #cnt {font-size:60px; color:black;}
    .wrap{ width:min(500px, 40vw); }
    svg{ width:100%; height:auto; display:block; }
    .edge{ stroke:#0d2630; stroke-width:3; fill:none; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Szakdolgozat</h1>
    <hr>
    <div class="buttons">
      <button id="startBtn" class="button">START</button>
      <button id="stopBtn" class="button">STOP</button>
    </div>

    <div class="counter-wrap">
      <div style="display: flex; flex-direction: column; align-items: center;">
        <div id="cnt" style="font-size:60px; color:black;">Számláló:0</div>
        <div id="temp" style="font-size: 60px; color: black; margin-top: 20px;">Hőmérséklet:</div>
      </div>
      <div class="wrap">
        <svg viewBox="0 0 900 420">     

          <defs>
            <!-- nyílhegy -->
            <marker id="arrow" markerWidth="14" markerHeight="10" refX="9" refY="5"
                    orient="auto" markerUnits="strokeWidth">
              <path d="M0,0 L0,10 L14,5 Z" fill="#0d2630"/>
            </marker>

            <!-- piros-sárga felezett kitöltés -->
            <linearGradient id="redYellowHalf" x1="0%" y1="0%" x2="100%" y2="0%">
              <stop offset="50%" style="stop-color:red; stop-opacity:1" />
              <stop offset="50%" style="stop-color:yellow; stop-opacity:1" />
            </linearGradient>
          </defs>

          <!-- Csomópontok gombként -->
          <g class="btn" onclick="nodeClick('pusher')">
            <circle id="node-pusher" class="node-btn" cx="120" cy="70" r="15"/>
            <text class="node-label" x="28" y="76">PUSHER</text>
          </g>

          <g class="btn" onclick="nodeClick('door')">
            <circle id="node-door" class="node-btn" cx="300" cy="70" r="15"/>
            <text class="node-label" x="308" y="48">DOOR</text>
          </g>

          <g class="btn" onclick="nodeClick('trapdoor')">
            <circle id="node-trapdoor" class="node-btn" cx="720" cy="150" r="15"/>
            <text class="node-label" x="736" y="156">TRAPDOOR</text>
          </g>

          <g class="btn" onclick="nodeClick('conveyor')">
            <circle id="node-conveyor" class="node-btn" cx="720" cy="330" r="15"/>
            <text class="node-label" x="740" y="336">CONVEYOR BELT</text>
          </g>

          <g class="btn" onclick="nodeClick('lift')">
            <circle id="node-lift" class="node-btn" cx="120" cy="330" r="15"/>
            <text class="node-label" x="70" y="372">LIFT</text>
          </g>

          <!-- Élek -->
          <line class="edge" x1="135" y1="70"  x2="285" y2="70"  marker-end="url(#arrow)"/>
          <line class="edge" x1="315" y1="75"  x2="705" y2="145" marker-end="url(#arrow)"/>
          <line class="edge" x1="720" y1="165" x2="720" y2="315" marker-end="url(#arrow)"/>
          <line class="edge" x1="705" y1="330" x2="135" y2="330" marker-end="url(#arrow)"/>
          <line class="edge" x1="120" y1="315" x2="120" y2="85"  marker-end="url(#arrow)"/>
        </svg>
      </div>
    </div>
  </div>

  <script>
    function updateCounter() {
      fetch('/value')
        .then(r => r.text())
        .then(t => document.getElementById('cnt').innerText = "Számláló: " + t);
    }

    function updateTemp() {
      fetch('/temperature')
        .then(r => r.text())
        .then(t => document.getElementById('temp').innerText = "Hőmérséklet: " + t);
    }

    // LED státusz lekérdezése és gomb színek
    function updateBtns() {
      fetch('/status')
        .then(r => r.text())
        .then(st => {
          document.getElementById('startBtn').classList.toggle('active', st==='ON');
          document.getElementById('stopBtn').classList.toggle('active', st==='OFF');
        });
    }

    // Csomópont kattintás – mindig piros marad
    function nodeClick(name) {
      fetch("/" + name); // szerver értesítése
      let node = document.getElementById("node-" + name);
      if (node && !node.classList.contains("active")) {
        node.classList.add("active");
      }
    }

    // Szenzor állapot frissítés
    function updateSensors() {
      fetch('/sensors')
      .then(r => r.json())
      .then(data => {
        for (let key in data) {
          let node = document.getElementById("node-" + key);
          if (node) {
            if (data[key] == 1) {
              node.classList.add("sensor-active");
            } else {
              node.classList.remove("sensor-active");
            }
          }
        }
      });
    }

    // RESET ellenőrzése
    function updateReset() {
      fetch('/reset_motors')
        .then(r => r.text())
        .then(state => {
          if (state.trim() === "true") {
            document.querySelectorAll(".node-btn.active")
                    .forEach(el => el.classList.remove("active"));
          }
        });
    }

    // Start / Stop gomb események
    document.getElementById('startBtn').onclick = () => fetch('/on').then(updateBtns);
    document.getElementById('stopBtn').onclick = () => fetch('/off').then(updateBtns);

    // Oldal betöltéskor frissítés
    updateCounter();
    updateSensors();
    updateBtns();
    updateTemp();
    setInterval(updateCounter, 500);
    setInterval(updateSensors, 500);
    setInterval(updateBtns, 500);
    setInterval(updateReset, 500);
    setInterval(updateTemp, 500);
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", page);
}

// A számláló értéke
void handleValue() {
  server.send(200, "text/plain", String(counterValue));
}

// Start
void handleOn() {
  START=true;
  state_of_buttons = true;
  server.send(200, "text/plain", "OK");
}

// Stop
void handleOff() {
  STOP=true;
  state_of_buttons = false;
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  server.send(200, "text/plain", state_of_buttons ? "ON" : "OFF");
}

void handleReset_motors() {
  if (Reset_motors) {
    server.send(200, "text/plain", "true");
    Reset_motors = false;  
  } else {
    server.send(200, "text/plain", "false");
  }
}
void handlePusher() {
  PUSHER_motor =true;
  ledcWriteChannel(pwmChannel_3, 0);
  digitalWrite(motor5_in1, LOW);
  digitalWrite(motor5_in2, LOW);
  server.send(200, "text/plain", "pusher");
}

void handleDoor() {
  DOOR_motor=true;
  digitalWrite(motor1_in1, LOW);
  digitalWrite(motor1_in2, LOW);
  server.send(200, "text/plain", "door");
}

void handleTrapdoor() {
  TRAPDOOR_motor=true;
  ledcWriteChannel(pwmChannel_1, 0);
  digitalWrite(motor3_in1, LOW);
  digitalWrite(motor3_in2, LOW);
  server.send(200, "text/plain", "trapdoor");
}

void handleConveyor() {
  CONVEYOR_motor=true;
  ledcWriteChannel(pwmChannel_0, 0);
  digitalWrite(motor2_in1, LOW);
  digitalWrite(motor2_in2, LOW);
  server.send(200, "text/plain", "conveyor");
}

void handleLift() {
  LIFT_motor=true;
  ledcWriteChannel(pwmChannel_2, 0);
  digitalWrite(motor4_in1, LOW);
  digitalWrite(motor4_in2, LOW); 
  server.send(200, "text/plain", "lift");
}

void handleSensors() {
  String json = "{";
  json += "\"pusher\":" + String(PUSHER) + ",";
  json += "\"door\":" + String(DOOR) + ",";
  json += "\"trapdoor\":" + String(TRAPDOOR) + ",";
  json += "\"conveyor\":" + String(CONVEYOR) + ",";
  json += "\"lift\":" + String(LIFT);
  json += "}";
  server.send(200, "application/json", json);
}

void handleTemperature() {
  // Két tizedesjegyre formázza a számot és hozzáteszi a " °C"-t
  String temperature = String(Temperature, 2) + " °C";
  server.send(200, "text/plain", temperature);
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // MCP23017 I²C címe 
  Wire.begin(21, 22);
  mcp.begin_I2C(0x20);
  //WiFi connection
  WiFi.begin(ssid, password);
  Serial.print("Kapcsolódás: ");
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500/ portTICK_PERIOD_MS);   
    Serial.print(".");
  }
  Serial.println("\nCsatlakozva!");
  Serial.print("IP cím: ");
  MDNS.begin("szakdolgozat"); // a címem http://szakdolgozat.local/
  Serial.println(WiFi.localIP());
  //Motorok
  pinMode(motor1_in1, OUTPUT);
  pinMode(motor1_in2, OUTPUT);
  pinMode(motor2_enable, OUTPUT);
  pinMode(motor2_in1, OUTPUT);
  pinMode(motor2_in2, OUTPUT);
  pinMode(motor3_in1, OUTPUT);
  pinMode(motor3_in2, OUTPUT);
  pinMode(motor3_enable,OUTPUT);
  pinMode(motor4_enable, OUTPUT);
  pinMode(motor4_in1, OUTPUT);
  pinMode(motor4_in2, OUTPUT);
  pinMode(motor5_enable, OUTPUT);
  pinMode(motor5_in1, OUTPUT);
  pinMode(motor5_in2, OUTPUT);
  pinMode(motor6_in1, OUTPUT);
  pinMode(motor6_in2, OUTPUT);  
  ledcAttachChannel(motor2_enable, freq, resolution, pwmChannel_0);
  ledcAttachChannel(motor3_enable, freq, resolution, pwmChannel_1);
  ledcAttachChannel(motor4_enable, freq, resolution, pwmChannel_2);
  ledcAttachChannel(motor5_enable, freq, resolution, pwmChannel_3);
  //Temperature_Sensor
  pinMode(Temperature_sensor,INPUT);
  //MCP23017: 
  mcp.pinMode(start_leds, OUTPUT);
  mcp.pinMode(stop_leds, OUTPUT);
  mcp.pinMode(Press_button_start, INPUT_PULLUP);
  mcp.pinMode(Press_button_stop, INPUT_PULLUP);
  mcp.pinMode(sensor_door, INPUT_PULLUP);
  mcp.pinMode(sensor_trapdoor, INPUT_PULLUP);
  mcp.pinMode(sensor_szalag, INPUT_PULLUP);
  mcp.pinMode(sensor_lift_lent, INPUT_PULLUP);
  mcp.pinMode(sensor_lift_fent, INPUT_PULLUP);
  mcp.pinMode(A, OUTPUT);
  mcp.pinMode(B, OUTPUT);
  mcp.pinMode(C, OUTPUT);
  mcp.pinMode(D, OUTPUT);
  mcp.pinMode(E, OUTPUT);
  mcp.pinMode(F, OUTPUT);
  mcp.pinMode(G, OUTPUT); 
  // HTTP útvonalak
  server.on("/",       HTTP_GET, handleRoot);
  server.on("/value",  HTTP_GET, handleValue);
  server.on("/on",     HTTP_GET, handleOn);
  server.on("/off",    HTTP_GET, handleOff);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/pusher", handlePusher);
  server.on("/door", handleDoor);
  server.on("/trapdoor", handleTrapdoor);
  server.on("/conveyor", handleConveyor);
  server.on("/lift", handleLift);
  server.on("/sensors", handleSensors);
  server.on("/reset_motors", HTTP_GET, handleReset_motors);
  server.on("/temperature", HTTP_GET, handleTemperature);
  server.begin();
  //Taskok
  xTaskCreatePinnedToCore(WebTask, "WebTask", 4096, NULL, 1, NULL,0);
  xTaskCreatePinnedToCore(startTask1, "Task_1", 2028, NULL, 2, NULL,1);
  xTaskCreatePinnedToCore(startTask2, "Task_2", 2028, NULL, 2, NULL,1);
  xTaskCreatePinnedToCore(startTask3, "Task_3", 2028, NULL, 2, NULL,1);
  xTaskCreatePinnedToCore(startTask4, "Task_4", 2028, NULL, 2, NULL,1);
  xTaskCreatePinnedToCore(startTask5, "Task_5", 2028, NULL, 2, NULL,1);
  xTaskCreatePinnedToCore(startTask6, "Task_6", 2028, NULL, 2, NULL,1);
  xTaskCreatePinnedToCore(startTask7, "Task_7", 2028, NULL, 2, NULL,1); 
  //Counter 0
  mcp.digitalWrite(A, HIGH);
  mcp.digitalWrite(B, HIGH);
  mcp.digitalWrite(C, HIGH);
  mcp.digitalWrite(D, HIGH);
  mcp.digitalWrite(E, HIGH);
  mcp.digitalWrite(F, HIGH);
  mcp.digitalWrite(G, LOW);  
  //start, stop LED LOW
  mcp.digitalWrite(start_leds, LOW);
  mcp.digitalWrite(stop_leds, LOW);
}
// ajtó kinyitás
void door(){
  if(DOOR_motor==false){
    if(DOOR_motor){
      digitalWrite(motor1_in1, LOW);
      digitalWrite(motor1_in2, LOW);
      return;
    }
    digitalWrite(motor1_in1, LOW);
    digitalWrite(motor1_in2, HIGH);
    vTaskDelay(100/ portTICK_PERIOD_MS);
    digitalWrite(motor1_in1, LOW);
    digitalWrite(motor1_in2, LOW);
  } else{
    digitalWrite(motor1_in1, LOW);
    digitalWrite(motor1_in2, LOW);
  }
}

// ajtó zárás
void door_close(){
  if(DOOR_motor==false){
    if(DOOR_motor){
      digitalWrite(motor1_in1, LOW);
      digitalWrite(motor1_in2, LOW);
      return;
    }
    digitalWrite(motor1_in1, HIGH);
    digitalWrite(motor1_in2, LOW);
    vTaskDelay(220/ portTICK_PERIOD_MS);
    digitalWrite(motor1_in1, LOW);
    digitalWrite(motor1_in2, LOW);
  } else{
    digitalWrite(motor1_in1, LOW);
    digitalWrite(motor1_in2, LOW);
  }  
}

//trapdoor le
void trapdoor_le(){
  if(TRAPDOOR_motor==false){
    if(TRAPDOOR_motor){
      ledcWriteChannel(pwmChannel_1, 0);
      digitalWrite(motor3_in1, LOW);
      digitalWrite(motor3_in2, LOW);   
      return;   
    }
    ledcWriteChannel(pwmChannel_1, 30);
    digitalWrite(motor3_in1, LOW);
    digitalWrite(motor3_in2, HIGH);
  } else{
    ledcWriteChannel(pwmChannel_1, 0);
    digitalWrite(motor3_in1, LOW);
    digitalWrite(motor3_in2, LOW);
  }
}

//trapdoor fel
void trapdoor_fel(){
  if(TRAPDOOR_motor==false){
    if(TRAPDOOR_motor){
      ledcWriteChannel(pwmChannel_1, 0);
      digitalWrite(motor3_in1, LOW);
      digitalWrite(motor3_in2, LOW);   
      return;   
    }    
    ledcWriteChannel(pwmChannel_1, 30);
    digitalWrite(motor3_in1, HIGH);
    digitalWrite(motor3_in2, LOW);
  } else{
    ledcWriteChannel(pwmChannel_1, 0);
    digitalWrite(motor3_in1, LOW);
    digitalWrite(motor3_in2, LOW);
  }
}

//szalag be
void szalag_be(){
  if(CONVEYOR_motor ==false){
    for (int n=50; n<160; n++){
      if (CONVEYOR_motor) {
        ledcWriteChannel(pwmChannel_0, 0);
        digitalWrite(motor2_in1, LOW);
        digitalWrite(motor2_in2, LOW);
        return; 
      }
      ledcWriteChannel(pwmChannel_0, n);
      digitalWrite(motor2_in1, HIGH);
      digitalWrite(motor2_in2, LOW);
      vTaskDelay(pdMS_TO_TICKS(11));
    } 
  } else{
    szalag_ki();
  }
}

//szalag ki
void szalag_ki(){
  ledcWriteChannel(pwmChannel_0, 0);
  digitalWrite(motor2_in1, LOW);
  digitalWrite(motor2_in2, LOW);
}

//Lift le 
void lift_le(){
  if(LIFT_motor ==false){
    for (int k=20; k<30; k++){
      if(LIFT_motor){
        ledcWriteChannel(pwmChannel_2, 0);
        digitalWrite(motor4_in1, LOW);
        digitalWrite(motor4_in2, LOW);        
        return;
      }
      ledcWriteChannel(pwmChannel_2, k);
      digitalWrite(motor4_in1, HIGH);
      digitalWrite(motor4_in2, LOW);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  } else{
    ledcWriteChannel(pwmChannel_2, 0);
    digitalWrite(motor4_in1, LOW);
    digitalWrite(motor4_in2, LOW);
  }
}

//lift fel
void lift_fel(){
  if(LIFT_motor ==false){
    for (int i=50; i<140; i++){
      if(LIFT_motor){
        ledcWriteChannel(pwmChannel_2, 0);
        digitalWrite(motor4_in1, LOW);
        digitalWrite(motor4_in2, LOW);        
        return;
      }      
      ledcWriteChannel(pwmChannel_2, i);
      digitalWrite(motor4_in1, LOW);
      digitalWrite(motor4_in2, HIGH);
      vTaskDelay(pdMS_TO_TICKS(18));
      if(i==139){
        ledcWriteChannel(pwmChannel_2, 95);
      }
    }
  } else{
    ledcWriteChannel(pwmChannel_2, 0);
    digitalWrite(motor4_in1, LOW);
    digitalWrite(motor4_in2, LOW);
  }
}

//toló be
void pusher_be(){ 
  if(PUSHER_motor ==false){
    if(PUSHER_motor){
      ledcWriteChannel(pwmChannel_3, 0);
      digitalWrite(motor5_in1, LOW);
      digitalWrite(motor5_in2, LOW);
      return;      
    }
    ledcWriteChannel(pwmChannel_3, 200);
    digitalWrite(motor5_in1, LOW);
    digitalWrite(motor5_in2, HIGH);
    vTaskDelay(150/ portTICK_PERIOD_MS);
    ledcWriteChannel(pwmChannel_3, 255);
    digitalWrite(motor5_in1, HIGH);
    digitalWrite(motor5_in2, LOW);
    vTaskDelay(400/ portTICK_PERIOD_MS);
    ledcWriteChannel(pwmChannel_3, 0);
    digitalWrite(motor5_in1, LOW);
    digitalWrite(motor5_in2, LOW);
  } else{
    ledcWriteChannel(pwmChannel_3, 0);
    digitalWrite(motor5_in1, LOW);
    digitalWrite(motor5_in2, LOW);    
  }
}

//toló ki
void pusher_ki(){
  if(PUSHER_motor ==false){
    if(PUSHER_motor){
      ledcWriteChannel(pwmChannel_3, 0);
      digitalWrite(motor5_in1, LOW);
      digitalWrite(motor5_in2, LOW);
      return;      
    }
    ledcWriteChannel(pwmChannel_3, 35);
    digitalWrite(motor5_in1, LOW);
    digitalWrite(motor5_in2, HIGH);
    vTaskDelay(35/ portTICK_PERIOD_MS);   
    ledcWriteChannel(pwmChannel_3, 0);
    digitalWrite(motor5_in1, LOW);
    digitalWrite(motor5_in2, LOW); 
  } else{
    ledcWriteChannel(pwmChannel_3, 0);
    digitalWrite(motor5_in1, LOW);
    digitalWrite(motor5_in2, LOW);        
  }
}

//ventilátor be
void ventilator_be(){
  digitalWrite(motor6_in1, HIGH);
  digitalWrite(motor6_in2, LOW);  
}
//ventilátor ki
void ventilator_ki(){
  digitalWrite(motor6_in1, LOW);
  digitalWrite(motor6_in2, LOW);  
}

//számláló
void counter(){
 
  if (value_door == HIGH && prev_sensor_state == LOW) {
    counterValue++;
    if(start==false&& stop==false){
      counterValue = 0;
    }
    if (counterValue > 10) {
      counterValue = 1;
    }
    displayNumber(counterValue);
    vTaskDelay(100/ portTICK_PERIOD_MS);   

  }
  if (value_door == LOW) {
    prev_sensor_state = LOW;
  }  else {
    prev_sensor_state = HIGH;
  }
}
// számok kiírása
void displayNumber(int number) {

 switch (number) {
    case 0:
      mcp.digitalWrite(A, HIGH);
      mcp.digitalWrite(B, HIGH);
      mcp.digitalWrite(C, HIGH);
      mcp.digitalWrite(D, HIGH);
      mcp.digitalWrite(E, HIGH);
      mcp.digitalWrite(F, HIGH);
      mcp.digitalWrite(G, LOW);   
      break;
    case 1:
      mcp.digitalWrite(A, LOW);
      mcp.digitalWrite(B, LOW);
      mcp.digitalWrite(C, LOW);
      mcp.digitalWrite(D, LOW);
      mcp.digitalWrite(E, HIGH);
      mcp.digitalWrite(F, HIGH);
      mcp.digitalWrite(G, LOW);
      break;
    
    case 2:
      mcp.digitalWrite(A, HIGH);
      mcp.digitalWrite(B, HIGH);
      mcp.digitalWrite(C, LOW);
      mcp.digitalWrite(D, HIGH);
      mcp.digitalWrite(E, HIGH);
      mcp.digitalWrite(F, LOW);
      mcp.digitalWrite(G, HIGH);
      break;

    case 3:
      mcp.digitalWrite(A, HIGH);
      mcp.digitalWrite(B, HIGH);
      mcp.digitalWrite(C, HIGH);
      mcp.digitalWrite(D, HIGH);
      mcp.digitalWrite(E, LOW);
      mcp.digitalWrite(F, LOW);
      mcp.digitalWrite(G, HIGH); 
      break;
    
    case 4:
      mcp.digitalWrite(A, LOW);
      mcp.digitalWrite(B, HIGH);
      mcp.digitalWrite(C, HIGH);
      mcp.digitalWrite(D, LOW);
      mcp.digitalWrite(E, LOW);
      mcp.digitalWrite(F, HIGH);
      mcp.digitalWrite(G, HIGH);  
      break;

    case 5:
      mcp.digitalWrite(A, HIGH);
      mcp.digitalWrite(B, LOW);
      mcp.digitalWrite(C, HIGH);
      mcp.digitalWrite(D, HIGH);
      mcp.digitalWrite(E, LOW);
      mcp.digitalWrite(F, HIGH);
      mcp.digitalWrite(G, HIGH);
      break;

    case 6:
      mcp.digitalWrite(A, HIGH);
      mcp.digitalWrite(B, LOW);
      mcp.digitalWrite(C, HIGH);
      mcp.digitalWrite(D, HIGH);
      mcp.digitalWrite(E, HIGH);
      mcp.digitalWrite(F, HIGH);
      mcp.digitalWrite(G, HIGH);
      break;

    case 7:
      mcp.digitalWrite(A, HIGH);
      mcp.digitalWrite(B, HIGH);
      mcp.digitalWrite(C, HIGH);
      mcp.digitalWrite(D, LOW);
      mcp.digitalWrite(E, LOW);
      mcp.digitalWrite(F, LOW);
      mcp.digitalWrite(G, LOW);
      break;

    case 8:
      mcp.digitalWrite(A, HIGH);
      mcp.digitalWrite(B, HIGH);
      mcp.digitalWrite(C, HIGH);
      mcp.digitalWrite(D, HIGH);
      mcp.digitalWrite(E, HIGH);
      mcp.digitalWrite(F, HIGH);
      mcp.digitalWrite(G, HIGH);
      break;

    case 9:
      mcp.digitalWrite(A, HIGH);
      mcp.digitalWrite(B, HIGH);
      mcp.digitalWrite(C, HIGH);
      mcp.digitalWrite(D, HIGH);
      mcp.digitalWrite(E, LOW);
      mcp.digitalWrite(F, HIGH);
      mcp.digitalWrite(G, HIGH);
      break;
    
    case 10:
      displayNumber(0);
      break;
  }
}

void stopAll(){
  ledcWriteChannel(pwmChannel_0, 0);
  ledcWriteChannel(pwmChannel_1, 0);
  ledcWriteChannel(pwmChannel_2, 0);
  ledcWriteChannel(pwmChannel_3, 0);
  digitalWrite(motor1_in1, LOW);
  digitalWrite(motor1_in2, LOW);
  digitalWrite(motor2_in1, LOW);
  digitalWrite(motor2_in2, LOW);
  digitalWrite(motor3_in1, LOW);
  digitalWrite(motor3_in2, LOW);
  digitalWrite(motor4_in1, LOW);
  digitalWrite(motor4_in2, LOW);
  digitalWrite(motor5_in1, LOW);
  digitalWrite(motor5_in2, LOW);
  mcp.digitalWrite(A, LOW);
  mcp.digitalWrite(B, LOW);
  mcp.digitalWrite(C, LOW);
  mcp.digitalWrite(D, LOW);
  mcp.digitalWrite(E, LOW);
  mcp.digitalWrite(F, LOW);
  mcp.digitalWrite(G, LOW);
  mcp.digitalWrite(start_leds, LOW);
  ventilator_ki();
  PUSHER=false;
  DOOR=true;
  TRAPDOOR=false;
  CONVEYOR=false;
  LIFT=false;
  start=false;
  stop = false;  
  state_of_buttons=false;
  PUSHER_motor =false;  
  DOOR_motor =false;
  TRAPDOOR_motor =false;
  CONVEYOR_motor =false;
  LIFT_motor =false;
  ventilator_state=false;
  Reset_motors = true;
  mcp.digitalWrite(stop_leds, LOW);
}

void loop() {
  value_door = mcp.digitalRead(sensor_door);
  value_trapdoor = mcp.digitalRead(sensor_trapdoor);
  value_szalag = mcp.digitalRead(sensor_szalag);
  value_lift_lent = mcp.digitalRead(sensor_lift_lent);
  value_lift_fent = mcp.digitalRead(sensor_lift_fent);
  Press_button_stop_state=mcp.digitalRead(Press_button_stop);
  Press_button_start_state=mcp.digitalRead(Press_button_start);
  // Számláló
  counter();
  vTaskDelay(pdMS_TO_TICKS(10));
}

// WebTask: Core 0-on fut, kezeli a HTTP-kliens kéréseit
void WebTask(void* pvParameters) {
  for (;;) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
//Minden mást kezel
void startTask1(void *parameter) {
  for(;;){
    //Start
    if (Press_button_start_state == HIGH || START){
    start = true;
    stop=false;
    state_of_buttons=true;
    Reset_motors=true;
    PUSHER =false;  
    DOOR =true;
    TRAPDOOR =false;
    CONVEYOR =false;
    LIFT =false;
    START=false;
    mcp.digitalWrite(start_leds, HIGH);
    mcp.digitalWrite(stop_leds, LOW);
    counterValue = 0;
    displayNumber(counterValue); 
    // Ajtó kinyílik és úgy marad, első állomás
    door(); 
    }
    // Stop
    if (Press_button_stop_state == HIGH || STOP){
      stop = true;
      state_of_buttons=false;
      mcp.digitalWrite(stop_leds, HIGH);
      mcp.digitalWrite(start_leds, LOW); 
      STOP=false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
//Lift lemegy, trapdoor lehajtódik
void startTask2(void *parameter) {
  for(;;){
    if(start){
      if(value_trapdoor==HIGH){
        sensor_trapdoor_Triggered = true;
        lift_le();
      }
      if(value_lift_lent == HIGH && sensor_trapdoor_Triggered){
        ledcWriteChannel(pwmChannel_2, 30);
        digitalWrite(motor4_in1, LOW);
        digitalWrite(motor4_in2, HIGH);     
        vTaskDelay(150/ portTICK_PERIOD_MS);
        ledcWriteChannel(pwmChannel_2, 0);
        digitalWrite(motor4_in1, LOW);
        digitalWrite(motor4_in2, LOW);     
        sensor_trapdoor_Triggered = false;
        trapdoor_le();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
//Szalag bekapcsol, felvonó felmegy
void startTask3(void *parameter) {
  for(;;){
    if(start){
      if(value_szalag == HIGH){
        sensor_szalag_Triggered = true;
        ledcWriteChannel(pwmChannel_1, 0);
        digitalWrite(motor3_in1,LOW);
        digitalWrite(motor3_in2,LOW);  
        vTaskDelay(150/ portTICK_PERIOD_MS);
        szalag_be();
      }
      if(value_lift_lent == HIGH && sensor_szalag_Triggered){
        szalag_ki();
        sensor_szalag_Triggered = false;
        sensor_lift_Triggered = true;
        trapdoor_fel();
        vTaskDelay(1000/ portTICK_PERIOD_MS);
        ledcWriteChannel(pwmChannel_1, 0);
        digitalWrite(motor3_in1, LOW);
        digitalWrite(motor3_in2, LOW);
        lift_fel();
      }    
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
//A labda kikerül a felvonóból
void startTask4(void *parameter) {
  for(;;){
    if(start){
      if(value_lift_fent == HIGH && sensor_lift_Triggered){
        vTaskDelay(400/ portTICK_PERIOD_MS);
        ledcWriteChannel(pwmChannel_2, 50);
        digitalWrite(motor4_in1, LOW);
        digitalWrite(motor4_in2, HIGH);
        pusher_be();
        pusher_ki();
        ledcWriteChannel(pwmChannel_2, 0);
        digitalWrite(motor4_in1, LOW);
        digitalWrite(motor4_in2, LOW);
        sensor_lift_Triggered = false;
      }    
    }
    vTaskDelay(pdMS_TO_TICKS(10));  
  }
}
//Stop
void startTask5(void *parameter) {
  for(;;){
    if(stop && DOOR_motor==false){
      door_close();
    }
    if(stop && start && value_door==HIGH){
      stopAll(); 
    }
    if((stop) && (start) && (PUSHER_motor || DOOR_motor || TRAPDOOR_motor || CONVEYOR_motor ||LIFT_motor)) {
      start = false;
      counterValue = 0;
      if(DOOR_motor==false){
        door_close();
        stopAll();
      }
      stopAll();
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
//Pozíció
void startTask6(void *parameter) {
  for(;;){
    if(start){
      if(value_door==HIGH){
        PUSHER=false;
        DOOR=true;
        TRAPDOOR=false;
        CONVEYOR=false;
        LIFT=false;
      }
      if(value_trapdoor==HIGH){
        PUSHER=false;
        DOOR=false;
        TRAPDOOR=true;
        CONVEYOR=false;
        LIFT=false;
      }
      if(value_szalag==HIGH){
        PUSHER=false;
        DOOR=false;
        TRAPDOOR=false;
        CONVEYOR=true;
        LIFT=false;
      }
      if(value_lift_lent==HIGH && sensor_lift_Triggered){
        PUSHER=false;
        DOOR=false;
        TRAPDOOR=false;
        CONVEYOR=false;
        LIFT=true;
      }
      if(value_lift_fent == HIGH && sensor_lift_Triggered){
        PUSHER=true;
        DOOR=false;
        TRAPDOOR=false;
        CONVEYOR=false;
        LIFT=false;
      }
      vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
//Hőmérséklet
void startTask7(void *parameter) {
  for(;;){
    long sum = 0;
    for (int f = 0; f < 100; f++) {
      sum += analogRead(Temperature_sensor);
    }
    float avg = sum / 100.0;
    float voltage = avg * (3.3 / 4095);
    Temperature = (voltage / 0.01)+10; //korrekció

    if (Temperature >= 21 && !ventilator_state) {
        ventilator_be();
        ventilator_state = true;
    }
    if (Temperature < 20 && ventilator_state) {
        ventilator_ki();
        ventilator_state = false;
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
