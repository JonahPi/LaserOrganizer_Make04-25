#include <Arduino.h>  // Not needed in Arduino IDE, but required for PlatformIO
#include <WiFi.h>
#include <PubSubClient.h>

#define SER_PIN   21  // PIN assigned to the SER pin of the shift register
#define RCLK_PIN  17  // PIN assigned to the RCLK pin of the shift register
#define SRCLK_PIN 16  // PIN assigned to the SRCLK pin of the shift register

#define DISABLE_BIT  0  // Bit of the shift register to disable the motor driver
#define TILT_STEP_BIT   5  // Bit of the shift register to step the tilt motor
#define PAN_STEP_BIT   1  // Bit of the shift register to step the pan motor
#define TILT_DIR_BIT    6  
#define PAN_DIR_BIT    2  
#define BEEPER_BIT   7  // Not used in this code, but defined for completeness

#define LED_PIN 22  // PIN assigned to the LED
#define X_ENDSTOP 36 // PIN assigned to X-axis endstop
#define Y_ENDSTOP 35 // PIN assigned to Y-axis endstop

#define X_MAXSTEPS 7700  // Maximum steps for the X-axis (pan)
#define Y_MAXSTEPS 4200 // Maximum steps for the Y-axis (tilt)
#define FASTMOVE 300 //us delay between steps
#define SLOMOVE 3000 //us delay between steps
#define LEDONDELAY 60 //seconds to keep LED on after moving
#define KEEP_STEPPER_ENGAGED 60 // seconds to keep stepper engaged after movement

#include "secrets.h"  // Include your secrets.h file for Wi-Fi and MQTT credentials

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;


WiFiClient espClient;
PubSubClient client(espClient);

uint8_t shiftRegisterState = 0;  
int tilt_position = 0;  
int pan_position = 0;  
int tilt_target = 0;
int pan_target = 0;
bool LedState = false;
bool MotorMoving = false;
unsigned long LedStartTime = 0;


// Function prototypes
void setBit(uint8_t bit, bool state); // Set a specific bit in the shift register state
void sendToShiftRegister(); // Send the current state to the shift register
void tiltStepperTo(int& currentPos, int targetPos); // Move the tilt stepper motor to the target position
void panStepperTo(int& panPos, int targetPan); // Move the pan stepper motor to the target position
void callback(char* topic, byte* message, unsigned int length); // MQTT callback function to get messages from Adarfruit IO
void reconnect();
void callibrate();// Move the stepper motors to end-stops to find zero-position

void setup() {
    Serial.begin(115200);
    pinMode(SER_PIN, OUTPUT);
    pinMode(RCLK_PIN, OUTPUT);
    pinMode(SRCLK_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(X_ENDSTOP, INPUT);
    pinMode(Y_ENDSTOP, INPUT);
    setBit(DISABLE_BIT, true);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    // Configure MQTT Server and Callback
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);
    callibrate();
}

void loop() {
    static unsigned long stepperEngagedUntil = 0;
    bool moved = false; 

    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    if (tilt_target != tilt_position || pan_target != pan_position) {
        LedState = true;
        LedStartTime = millis(); // Reset the timer before movement
    }
  
    // Ensure LedState stays true for at least 5 seconds
    if (LedState) {
        unsigned long elapsedTime = millis() - LedStartTime;
    
        if (elapsedTime >= LEDONDELAY * 1000) {
            LedState = false;  // Turn off LED after full duration
            digitalWrite(LED_PIN, LOW); 
        } 
        else if (!MotorMoving) {
            // Start blinking when the motor stops moving
            if ((elapsedTime / 300) % 2 == 0) {  // Blink every 500ms
                digitalWrite(LED_PIN, HIGH);
            } else {
                digitalWrite(LED_PIN, LOW);
            }
        } 
        else {
            digitalWrite(LED_PIN, HIGH);  // LED stays ON before blinking starts
        }
    }

    if (tilt_target != tilt_position) {
        if (millis() > stepperEngagedUntil) { //If the stepper has been disengaged, callibrate before moving
            callibrate();
        }
        moved = true;
        MotorMoving = true;
        setBit(DISABLE_BIT, false);
        tiltStepperTo(tilt_position, tilt_target);
        stepperEngagedUntil = millis() + KEEP_STEPPER_ENGAGED * 1000;  // Keep stepper engaged so it doesn't move by gravity
        MotorMoving = false;
    }

    if (pan_target != pan_position) {
        if (millis() > stepperEngagedUntil) { //If the stepper has been disengaged, callibrate before moving
            callibrate();
        }
        moved = true;
        MotorMoving = true;
        setBit(DISABLE_BIT, false);
        panStepperTo(pan_position, pan_target);
        stepperEngagedUntil = millis() + KEEP_STEPPER_ENGAGED * 1000;
        MotorMoving = false;
    } 

    if (millis() > stepperEngagedUntil) {  //Disengage stepper after a delay to save power
        setBit(DISABLE_BIT, true);
    }

    if (!moved) {
        delay(10);
    }
}

