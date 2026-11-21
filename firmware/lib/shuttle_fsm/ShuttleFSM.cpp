#include "ShuttleFSM.h"

#if defined(ARDUINO)
#include <Arduino.h>
#define OPP_LOG(msg) Serial.println(msg)
#else
#include <cstdio>
#define OPP_LOG(msg) std::puts(msg)
#endif

void ShuttleFSM::begin(uint32_t nowMillis) {
  currentState = GroundIdle;
  mhdSub = PlasmaIgnition;
  lastStateChange = nowMillis;
  enterState(GroundIdle);
}

void ShuttleFSM::update(const ShuttleSensors& sensors, uint32_t nowMillis) {
  // A fault is treated as interrupt-priority: it preempts whatever state
  // logic runs below, regardless of current flight phase. Recovery out of
  // AbortSafeMode is a deliberate, ground-commanded action -- never
  // automatic -- so once here we stay here.
  if (sensors.faultDetected && currentState != AbortSafeMode) {
    transitionTo(AbortSafeMode, nowMillis);
    return;
  }

  switch (currentState) {
    case GroundIdle:
      if (sensors.preflightChecksPassed) {
        transitionTo(PreFlightChecks, nowMillis);
      }
      break;

    case PreFlightChecks:
      if (sensors.preflightChecksPassed && timeInState(nowMillis) >= kPreFlightMinDwellMs) {
        transitionTo(TakeoffRoll, nowMillis);
      }
      break;

    case TakeoffRoll:
      if (sensors.airspeed_mps > kTakeoffSpeedThreshold_mps) {
        transitionTo(AscentAtmospheric, nowMillis);
      }
      break;

    case AscentAtmospheric:
      if (sensors.machNumber > 5.0f && sensors.altitude_m > 30000.0f) {
        transitionTo(HypersonicTransition, nowMillis);
      }
      break;

    case HypersonicTransition:
      if (sensors.mhdTransitionReady) {
        transitionTo(MHD_Activation, nowMillis);
      }
      break;

    case MHD_Activation:
      updateMHDSubStates(sensors);
      if (mhdSub == ThrustStabilize) {
        transitionTo(OrbitalCruise, nowMillis);
      }
      break;

    case OrbitalCruise:
      // Station-keeping, solar-wind trickle harvesting, OTF-1 telemetry
      // reporting. Ground-commanded transition to PayloadDeployment
      // (CmdDeployPayload, see command()).
      break;

    case PayloadDeployment:
      // Deploy asteroid miner pods.
      break;

    case Reentry:
      // Heat-shield / ionic-liquid thermal-buffer monitoring, attitude
      // control.
      break;

    case ApproachAndLanding:
      // Glide slope, flare, runway alignment.
      break;

    case AbortSafeMode:
      // Safe the vehicle, power down non-critical systems.
      break;

    case Maintenance:
      // Diagnostics, logging.
      break;
  }
}

bool ShuttleFSM::command(GroundCommand cmd, const ShuttleSensors& sensors, uint32_t nowMillis) {
  // A fault outranks the ground. Handled before anything else so that no
  // command, recovery included, gets past a standing fault.
  if (sensors.faultDetected) {
    OPP_LOG("Ground command ignored - fault standing");
    return false;
  }

  ShuttleState target = currentState;
  switch (cmd) {
    case CmdDeployPayload:
      if (currentState == OrbitalCruise) target = PayloadDeployment;
      break;
    case CmdBeginReentry:
      if (currentState == PayloadDeployment) target = Reentry;
      break;
    case CmdBeginApproach:
      if (currentState == Reentry) target = ApproachAndLanding;
      break;
    case CmdLandingComplete:
      if (currentState == ApproachAndLanding) target = Maintenance;
      break;
    case CmdRecover:
      // Out of AbortSafeMode, deliberately and only by this. To Maintenance
      // (diagnostics) and not back to a flight phase: a vehicle that has just
      // safed itself is inspected before it is anything else.
      if (currentState == AbortSafeMode) target = Maintenance;
      break;
    case CmdReturnToIdle:
      if (currentState == Maintenance) target = GroundIdle;
      break;
    case CmdNone:
    default:
      break;
  }

  if (target == currentState) {
    OPP_LOG("Ground command ignored - not valid in this state");
    return false;
  }
  transitionTo(target, nowMillis);
  return true;
}

void ShuttleFSM::transitionTo(ShuttleState newState, uint32_t nowMillis) {
  exitState(currentState);
  currentState = newState;
  lastStateChange = nowMillis;
  enterState(currentState);
  OPP_LOG(shuttleStateName(currentState));
}

void ShuttleFSM::enterState(ShuttleState state) {
  switch (state) {
    case GroundIdle:
      OPP_LOG("Entered GroundIdle - systems safe");
      break;
    case PreFlightChecks:
      OPP_LOG("Entered PreFlightChecks");
      break;
    case MHD_Activation:
      OPP_LOG("MHD core activating - plasma systems initializing");
      mhdSub = PlasmaIgnition;
      break;
    case AbortSafeMode:
      OPP_LOG("ABORT - safing vehicle");
      break;
    default:
      break;
  }
}

void ShuttleFSM::exitState(ShuttleState state) {
  switch (state) {
    case MHD_Activation:
      OPP_LOG("Exiting MHD_Activation");
      break;
    default:
      break;
  }
}

void ShuttleFSM::updateMHDSubStates(const ShuttleSensors& sensors) {
  switch (mhdSub) {
    case PlasmaIgnition:
      if (sensors.plasmaStable) mhdSub = FieldRamp;
      break;
    case FieldRamp:
      if (sensors.fieldAtTarget) mhdSub = ThrustStabilize;
      break;
    case ThrustStabilize:
      // Thrust/Hall-effect/temperature monitoring feeds sensors.faultDetected
      // upstream (see shuttle_fsm.ino) rather than living here, so this
      // function stays focused on sub-state progression only.
      break;
  }
}
