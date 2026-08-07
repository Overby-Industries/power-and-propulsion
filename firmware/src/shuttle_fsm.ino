// =============================================
// Overby Industries SSTO Shuttle FSM -- Arduino sketch
// Wires ShuttleFSM (see ShuttleFSM.h/.cpp) to real pins.
//
// Non-blocking: no delay() anywhere in loop(). Every timed action is driven
// off millis(), so the FSM keeps servicing sensors and the fault check every
// pass instead of blocking for a fixed window.
// =============================================
#include <Arduino.h>
#include <avr/wdt.h>
#include "ShuttleState.h"
#include "ShuttleFSM.h"

const uint8_t kLedPin = 13;          // Status LED
const uint8_t kPreflightButtonPin = 2; // Ground-test stand-in for the real preflight-checks signal
const uint8_t kFaultInputPin = 3;    // Wire to a real fault line (over-temp comparator, etc.)

ShuttleFSM fsm;

// Set from loop(); read by update(). On real hardware, anything set from an
// ISR needs to be `volatile` -- this one currently isn't touched from an
// ISR, but ISR-driven fault sources (e.g. an over-current comparator on an
// external interrupt pin) are the next step once real MHD sensors exist.
bool faultLatched = false;

uint32_t lastBlinkMillis = 0;
const uint16_t kBlinkIntervalMs = 500;
bool ledState = false;

void setup() {
  Serial.begin(115200);
  pinMode(kLedPin, OUTPUT);
  pinMode(kPreflightButtonPin, INPUT_PULLUP);
  pinMode(kFaultInputPin, INPUT_PULLUP);

  Serial.println(F("Overby Industries SSTO Shuttle FSM starting..."));
  fsm.begin(millis());

  // 8-second watchdog: if loop() ever hangs (e.g. a blocking call creeps
  // back in during development), the MCU resets rather than sitting dead
  // in whatever state it was last in.
  wdt_enable(WDTO_8S);
}

void loop() {
  wdt_reset();

  const uint32_t now = millis();

  // --- Non-blocking status blink, independent of FSM state timing ---
  if (now - lastBlinkMillis >= kBlinkIntervalMs) {
    lastBlinkMillis = now;
    ledState = !ledState;
    digitalWrite(kLedPin, ledState ? HIGH : LOW);
  }

  // --- Gather sensor snapshot for this tick ---
  // Replace each placeholder with a real sensor read as hardware comes
  // online (altitude, airspeed, MHD temperature, plasma diagnostics, etc.).
  ShuttleSensors sensors;
  sensors.preflightChecksPassed = (digitalRead(kPreflightButtonPin) == LOW);
  sensors.faultDetected = faultLatched || (digitalRead(kFaultInputPin) == LOW);

  // TODO once instrumented: sensors.airspeed_mps, .altitude_m, .machNumber,
  // .mhdTransitionReady, .plasmaStable, .fieldAtTarget, .mhdTemperature_C.

  fsm.update(sensors, now);
}
