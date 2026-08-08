# System Architecture

Same architecture as [`papers/2026-mhd-power-propulsion`](https://github.com/Overby-Industries/whitepapers/tree/main/papers/2026-mhd-power-propulsion)
Section 8 -- reproduced here so it's visible without leaving this repo.

```
 [Solar Arrays]        [Solar Wind Collector]
       |                        |
       v                        v
 +---------------------------------+
 |     DC Link / Power Mgmt        |<---->[Graphene Supercapacitors]
 |     (MPPT, rectifiers)          |<---->[Al-ion Battery / Thermal Tank]
 +---------------------------------+
       ^                        |
       |  (bidirectional)       v
 +-----------------------------------+
 |            MHD Core                |<-- Pre-ionizer (RF/ECR/Arc)
 |  Mode A: Generator  --> DC Link    |
 |  Mode B: Thruster   <-- DC Link    |
 +-----------------------------------+
                  |
          Magnetic / Fluid Nozzle
                  |
      +-----------+------------------+
      | ABEP Inlet (LEO/VLEO)        |  Electrospray Emitter (ionic-liquid)
      +-----------+------------------+
                  v
                Thrust
```

```mermaid
flowchart LR
    SA[Solar arrays] --> PDM[DC Link & Power Mgmt]
    SWC[Solar-wind collector] --> PDM
    CAP[Graphene supercaps] <--> PDM
    AIB[Al-ion battery & thermal tank] <--> PDM
    PDM <--> MHD[MHD Core: Generator / Thruster]
    ION[Pre-ionizer RF/ECR] --> MHD
    MHD --> NOZ[Magnetic / Fluid nozzle]
    ABEP[ABEP intake] --> MHD
    ES[Electrospray - ionic liquid] --> NOZ
    NOZ --> THRUST[Thrust]
```

## Flight-phase control (firmware/)

The `ShuttleFSM` in [`firmware/lib/shuttle_fsm`](../firmware/lib/shuttle_fsm)
sequences a vehicle through the phases that use the above power/propulsion
core:

```mermaid
stateDiagram-v2
    [*] --> GroundIdle
    GroundIdle --> PreFlightChecks: checks passed
    PreFlightChecks --> TakeoffRoll: min dwell elapsed
    TakeoffRoll --> AscentAtmospheric: airspeed > threshold
    AscentAtmospheric --> HypersonicTransition: Mach > 5 and altitude > 30km
    HypersonicTransition --> MHD_Activation: MHD transition ready
    MHD_Activation --> OrbitalCruise: sub-FSM reaches ThrustStabilize
    OrbitalCruise --> PayloadDeployment
    PayloadDeployment --> Reentry
    Reentry --> ApproachAndLanding
    ApproachAndLanding --> Maintenance
    GroundIdle --> AbortSafeMode: fault
    TakeoffRoll --> AbortSafeMode: fault
    AscentAtmospheric --> AbortSafeMode: fault
    HypersonicTransition --> AbortSafeMode: fault
    MHD_Activation --> AbortSafeMode: fault
    OrbitalCruise --> AbortSafeMode: fault
```

```mermaid
stateDiagram-v2
    [*] --> PlasmaIgnition
    PlasmaIgnition --> FieldRamp: plasma stable
    FieldRamp --> ThrustStabilize: field at target
```

A fault (`ShuttleSensors::faultDetected`) preempts every state above and is
sticky -- recovery out of `AbortSafeMode` is a deliberate, ground-commanded
action, never automatic. See `ShuttleFSM::update()` for the exact logic.
