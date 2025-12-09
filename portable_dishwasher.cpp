/*
Rafid Ahmed, Darson Chen, Mira Pudipedi, Jenny Dong
Portable Dishwasher
*/

#include <Arduino.h>

//  PIN CONFIG
const uint8_t PIN_PIR = 2;
const uint8_t PIN_MOTOR_IN1 = 5;
const uint8_t PIN_MOTOR_IN2 = 6;
const uint8_t PIN_MOTOR_EN  = 9;
const uint8_t PIN_PUMP_RELAY = 7;
const uint8_t PIN_LED = 13;

// CYCLE CONFIG (seconds)
unsigned long washDuration  = 30;
unsigned long rinseDuration = 20;
unsigned long spinDuration  = 10;

//  SAFETY CONFIG
const unsigned long PIR_DEBOUNCE_MS = 1500;
const unsigned long COOLDOWN_MS = 60000;         // 1 minute between cycles
const unsigned long MAX_RUNTIME_MS = 600000;     // 10 min safety timeout

//  STATE
volatile bool pirTriggered = false;
unsigned long lastPirTime = 0;
unsigned long lastCycleEnd = 0;

enum State { IDLE, RUNNING };
State systemState = IDLE;

unsigned long phaseEndTime = 0;
unsigned long cycleStartTime = 0;
String phase = "NONE";

uint8_t motorSpeed = 200;

//  PIR INTERRUPT
void pirISR() {
  unsigned long now = millis();
  if (now - lastPirTime > PIR_DEBOUNCE_MS) {
    pirTriggered = true;
    lastPirTime = now;
  }
}

//  MOTOR CONTROL
void motorStop() {
  analogWrite(PIN_MOTOR_EN, 0);
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
}

void motorForward(uint8_t speed) {
  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  analogWrite(PIN_MOTOR_EN, speed);
}

void motorReverse(uint8_t speed) {
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, HIGH);
  analogWrite(PIN_MOTOR_EN, speed);
}

//  PUMP CONTROL
void pumpOn()  { digitalWrite(PIN_PUMP_RELAY, HIGH); }
void pumpOff() { digitalWrite(PIN_PUMP_RELAY, LOW);  }

//  SETUP
void setup() {
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_MOTOR_EN, OUTPUT);
  pinMode(PIN_PUMP_RELAY, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  motorStop();
  pumpOff();
  digitalWrite(PIN_LED, LOW);

  attachInterrupt(digitalPinToInterrupt(PIN_PIR), pirISR, RISING);
}

//  CYCLE CONTROL
void startCycle() {
  systemState = RUNNING;
  cycleStartTime = millis();
  phase = "WASH";
  phaseEndTime = millis() + (washDuration * 1000UL);

  pumpOn();
  motorForward(motorSpeed);
  digitalWrite(PIN_LED, HIGH);
}

void endCycle() {
  motorStop();
  pumpOff();
  digitalWrite(PIN_LED, LOW);

  systemState = IDLE;
  phase = "NONE";
  lastCycleEnd = millis();
}

void updateCycle() {
  if (systemState != RUNNING) return;

  unsigned long now = millis();

  // Safety timeout
  if (now - cycleStartTime > MAX_RUNTIME_MS) {
    endCycle();
    return;
  }

  if (now >= phaseEndTime) {
    if (phase == "WASH") {
      phase = "RINSE";
      phaseEndTime = now + (rinseDuration * 1000UL);
      motorForward(motorSpeed);
    }
    else if (phase == "RINSE") {
      phase = "SPIN";
      phaseEndTime = now + (spinDuration * 1000UL);
      motorReverse(motorSpeed / 2);
    }
    else if (phase == "SPIN") {
      endCycle();
    }
  }
}

//  LOOP 
void loop() {
  // Motion-triggered start
  if (pirTriggered) {
    pirTriggered = false;

    unsigned long now = millis();
    if (systemState == IDLE && (now - lastCycleEnd) > COOLDOWN_MS) {
      startCycle();
    }
  }

  // Update cycle phases
  updateCycle();
}

