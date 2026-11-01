// multitasking rover arm: pot-controlled servo + heartbeat led
// - the pot sets a target angle, the servo steps toward it at 1 degree per 15 ms
// - the onboard led blinks at 2 hz (250 ms on, 250 ms off)
// - an exponential moving average (ema) filter smooths the pot reading (a = 0.2)
// - no delay() anywhere: every task runs on its own millis() timestamp

#include <Servo.h>

// ---------- pins ----------
const uint8_t POT_PIN   = A0;           // potentiometer wiper (analog input)
const uint8_t SERVO_PIN = 9;            // servo signal (pwm)
const uint8_t LED_PIN   = LED_BUILTIN;  // onboard status led (pin 13)

// ---------- timing (all in milliseconds) ----------
const unsigned long LED_INTERVAL  = 250;  // toggle every 250 ms -> 2 hz blink, 50% duty
const unsigned long STEP_INTERVAL = 15;   // move the servo 1 degree every 15 ms (about 66.7 deg/s)
const unsigned long ADC_INTERVAL  = 10;   // sample the pot every 10 ms (gives the filter a fixed sample rate)

// ---------- filter ----------
const float ALPHA = 0.2;  // ema weight: small = smoother but slower, large = faster but noisier

// ---------- state ----------
Servo arm;                          // servo object
unsigned long lastLed  = 0;         // timestamp of the last led toggle
unsigned long lastStep = 0;         // timestamp of the last servo step
unsigned long lastAdc  = 0;         // timestamp of the last pot sample
bool ledOn = false;                 // current led state
int currentAngle = 90;              // where the arm is now (what we last commanded)
int targetAngle  = 90;              // where the operator wants the arm (from the pot)
float filtered = 0;                 // filtered pot reading (float so the ema keeps its fractions)

void setup() {
  pinMode(LED_PIN, OUTPUT);
  arm.attach(SERVO_PIN);

  // seed the filter with the real pot value so it does not start from 0
  // and drag the target toward zero during the first samples
  filtered = analogRead(POT_PIN);

  // start the arm at the pot's position (scaled from 0-1023 to 0-180 degrees)
  // note: the servo has no position feedback, so the first write may move the horn quickly
  currentAngle = targetAngle = constrain(map((int)filtered, 0, 1023, 0, 180), 0, 180);
  arm.write(currentAngle);
}

void loop() {
  unsigned long now = millis();  // one time reading per pass, shared by all tasks

  // task 1: operator control (read and filter the pot)
  // unsigned subtraction stays correct even when millis() overflows (about every 49.7 days)
  if (now - lastAdc >= ADC_INTERVAL) {
    lastAdc += ADC_INTERVAL;  // += (not = now) so the sample rate does not drift
    int raw = analogRead(POT_PIN);

    // ema filter: blend the new sample with the previous filtered value
    filtered = ALPHA * raw + (1.0 - ALPHA) * filtered;

    // convert the filtered value to a target angle and clamp it to the valid range
    targetAngle = constrain(map((int)filtered, 0, 1023, 0, 180), 0, 180);
  }

  // task 1b: smooth servo motion (slew-rate limit)
  // the pot only changes the target; the arm moves at most 1 degree per step
  if (now - lastStep >= STEP_INTERVAL) {
    lastStep += STEP_INTERVAL;
    if (currentAngle < targetAngle)      currentAngle++;  // move up toward the target
    else if (currentAngle > targetAngle) currentAngle--;  // move down toward the target
    arm.write(currentAngle);                              // send the new angle to the servo
  }

  // task 2: system health (heartbeat led)
  // keeps blinking at a steady rate whatever the pot or the servo is doing,
  // which shows the loop is never blocked
  if (now - lastLed >= LED_INTERVAL) {
    lastLed += LED_INTERVAL;
    ledOn = !ledOn;                // flip the led state
    digitalWrite(LED_PIN, ledOn);  // update the pin
  }
}
