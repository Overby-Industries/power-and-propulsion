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
