#pragma once
#include <stdint.h>

// Overby Industries SSTO Shuttle -- top-level flight-phase state machine.
// Values are fixed so telemetry/log consumers (see the OTF-1 telemetry
// protocol in the `telemetry-protocol` repo) can treat the state as a
// stable numeric ID across firmware revisions.
enum ShuttleState : uint8_t {
  GroundIdle = 0,
  PreFlightChecks = 1,
  TakeoffRoll = 2,
  AscentAtmospheric = 3,
  HypersonicTransition = 4,
  MHD_Activation = 5,
  OrbitalCruise = 6,
  PayloadDeployment = 7,
  Reentry = 8,
  ApproachAndLanding = 9,
  AbortSafeMode = 10,
  Maintenance = 11
};

// Sub-states entered only while currentState() == MHD_Activation.
enum MHDSubState : uint8_t {
  PlasmaIgnition = 0,
  FieldRamp = 1,
  ThrustStabilize = 2
};

// Human-readable names for logging/telemetry, indexed by the enum value.
// A command from the ground. Unlike a sensor reading, a command is an event,
// not a level: it is handed to ShuttleFSM::command() once, when it arrives.
//
// Every phase change the FSM cannot decide for itself is one of these. The
// design docs draw OrbitalCruise -> PayloadDeployment -> Reentry ->
// ApproachAndLanding -> Maintenance with no trigger on the arrows, and say
// that recovery out of AbortSafeMode is "a deliberate, ground-commanded
// action, never automatic". Rather than invent sensor thresholds for flight
// phases whose real criteria do not exist yet, each of those steps is an
// explicit command, valid from exactly one state.
enum GroundCommand : uint8_t {
  CmdNone = 0,
  CmdDeployPayload = 1,    // OrbitalCruise      -> PayloadDeployment
  CmdBeginReentry = 2,     // PayloadDeployment  -> Reentry
  CmdBeginApproach = 3,    // Reentry            -> ApproachAndLanding
  CmdLandingComplete = 4,  // ApproachAndLanding -> Maintenance
  CmdRecover = 5,          // AbortSafeMode      -> Maintenance (only once the fault is clear)
  CmdReturnToIdle = 6      // Maintenance        -> GroundIdle
};

inline const char* groundCommandName(GroundCommand c) {
  switch (c) {
    case CmdNone: return "CmdNone";
    case CmdDeployPayload: return "CmdDeployPayload";
    case CmdBeginReentry: return "CmdBeginReentry";
    case CmdBeginApproach: return "CmdBeginApproach";
    case CmdLandingComplete: return "CmdLandingComplete";
    case CmdRecover: return "CmdRecover";
    case CmdReturnToIdle: return "CmdReturnToIdle";
    default: return "Unknown";
  }
}

inline const char* shuttleStateName(ShuttleState s) {
  switch (s) {
    case GroundIdle: return "GroundIdle";
    case PreFlightChecks: return "PreFlightChecks";
    case TakeoffRoll: return "TakeoffRoll";
    case AscentAtmospheric: return "AscentAtmospheric";
    case HypersonicTransition: return "HypersonicTransition";
    case MHD_Activation: return "MHD_Activation";
    case OrbitalCruise: return "OrbitalCruise";
    case PayloadDeployment: return "PayloadDeployment";
    case Reentry: return "Reentry";
    case ApproachAndLanding: return "ApproachAndLanding";
    case AbortSafeMode: return "AbortSafeMode";
    case Maintenance: return "Maintenance";
    default: return "Unknown";
  }
}
