#pragma once

namespace opp {

// Order-of-magnitude sizing for a solar-wind trickle-charge collector.
// Deliberately conservative -- see papers/2026-mhd-power-propulsion,
// Section 6 (whitepapers repo): this is a slow bus trickle-charge source,
// not primary propulsion power, and the numbers below should come out in
// the nanowatt-to-microwatt-per-square-meter range for realistic 1 AU
// parameters, not watts.
struct SolarWindConfig {
  double numberDensity_per_m3;  // n; ~3-10 /cm^3 -> 3e6-1e7 /m^3 at 1 AU
  double velocity_mps;          // v; ~400 km/s nominal at 1 AU
  double collectorArea_m2;      // effective collection area
  double collectionEfficiency;  // 0..1, fraction of incident kinetic power captured
  double particleMass_kg;       // dominant species mass (protons by default)
};

// Proton mass, kg -- the dominant solar-wind species by number.
constexpr double kProtonMass_kg = 1.6726219e-27;

class SolarWindCollector {
 public:
  explicit SolarWindCollector(const SolarWindConfig& cfg) : cfg_(cfg) {}

  // Mass density of the flow: rho = n * m.
  double massDensity_kgpm3() const {
    return cfg_.numberDensity_per_m3 * cfg_.particleMass_kg;
  }

  // Dynamic pressure: q = rho * v^2. Realistic 1 AU parameters give
  // roughly 1-3 nPa.
  double dynamicPressure_Pa() const {
    return massDensity_kgpm3() * cfg_.velocity_mps * cfg_.velocity_mps;
  }

  // Available kinetic power flux through the collector:
  // P = 0.5 * rho * v^3 * A * efficiency.
  double availablePower_W() const {
    return 0.5 * massDensity_kgpm3() * cfg_.velocity_mps * cfg_.velocity_mps *
           cfg_.velocity_mps * cfg_.collectorArea_m2 * cfg_.collectionEfficiency;
  }

  const SolarWindConfig& config() const { return cfg_; }

 private:
  SolarWindConfig cfg_;
};

}  // namespace opp
