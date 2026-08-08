#pragma once

namespace opp {

// First-order sizing model for the same MHD channel run in accelerator
// (thruster) mode: current driven through the duct produces a J x B body
// force that accelerates the working fluid out the nozzle. See
// papers/2026-mhd-power-propulsion (whitepapers repo), Section 5.
struct LorentzThrusterConfig {
  double currentDensity_Apm2;  // J, driven into the channel (not induced)
  double magneticField_T;      // B
  double crossSectionArea_m2;  // channel cross-section, perpendicular to flow
  double channelLength_m;      // L
};

class LorentzThruster {
 public:
  explicit LorentzThruster(const LorentzThrusterConfig& cfg) : cfg_(cfg) {}

  // Body force density f = J x B, simplified to a scalar product since J
  // and B are orthogonal by construction in this channel geometry (same
  // assumption the generator-mode model uses, just with current driven in
  // rather than induced out).
  double bodyForceDensity_Npm3() const {
    return cfg_.currentDensity_Apm2 * cfg_.magneticField_T;
  }

  // Total thrust: integrate the body force density over the channel
  // volume, assuming uniform J and B across the channel. Real field/
  // current profiles taper toward the nozzle exit, so treat this as an
  // upper bound to be refined against Phase II/III test data (see
  // docs/testing-roadmap.md) rather than a specific-impulse-grade number.
  double thrust_N() const {
    const double volume_m3 = cfg_.crossSectionArea_m2 * cfg_.channelLength_m;
    return bodyForceDensity_Npm3() * volume_m3;
  }

  const LorentzThrusterConfig& config() const { return cfg_; }

 private:
  LorentzThrusterConfig cfg_;
};

}  // namespace opp
