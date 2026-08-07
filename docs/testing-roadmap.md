# Testing Roadmap

Mirrors the roadmap in [`papers/2026-mhd-power-propulsion`](https://github.com/Overby-Industries/whitepapers/tree/main/papers/2026-mhd-power-propulsion)
(Section 9), with the concrete artifact in *this* repo that each phase produces.

| Phase | Focus | Artifact here |
|---|---|---|
| I -- Simulation | Validate the non-dimensional groups (Ha, N, Rm) against a candidate channel geometry before cutting any hardware. | [`sim/`](../sim) -- `opp_sim_demo`, `ctest` suite |
| II -- Benchtop | Ionic-liquid/plasma vortex chamber at low power; measure achieved sigma, Ha, N against simulation. | Not yet started -- benchtop data feeds back into `sim/` as validation, not just design input. |
| III -- Vacuum chamber | Full accelerator-mode test; measure thrust, specific impulse, mode-switching transient behavior. | Not yet started. |
| IV -- CubeSat demo | Unmanned orbital demonstrator combining generator trickle-charge, ABEP intake, and electrospray thrust. | The [`firmware/`](../firmware) flight-phase FSM (`ShuttleFSM`) is written to scale toward exactly this kind of mission sequencing, starting from an Arduino Uno testbed. |

## What "done" looks like for Phase I

- [x] Non-dimensional sizing library with unit tests (`sim/`)
- [x] Flight-phase FSM with an `MHD_Activation` sub-FSM matching the
      plasma-ignition -> field-ramp -> thrust-stabilize sequence (`firmware/`)
- [ ] A design point validated against real benchtop conductivity/density
      measurements once Phase II starts (currently uses literature-range
      placeholder values, see `sim/src/main.cpp`)
- [ ] CAD geometry ([`cad/mhd_core_blender.py`](../cad/mhd_core_blender.py))
      dimensioned to match whatever design point Phase I settles on
