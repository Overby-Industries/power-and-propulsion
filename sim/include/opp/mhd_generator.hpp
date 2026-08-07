#pragma once
#include <cmath>

namespace opp {

// First-order sizing model for a Faraday/Hall MHD generator channel.
//
// Reference: Sutton & Sherman, "Engineering Magnetohydrodynamics" (1965) --
// see papers/2026-mhd-power-propulsion in the whitepapers repo for the
// physics writeup this implements against (Section 4). This is a
// benchtop/conceptual sizing tool for Phase I simulation work, not a
// flight-qualified performance model.
struct MHDGeneratorConfig {
  double magneticField_T;       // B
  double channelLength_m;       // L, along the flow direction
  double channelHeight_m;       // h, electrode gap (Faraday direction)
  double conductivity_Spm;      // sigma, plasma conductivity
  double density_kgpm3;         // rho, working-fluid mass density
  double velocity_mps;          // v, bulk flow velocity
  double dynamicViscosity_Pas;  // mu
};

class MHDGenerator {
 public:
  explicit MHDGenerator(const MHDGeneratorConfig& cfg) : cfg_(cfg) {}

  // Hartmann number: Ha = B*L*sqrt(sigma/mu). Target Ha >> 1: the magnetic
  // field dominates viscous forces, giving real MHD control authority.
  double hartmannNumber() const {
    return cfg_.magneticField_T * cfg_.channelLength_m *
           std::sqrt(cfg_.conductivity_Spm / cfg_.dynamicViscosity_Pas);
  }

  // Interaction parameter: N = sigma*B^2*L / (rho*v). Target N ~ 1-10:
  // efficient energy conversion without choking the flow.
  double interactionParameter() const {
    return cfg_.conductivity_Spm * cfg_.magneticField_T * cfg_.magneticField_T *
           cfg_.channelLength_m / (cfg_.density_kgpm3 * cfg_.velocity_mps);
  }

  // Magnetic Reynolds number: Rm = mu0*sigma*v*L. Target Rm << 1: B can be
  // treated as externally imposed rather than needing to account for the
  // plasma's own induced field.
  double magneticReynoldsNumber() const {
    // Permeability of free space, H/m (4*pi x 1e-7 exactly, in the classical
    // SI definition still standard for engineering use). Spelled out as a
    // literal rather than via M_PI, which isn't standard C++ and MSVC
    // doesn't define without a non-portable macro.
    constexpr double kMu0 = 1.25663706143591729e-6;
    return kMu0 * cfg_.conductivity_Spm * cfg_.velocity_mps * cfg_.channelLength_m;
  }

  // Ideal (Hall-free, lossless) Faraday EMF across the electrode gap:
  // E = v * B * h.
  double idealFaradayVoltage_V() const {
    return cfg_.velocity_mps * cfg_.magneticField_T * cfg_.channelHeight_m;
  }

  // Effective conductivity along the load direction once Hall drift is
  // accounted for: sigma_eff = sigma / (1 + beta^2), where beta is the Hall
  // parameter (omega_c * tau, the cyclotron-frequency/collision-time
  // product). beta=0 recovers the ideal conductivity; large beta collapses
  // useful output because current is deflected sideways instead of into
  // the load -- this is why electrode segmentation (Hall generator
  // topology) matters at high beta.
  double effectiveConductivity(double hallParameter) const {
    return cfg_.conductivity_Spm / (1.0 + hallParameter * hallParameter);
  }

  // Order-of-magnitude electrical power implied by the induced field: the
  // Ohmic dissipation P = sigma_eff * E^2 integrated over the channel
  // volume, with E = v*B.
  //
  // This is total resistive dissipation, NOT a matched-load result -- the
  // fraction actually delivered to an external load vs. dropped across
  // internal/electrode resistance depends on a circuit split this
  // simplified model does not resolve. Treat it as an upper-bound sizing
  // estimate for Phase I exploration (see docs/testing-roadmap.md), not a
  // number to design hardware against directly.
  double estimatedPower_W(double hallParameter, double crossSectionArea_m2) const {
    const double sigmaEff = effectiveConductivity(hallParameter);
    const double inducedField_Vpm = cfg_.velocity_mps * cfg_.magneticField_T;
    const double volume_m3 = crossSectionArea_m2 * cfg_.channelLength_m;
    return sigmaEff * inducedField_Vpm * inducedField_Vpm * volume_m3;
  }

  // Current density implied by the same Ohmic model: J = sigma_eff * E.
  // Compare against the electrode current-density limit (1-20 A/cm^2 per
  // the whitepaper) to sanity-check whether a design point is electrode-
  // erosion-plausible, not just non-dimensionally sound.
  double estimatedCurrentDensity_Apm2(double hallParameter) const {
    return effectiveConductivity(hallParameter) * cfg_.velocity_mps * cfg_.magneticField_T;
  }

  const MHDGeneratorConfig& config() const { return cfg_; }

 private:
  MHDGeneratorConfig cfg_;
};

}  // namespace opp
