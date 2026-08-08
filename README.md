# Power and Propulsion

**Overby Industries -- dual-mode power and propulsion core.**

[![Build & Test](https://github.com/Overby-Industries/power-and-propulsion/actions/workflows/build.yml/badge.svg)](https://github.com/Overby-Industries/power-and-propulsion/actions/workflows/build.yml)
[![License: Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

One channel, one set of coils, two jobs: a magnetohydrodynamic (MHD) core
that generates electricity from a flowing conductive plasma, and -- with
current driven the other direction -- accelerates that same plasma out a
nozzle for thrust. Paired with dual-mode ion propulsion (Air-Breathing
Electric Propulsion in atmosphere, ionic-liquid electrospray in vacuum) and
a solar-wind trickle-charge collector.

The physics writeup this repo implements against lives in
[`whitepapers/papers/2026-mhd-power-propulsion`](https://github.com/Overby-Industries/whitepapers/tree/main/papers/2026-mhd-power-propulsion) --
read that first if you want the *why*; this repo is the *how*.

## Repo Structure

```
power-and-propulsion/
├── docs/
│   ├── physics-basis.md        # maps whitepaper equations -> code
│   ├── system-architecture.md  # block diagrams, FSM state diagrams
│   ├── testing-roadmap.md      # Phase I-IV, what's done vs. not
│   └── concept-notes.md        # original brainstorm, historical record
├── sim/                        # C++17 non-dimensional sizing library
│   ├── include/opp/            # mhd_generator.hpp, lorentz_thruster.hpp, solar_wind.hpp
│   ├── src/main.cpp            # Phase I sizing demo (prints Ha, N, Rm, power, thrust)
│   ├── tests/                  # plain-assert sanity tests, run via ctest
│   └── CMakeLists.txt
├── firmware/                   # Flight-phase FSM (Arduino / PlatformIO)
│   ├── lib/shuttle_fsm/        # ShuttleFSM -- portable C++, no Arduino dependency
│   ├── src/shuttle_fsm.ino     # Arduino Uno entry point (non-blocking, watchdog-guarded)
│   └── test/test_fsm/          # Unity-based logic tests, run natively via `pio test -e native`
├── cad/
│   └── mhd_core_blender.py     # starter geometry: channel, coil wraps, nozzle
├── diagrams/
│   └── mhd-core-diagram.excalidraw
└── platformio.ini
```

## Getting Started

**Sizing library (`sim/`):**
```bash
cmake -S sim -B sim/build -DCMAKE_BUILD_TYPE=Release
cmake --build sim/build
ctest --test-dir sim/build --output-on-failure
./sim/build/opp_sim_demo   # prints Ha / N / Rm / power / thrust for one candidate design point
```

**Firmware (`firmware/`), via [PlatformIO](https://platformio.org/):**
```bash
pip install platformio
pio run -e uno            # build for a real Arduino Uno
pio test -e native         # run the FSM logic tests on your desktop, no hardware needed
```

Flashing to real hardware: `pio run -e uno -t upload` with an Uno connected.

## Where This Fits

- **[`whitepapers`](https://github.com/Overby-Industries/whitepapers)** --
  the physics this repo implements, peer-reviewable, versioned separately
  from the code.
- **[`overby-industries-pwa`](https://github.com/Overby-Industries/overby-industries-pwa)** --
  the public-facing site; its `/aerospace/propulsion` page summarizes this
  work for a general audience.
- This repo -- the sizing tools and flight-control firmware that turn the
  physics into something that can actually be simulated, benchtop-tested,
  and eventually flown.

## Status

Early / conceptual stage (see [`docs/testing-roadmap.md`](docs/testing-roadmap.md)
for what "Phase I" actually means here). The sizing models are first-order
and explicitly not flight-performance-grade -- see
[`docs/physics-basis.md`](docs/physics-basis.md) for the caveats built into
each formula.

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) and [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md).
Engineers, plasma physicists, embedded/controls folks, and CAD contributors
are all useful here -- open an issue or PR.

## License

[Apache 2.0](LICENSE).
