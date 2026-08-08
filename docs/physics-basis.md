# Physics Basis

The canonical physics writeup for this repository lives in the
[`whitepapers`](https://github.com/Overby-Industries/whitepapers) repo, in
[`papers/2026-mhd-power-propulsion`](https://github.com/Overby-Industries/whitepapers/tree/main/papers/2026-mhd-power-propulsion).
That paper is the single source of truth for the equations; this page just
maps each concept to where it's implemented here, so the two don't drift
out of sync with each other.

| Concept (whitepaper section) | Implementation |
|---|---|
| Faraday's law, motional EMF, Hall current (Sec. 4) | [`sim/include/opp/mhd_generator.hpp`](../sim/include/opp/mhd_generator.hpp) -- `idealFaradayVoltage_V()`, `effectiveConductivity()` |
| Hartmann number, interaction parameter, magnetic Reynolds number (Sec. 4.1) | `MHDGenerator::hartmannNumber()`, `interactionParameter()`, `magneticReynoldsNumber()` |
| Lorentz body force, thruster mode (Sec. 5) | [`sim/include/opp/lorentz_thruster.hpp`](../sim/include/opp/lorentz_thruster.hpp) -- `bodyForceDensity_Npm3()`, `thrust_N()` |
| Solar wind trickle power (Sec. 6) | [`sim/include/opp/solar_wind.hpp`](../sim/include/opp/solar_wind.hpp) -- `dynamicPressure_Pa()`, `availablePower_W()` |
| MHD activation flight-phase sequencing (plasma ignition -> field ramp -> thrust stabilize) | [`firmware/lib/shuttle_fsm`](../firmware/lib/shuttle_fsm) -- `MHDSubState`, `ShuttleFSM::updateMHDSubStates()` |

## Why the sim/ models are deliberately simplified

Every formula in `sim/` is a first-order, order-of-magnitude sizing tool
for Phase I work (see [`testing-roadmap.md`](testing-roadmap.md)), not a
flight-performance model. Specifically:

- The generator power estimate is total Ohmic dissipation implied by the
  induced field, not a matched-load circuit result -- see the caveat in
  `MHDGenerator::estimatedPower_W()`.
- The thruster model assumes uniform current density and field across the
  channel, which real profiles don't do near the nozzle exit.
- Neither model accounts for thermal limits, electrode erosion over time,
  or plasma instabilities -- those are exactly what Phase II benchtop
  testing exists to characterize.

If you're extending these models, keep that honesty: it's fine to say "this
is a rough bound," it's not fine to imply more precision than a benchtop
core actually has.
