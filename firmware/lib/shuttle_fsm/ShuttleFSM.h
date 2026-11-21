#pragma once
#include <stdint.h>
#include "ShuttleState.h"

// Sensor snapshot the FSM reads each tick. Populate this from real hardware
// (or from a test harness); every field defaults to a safe/neutral value so
// the FSM never transitions on uninitialized data.
struct ShuttleSensors {
  bool preflightChecksPassed = false;
  float airspeed_mps = 0.0f;
  float altitude_m = 0.0f;
  float machNumber = 0.0f;
  bool mhdTransitionReady = false;
  bool plasmaStable = false;
  bool fieldAtTarget = false;
  float mhdTemperature_C = 0.0f;

  // Set true from fault-detection logic (ideally an ISR-fed volatile flag
  // upstream, see shuttle_fsm.ino) to force an immediate AbortSafeMode
  // transition regardless of current state.
  bool faultDetected = false;
};

// Minimum airspeed (m/s) before TakeoffRoll may hand off to
// AscentAtmospheric. Placeholder -- replace with the real rotation speed
// once airframe data exists.
constexpr float kTakeoffSpeedThreshold_mps = 70.0f;

class ShuttleFSM {
public:
  void begin(uint32_t nowMillis);
  void update(const ShuttleSensors& sensors, uint32_t nowMillis);

  // Hand the FSM one command from the ground, when it arrives. Returns true if
  // it caused a transition and false if it was ignored.
  //
  // A command is valid from exactly one state (see GroundCommand), so
  // repeating it, or sending it at the wrong moment, does nothing -- and a
  // caller that holds one across several ticks cannot chain the FSM through
  // phases it did not mean to skip.
  //
  // Nothing is accepted while `sensors.faultDetected` is true, CmdRecover
  // included: a fault outranks the ground, and recovery over a standing fault
  // would only abort again on the next update(). Recovery goes to Maintenance,
  // never straight back into flight.
  bool command(GroundCommand cmd, const ShuttleSensors& sensors, uint32_t nowMillis);

  ShuttleState state() const { return currentState; }
  MHDSubState mhdSubState() const { return mhdSub; }
  uint32_t timeInState(uint32_t nowMillis) const { return nowMillis - lastStateChange; }

private:
  void transitionTo(ShuttleState newState, uint32_t nowMillis);
  void enterState(ShuttleState state);
  void exitState(ShuttleState state);
  void updateMHDSubStates(const ShuttleSensors& sensors);

  ShuttleState currentState = GroundIdle;
  MHDSubState mhdSub = PlasmaIgnition;
  uint32_t lastStateChange = 0;

  // Minimum dwell time in PreFlightChecks before it's allowed to hand off,
  // even if the checks-passed flag is already true -- so a single noisy
  // sensor read can't bounce the FSM straight through the state.
  static const uint32_t kPreFlightMinDwellMs = 1500;
};
