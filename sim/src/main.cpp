// Phase I sizing demo (see docs/testing-roadmap.md): explores one candidate
// benchtop-scale MHD generator design point and checks it against the
// non-dimensional targets from papers/2026-mhd-power-propulsion (Ha >> 1,
// N ~ 1-10, Rm << 1), plus the solar-wind trickle-charge estimate.
//
// These example numbers are ONE self-consistent point in the design space,
// not a claim that any benchtop core will be built to exactly these specs
// -- finding a good point is what this tool is for. Change the values
// below and re-run to explore the trade space.
#include <iostream>

#include "opp/lorentz_thruster.hpp"
#include "opp/mhd_generator.hpp"
#include "opp/solar_wind.hpp"

int main() {
  // --- MHD generator: one candidate benchtop design point ---
  opp::MHDGeneratorConfig genCfg{};
  genCfg.magneticField_T = 1.5;         // within the 0.5-3 T target range
  genCfg.channelLength_m = 0.4;         // within the 20-80 cm target range
  genCfg.channelHeight_m = 0.03;        // within the 1-5 cm target range
  genCfg.conductivity_Spm = 100.0;      // within the 100-1000 S/m target range
  genCfg.density_kgpm3 = 0.01;          // low-pressure seeded vapor, not free-stream plasma
  genCfg.velocity_mps = 1000.0;
  genCfg.dynamicViscosity_Pas = 1.0e-5; // typical low-density gas

  const double channelWidth_m = 0.1;    // within the 5-15 cm target range
  const double crossSectionArea_m2 = genCfg.channelHeight_m * channelWidth_m;
  const double hallParameter = 0.5;      // moderate Hall drift, needs segmented electrodes

  opp::MHDGenerator generator(genCfg);

  std::cout << "=== MHD Generator (benchtop candidate point) ===\n";
  std::cout << "Hartmann number Ha:       " << generator.hartmannNumber()
            << "  (target: >> 1)\n";
  std::cout << "Interaction parameter N:  " << generator.interactionParameter()
            << "  (target: 1-10)\n";
  std::cout << "Magnetic Reynolds Rm:     " << generator.magneticReynoldsNumber()
            << "  (target: << 1)\n";
  std::cout << "Ideal Faraday EMF (V):    " << generator.idealFaradayVoltage_V() << "\n";
  std::cout << "Est. current density:    "
            << generator.estimatedCurrentDensity_Apm2(hallParameter) / 1.0e4
            << " A/cm^2  (electrode limit: 1-20 A/cm^2)\n";
  std::cout << "Est. power (upper bound): "
            << generator.estimatedPower_W(hallParameter, crossSectionArea_m2) / 1000.0
            << " kW  (total resistive dissipation, NOT a matched-load design number)\n\n";

  // --- Same channel, reconfigured as a Lorentz thruster ---
  opp::LorentzThrusterConfig thrCfg{};
  thrCfg.currentDensity_Apm2 = generator.estimatedCurrentDensity_Apm2(hallParameter);
  thrCfg.magneticField_T = genCfg.magneticField_T;
  thrCfg.crossSectionArea_m2 = crossSectionArea_m2;
  thrCfg.channelLength_m = genCfg.channelLength_m;

  opp::LorentzThruster thruster(thrCfg);
  std::cout << "=== Same channel, Lorentz thruster mode ===\n";
  std::cout << "Body force density:       " << thruster.bodyForceDensity_Npm3() << " N/m^3\n";
  std::cout << "Estimated thrust (upper bound): " << thruster.thrust_N() << " N\n\n";

  // --- Solar wind trickle-charge collector, 1 AU nominal ---
  opp::SolarWindConfig swCfg{};
  swCfg.numberDensity_per_m3 = 7.0e6;  // ~7 /cm^3, mid-range at 1 AU
  swCfg.velocity_mps = 400000.0;       // 400 km/s nominal
  swCfg.collectorArea_m2 = 100.0;      // a modest 100 m^2 collector
  swCfg.collectionEfficiency = 0.3;
  swCfg.particleMass_kg = opp::kProtonMass_kg;

  opp::SolarWindCollector collector(swCfg);
  std::cout << "=== Solar wind trickle-charge collector (1 AU, 100 m^2) ===\n";
  std::cout << "Dynamic pressure:         " << collector.dynamicPressure_Pa() * 1.0e9
            << " nPa  (expect ~1-3 nPa)\n";
  std::cout << "Available power:          " << collector.availablePower_W() * 1.0e6
            << " uW  (trickle-charge only -- not primary propulsion power)\n";

  return 0;
}
