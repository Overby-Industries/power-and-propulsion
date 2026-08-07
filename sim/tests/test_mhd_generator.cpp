// Plain-assert sanity tests for the sizing formulas in opp/*.hpp. No test
// framework dependency by design, so `cmake --build . && ctest` works with
// nothing beyond a C++17 compiler.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>

#include "opp/lorentz_thruster.hpp"
#include "opp/mhd_generator.hpp"
#include "opp/solar_wind.hpp"

static bool approxEqual(double a, double b, double relTol = 1e-9) {
  return std::fabs(a - b) <= relTol * std::max(std::fabs(a), std::fabs(b));
}

static void test_hartmann_number_matches_closed_form() {
  opp::MHDGeneratorConfig cfg{};
  cfg.magneticField_T = 2.0;
  cfg.channelLength_m = 0.5;
  cfg.conductivity_Spm = 400.0;
  cfg.dynamicViscosity_Pas = 1.0e-5;
  cfg.channelHeight_m = 0.03;
  cfg.density_kgpm3 = 0.01;
  cfg.velocity_mps = 1000.0;

  opp::MHDGenerator gen(cfg);
  const double expected = 2.0 * 0.5 * std::sqrt(400.0 / 1.0e-5);
  assert(approxEqual(gen.hartmannNumber(), expected));
}

static void test_interaction_parameter_matches_closed_form() {
  opp::MHDGeneratorConfig cfg{};
  cfg.magneticField_T = 1.5;
  cfg.channelLength_m = 0.4;
  cfg.conductivity_Spm = 100.0;
  cfg.density_kgpm3 = 0.01;
  cfg.velocity_mps = 1000.0;
  cfg.dynamicViscosity_Pas = 1.0e-5;
  cfg.channelHeight_m = 0.03;

  opp::MHDGenerator gen(cfg);
  const double expected = 100.0 * 1.5 * 1.5 * 0.4 / (0.01 * 1000.0);
  assert(approxEqual(gen.interactionParameter(), expected));
  // This design point is chosen to land within the paper's target N ~ 1-10.
  assert(gen.interactionParameter() >= 1.0 && gen.interactionParameter() <= 10.0);
}

static void test_effective_conductivity_hall_limits() {
  opp::MHDGeneratorConfig cfg{};
  cfg.conductivity_Spm = 200.0;
  cfg.magneticField_T = 1.0;
  cfg.channelLength_m = 0.3;
  cfg.channelHeight_m = 0.02;
  cfg.density_kgpm3 = 0.01;
  cfg.velocity_mps = 500.0;
  cfg.dynamicViscosity_Pas = 1.0e-5;

  opp::MHDGenerator gen(cfg);
  // beta = 0: no Hall drift, full conductivity available.
  assert(approxEqual(gen.effectiveConductivity(0.0), 200.0));
  // Large beta: current is deflected sideways, useful conductivity collapses.
  assert(gen.effectiveConductivity(50.0) < gen.effectiveConductivity(1.0));
  assert(gen.effectiveConductivity(50.0) < 1.0);
}

static void test_lorentz_thrust_scales_with_current_and_field() {
  opp::LorentzThrusterConfig cfg{};
  cfg.currentDensity_Apm2 = 1.0e4;
  cfg.magneticField_T = 1.0;
  cfg.crossSectionArea_m2 = 0.003;
  cfg.channelLength_m = 0.4;

  opp::LorentzThruster baseline(cfg);
  const double baselineThrust = baseline.thrust_N();

  opp::LorentzThrusterConfig doubledField = cfg;
  doubledField.magneticField_T = 2.0;
  opp::LorentzThruster doubled(doubledField);

  // f = J x B is linear in B, so doubling B should double the thrust.
  assert(approxEqual(doubled.thrust_N(), 2.0 * baselineThrust));
}

static void test_solar_wind_power_is_trickle_scale() {
  opp::SolarWindConfig cfg{};
  cfg.numberDensity_per_m3 = 7.0e6;
  cfg.velocity_mps = 400000.0;
  cfg.collectorArea_m2 = 100.0;
  cfg.collectionEfficiency = 0.3;
  cfg.particleMass_kg = opp::kProtonMass_kg;

  opp::SolarWindCollector collector(cfg);

  // Dynamic pressure should land in the realistic 1-3 nPa band the paper
  // cites for 1 AU.
  const double q_nPa = collector.dynamicPressure_Pa() * 1.0e9;
  assert(q_nPa > 0.5 && q_nPa < 5.0);

  // A 100 m^2 collector at these parameters should deliver microwatts, not
  // watts -- if this ever comes out >= 1 W, something in the model (or the
  // inputs) is off by orders of magnitude.
  assert(collector.availablePower_W() < 1.0);
}

int main() {
  test_hartmann_number_matches_closed_form();
  test_interaction_parameter_matches_closed_form();
  test_effective_conductivity_hall_limits();
  test_lorentz_thrust_scales_with_current_and_field();
  test_solar_wind_power_is_trickle_scale();
  std::puts("All opp sizing-model tests passed.");
  return 0;
}