void tiltStepperTo(int& currentPos, int targetPos) { // The tilt action is triggered by the y-axis motor only
    int delay_ms;
    bool direction = (targetPos > currentPos);
    setBit(TILT_DIR_BIT, direction);
    
    int steps = abs(targetPos - currentPos);
    if (steps > 400) {
        delay_ms = FASTMOVE;
    } else {
        delay_ms = SLOMOVE;
    }
    for (int i = 0; i < steps; i++) {
        setBit(TILT_STEP_BIT, true);
        delayMicroseconds(delay_ms);
        setBit(TILT_STEP_BIT, false);
        delayMicroseconds(delay_ms);
    }
    currentPos = targetPos;
}

void panStepperTo(int& panPos, int targetPan) { // For a pan action, both motors are activated
    int delay_ms;
    bool panDirection = (targetPan > panPos);
    setBit(PAN_DIR_BIT, panDirection);
    setBit(TILT_DIR_BIT, !panDirection); 
    
    int steps = abs(targetPan - panPos);
    if (steps > 400) {
        delay_ms = FASTMOVE;
    } else {
        delay_ms = SLOMOVE;
    }
    for (int i = 0; i < steps; i++) {
        setBit(PAN_STEP_BIT, true);
        setBit(TILT_STEP_BIT, true); // Ensure opposite movement for pan
        delayMicroseconds(delay_ms);
        setBit(PAN_STEP_BIT, false);
        setBit(TILT_STEP_BIT, false);
        delayMicroseconds(delay_ms);
    }
    panPos = targetPan;
}

void setBit(uint8_t bit, bool state) {
    if (state)
        shiftRegisterState |= (1 << bit);  
    else
        shiftRegisterState &= ~(1 << bit); 

    sendToShiftRegister();
}

void sendToShiftRegister() {
    digitalWrite(RCLK_PIN, LOW);
    for (int i = 7; i >= 0; i--) {
        digitalWrite(SRCLK_PIN, LOW);
        digitalWrite(SER_PIN, (shiftRegisterState >> i) & 1);
        digitalWrite(SRCLK_PIN, HIGH);
        delayMicroseconds(10);
    }
    digitalWrite(RCLK_PIN, HIGH);
}

void callback(char* topic, byte* message, unsigned int length) {
    String msg = "";
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)message[i];
    }

    
    if (strcmp(topic, MQTT_FEED_POSITION) == 0) {
        int sepIndex = msg.indexOf('/');
        if (sepIndex != -1) {
            pan_target = msg.substring(0, sepIndex).toInt();
            tilt_target = msg.substring(sepIndex + 1).toInt();
            
            pan_target = constrain(pan_target, 0, X_MAXSTEPS);
            tilt_target = constrain(tilt_target, 0, Y_MAXSTEPS);
        }
    }
    if ((tilt_target == 0) && (pan_target == 0)){
        callibrate();
    }
}


void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        
        if (client.connect("ESP32_Client", MQTT_USER, MQTT_PASS)) {
            Serial.println("Connected!");
            client.subscribe(MQTT_FEED_POSITION);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" Retrying in 5 seconds...");
            delay(5000);
        }
    }
}

void callibrate(){
    int delay_ms = FASTMOVE;
    digitalWrite(LED_PIN,false);
    setBit(DISABLE_BIT, false);
    setBit(PAN_DIR_BIT, false);
    setBit(TILT_DIR_BIT, true);
    while (digitalRead(X_ENDSTOP))
    {
        setBit(PAN_STEP_BIT, true);
        setBit(TILT_STEP_BIT, true); 
        delayMicroseconds(delay_ms);
        setBit(PAN_STEP_BIT, false);
        setBit(TILT_STEP_BIT, false);
        delayMicroseconds(delay_ms);
    }
    pan_position = 0;
    setBit(TILT_DIR_BIT, false);
    while (digitalRead(Y_ENDSTOP))
    {
        setBit(TILT_STEP_BIT, true); 
        delayMicroseconds(delay_ms);
        setBit(TILT_STEP_BIT, false);
        delayMicroseconds(delay_ms);
    }
    setBit(DISABLE_BIT, true);
    tilt_position = 0;    
}
