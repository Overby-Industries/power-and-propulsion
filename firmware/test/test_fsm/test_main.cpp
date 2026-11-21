// Host-native logic tests for ShuttleFSM. Run with `pio test -e native` --
// no Arduino hardware required. PlatformIO's LDF pulls in
// firmware/lib/shuttle_fsm automatically because this file includes
// ShuttleFSM.h.
#include <unity.h>
#include "ShuttleFSM.h"

void setUp(void) {}
void tearDown(void) {}

static void test_ground_idle_to_preflight(void) {
  ShuttleFSM fsm;
  fsm.begin(0);
  TEST_ASSERT_EQUAL(GroundIdle, fsm.state());

  ShuttleSensors s;
  s.preflightChecksPassed = false;
  fsm.update(s, 10);
  TEST_ASSERT_EQUAL(GroundIdle, fsm.state());

  s.preflightChecksPassed = true;
  fsm.update(s, 20);
  TEST_ASSERT_EQUAL(PreFlightChecks, fsm.state());
}

static void test_preflight_enforces_minimum_dwell(void) {
  ShuttleFSM fsm;
  fsm.begin(0);
  ShuttleSensors s;
  s.preflightChecksPassed = true;

  fsm.update(s, 0);
  TEST_ASSERT_EQUAL(PreFlightChecks, fsm.state());

  fsm.update(s, 500); // short of the minimum dwell
  TEST_ASSERT_EQUAL(PreFlightChecks, fsm.state());

  fsm.update(s, 2000); // past the minimum dwell
  TEST_ASSERT_EQUAL(TakeoffRoll, fsm.state());
}

static void test_takeoff_roll_requires_speed_threshold(void) {
  ShuttleFSM fsm;
  fsm.begin(0);
  ShuttleSensors s;
  s.preflightChecksPassed = true;
  fsm.update(s, 0);
  fsm.update(s, 2000);
  TEST_ASSERT_EQUAL(TakeoffRoll, fsm.state());

  s.airspeed_mps = kTakeoffSpeedThreshold_mps - 1.0f;
  fsm.update(s, 2100);
  TEST_ASSERT_EQUAL(TakeoffRoll, fsm.state());

  s.airspeed_mps = kTakeoffSpeedThreshold_mps + 1.0f;
  fsm.update(s, 2200);
  TEST_ASSERT_EQUAL(AscentAtmospheric, fsm.state());
}

static void test_mhd_substate_progression_reaches_orbital_cruise(void) {
  ShuttleFSM fsm;
  fsm.begin(0);
  ShuttleSensors s;
  s.preflightChecksPassed = true;
  fsm.update(s, 0);
  fsm.update(s, 2000);
  s.airspeed_mps = kTakeoffSpeedThreshold_mps + 1.0f;
  fsm.update(s, 2100);
  s.machNumber = 6.0f;
  s.altitude_m = 31000.0f;
  fsm.update(s, 2200);
  TEST_ASSERT_EQUAL(HypersonicTransition, fsm.state());

  s.mhdTransitionReady = true;
  fsm.update(s, 2300);
  TEST_ASSERT_EQUAL(MHD_Activation, fsm.state());
  TEST_ASSERT_EQUAL(PlasmaIgnition, fsm.mhdSubState());

  s.plasmaStable = true;
  fsm.update(s, 2400);
  TEST_ASSERT_EQUAL(MHD_Activation, fsm.state());
  TEST_ASSERT_EQUAL(FieldRamp, fsm.mhdSubState());

  // Reaching ThrustStabilize hands off to OrbitalCruise within the same
  // tick -- the sub-state and the top-level transition are evaluated in one
  // update() call, not staggered across two.
  s.fieldAtTarget = true;
  fsm.update(s, 2500);
  TEST_ASSERT_EQUAL(OrbitalCruise, fsm.state());
}

static void test_fault_preempts_and_is_sticky(void) {
  ShuttleFSM fsm;
  fsm.begin(0);
  ShuttleSensors s;
  s.preflightChecksPassed = true;
  fsm.update(s, 0);
  fsm.update(s, 2000);
  TEST_ASSERT_EQUAL(TakeoffRoll, fsm.state());

  s.faultDetected = true;
  fsm.update(s, 2100);
  TEST_ASSERT_EQUAL(AbortSafeMode, fsm.state());

  // Clearing the flag does not recover the vehicle -- that's a deliberate,
  // ground-commanded action, never automatic.
  s.faultDetected = false;
  s.airspeed_mps = 999.0f;
  fsm.update(s, 2200);
  TEST_ASSERT_EQUAL(AbortSafeMode, fsm.state());
}


// ---------------------------------------------------------------------------
// Ground commands
// ---------------------------------------------------------------------------

// Drive a fresh FSM into `target` by the same routes a real vehicle takes:
// sensor readings through the flight phases, then ground commands for the
// phases that are ground-commanded. Returns the time of the last step.
static uint32_t driveTo(ShuttleFSM& fsm, ShuttleState target) {
  uint32_t t = 0;
  fsm.begin(t);
  if (target == GroundIdle) return t;

  if (target == AbortSafeMode) {
    ShuttleSensors fault;
    fault.faultDetected = true;
    fsm.update(fault, t += 100);
    return t;
  }

  ShuttleSensors s;
  s.preflightChecksPassed = true;
  fsm.update(s, t);                               // -> PreFlightChecks
  if (fsm.state() == target) return t;
  fsm.update(s, t += 2000);                       // -> TakeoffRoll
  if (fsm.state() == target) return t;
  s.airspeed_mps = kTakeoffSpeedThreshold_mps + 1.0f;
  fsm.update(s, t += 100);                        // -> AscentAtmospheric
  if (fsm.state() == target) return t;
  s.machNumber = 6.0f;
  s.altitude_m = 31000.0f;
  fsm.update(s, t += 100);                        // -> HypersonicTransition
  if (fsm.state() == target) return t;
  s.mhdTransitionReady = true;
  fsm.update(s, t += 100);                        // -> MHD_Activation
  if (fsm.state() == target) return t;
  s.plasmaStable = true;
  fsm.update(s, t += 100);
  s.fieldAtTarget = true;
  fsm.update(s, t += 100);                        // -> OrbitalCruise
  if (fsm.state() == target) return t;

  ShuttleSensors clean;
  fsm.command(CmdDeployPayload, clean, t += 100);     // -> PayloadDeployment
  if (fsm.state() == target) return t;
  fsm.command(CmdBeginReentry, clean, t += 100);      // -> Reentry
  if (fsm.state() == target) return t;
  fsm.command(CmdBeginApproach, clean, t += 100);     // -> ApproachAndLanding
  if (fsm.state() == target) return t;
  fsm.command(CmdLandingComplete, clean, t += 100);   // -> Maintenance
  return t;
}

static void test_drive_helper_reaches_every_state(void) {
  for (int state = 0; state <= Maintenance; ++state) {
    ShuttleFSM fsm;
    driveTo(fsm, static_cast<ShuttleState>(state));
    TEST_ASSERT_EQUAL(state, fsm.state());
  }
}

static void test_the_documented_chain_runs_end_to_end(void) {
  ShuttleFSM fsm;
  uint32_t t = driveTo(fsm, OrbitalCruise);
  ShuttleSensors clean;

  TEST_ASSERT_TRUE(fsm.command(CmdDeployPayload, clean, t += 100));
  TEST_ASSERT_EQUAL(PayloadDeployment, fsm.state());
  TEST_ASSERT_TRUE(fsm.command(CmdBeginReentry, clean, t += 100));
  TEST_ASSERT_EQUAL(Reentry, fsm.state());
  TEST_ASSERT_TRUE(fsm.command(CmdBeginApproach, clean, t += 100));
  TEST_ASSERT_EQUAL(ApproachAndLanding, fsm.state());
  TEST_ASSERT_TRUE(fsm.command(CmdLandingComplete, clean, t += 100));
  TEST_ASSERT_EQUAL(Maintenance, fsm.state());
  TEST_ASSERT_TRUE(fsm.command(CmdReturnToIdle, clean, t += 100));
  TEST_ASSERT_EQUAL(GroundIdle, fsm.state());
}

// Every command is valid from exactly one state and ignored from the other
// eleven. Exhaustive: 6 commands x 12 states.
static void test_each_command_acts_from_exactly_one_state(void) {
  struct Rule { GroundCommand cmd; ShuttleState from; ShuttleState to; };
  const Rule rules[] = {
    {CmdDeployPayload,   OrbitalCruise,      PayloadDeployment},
    {CmdBeginReentry,    PayloadDeployment,  Reentry},
    {CmdBeginApproach,   Reentry,            ApproachAndLanding},
    {CmdLandingComplete, ApproachAndLanding, Maintenance},
    {CmdRecover,         AbortSafeMode,      Maintenance},
    {CmdReturnToIdle,    Maintenance,        GroundIdle},
  };

  ShuttleSensors clean;
  for (unsigned r = 0; r < sizeof(rules) / sizeof(rules[0]); ++r) {
    for (int state = 0; state <= Maintenance; ++state) {
      ShuttleFSM fsm;
      uint32_t t = driveTo(fsm, static_cast<ShuttleState>(state));
      const bool valid = (state == rules[r].from);
      const bool changed = fsm.command(rules[r].cmd, clean, t + 100);
      TEST_ASSERT_EQUAL(valid, changed);
      TEST_ASSERT_EQUAL(valid ? rules[r].to : static_cast<ShuttleState>(state), fsm.state());
    }
  }
}

static void test_cmdnone_and_unknown_commands_do_nothing(void) {
  ShuttleSensors clean;
  for (int state = 0; state <= Maintenance; ++state) {
    ShuttleFSM fsm;
    uint32_t t = driveTo(fsm, static_cast<ShuttleState>(state));
    TEST_ASSERT_FALSE(fsm.command(CmdNone, clean, t + 100));
    TEST_ASSERT_FALSE(fsm.command(static_cast<GroundCommand>(200), clean, t + 100));
    TEST_ASSERT_EQUAL(state, fsm.state());
  }
}

static void test_a_held_command_cannot_chain_phases(void) {
  // A caller that keeps sending the same command (a stuck ground-station
  // key, a retried packet) must not carry the vehicle through phases it did
  // not mean to skip.
  ShuttleFSM fsm;
  uint32_t t = driveTo(fsm, OrbitalCruise);
  ShuttleSensors clean;
  TEST_ASSERT_TRUE(fsm.command(CmdDeployPayload, clean, t += 100));
  for (int i = 0; i < 5; ++i) {
    TEST_ASSERT_FALSE(fsm.command(CmdDeployPayload, clean, t += 100));
    TEST_ASSERT_EQUAL(PayloadDeployment, fsm.state());
  }
}

static void test_a_standing_fault_outranks_every_command(void) {
  ShuttleSensors fault;
  fault.faultDetected = true;
  for (int cmd = CmdDeployPayload; cmd <= CmdReturnToIdle; ++cmd) {
    for (int state = 0; state <= Maintenance; ++state) {
      ShuttleFSM fsm;
      uint32_t t = driveTo(fsm, static_cast<ShuttleState>(state));
      TEST_ASSERT_FALSE(fsm.command(static_cast<GroundCommand>(cmd), fault, t + 100));
      TEST_ASSERT_EQUAL(state, fsm.state());
    }
  }

  // And the fault still does its own job on the next update.
  ShuttleFSM fsm;
  uint32_t t = driveTo(fsm, OrbitalCruise);
  TEST_ASSERT_FALSE(fsm.command(CmdDeployPayload, fault, t += 100));
  fsm.update(fault, t += 100);
  TEST_ASSERT_EQUAL(AbortSafeMode, fsm.state());
}

static void test_recovery_is_commanded_never_automatic_and_never_into_flight(void) {
  ShuttleFSM fsm;
  uint32_t t = driveTo(fsm, OrbitalCruise);
  ShuttleSensors fault;
  fault.faultDetected = true;
  fsm.update(fault, t += 100);
  TEST_ASSERT_EQUAL(AbortSafeMode, fsm.state());

  // The fault clearing does not recover the vehicle, however long it stays
  // clear and whatever the sensors then say.
  ShuttleSensors clearAndWellPositioned;
  clearAndWellPositioned.machNumber = 6.0f;
  clearAndWellPositioned.altitude_m = 31000.0f;
  clearAndWellPositioned.preflightChecksPassed = true;
  for (int i = 0; i < 10; ++i) fsm.update(clearAndWellPositioned, t += 1000);
  TEST_ASSERT_EQUAL(AbortSafeMode, fsm.state());

  // Recovery over a fault that is still there is refused.
  TEST_ASSERT_FALSE(fsm.command(CmdRecover, fault, t += 100));
  TEST_ASSERT_EQUAL(AbortSafeMode, fsm.state());

  // Once it is clear, the command takes the vehicle to Maintenance -- not to
  // GroundIdle, and not back to the phase it was in.
  TEST_ASSERT_TRUE(fsm.command(CmdRecover, clearAndWellPositioned, t += 100));
  TEST_ASSERT_EQUAL(Maintenance, fsm.state());

  // Maintenance does not leave on its own either; a second command is needed.
  for (int i = 0; i < 10; ++i) fsm.update(clearAndWellPositioned, t += 1000);
  TEST_ASSERT_EQUAL(Maintenance, fsm.state());
  TEST_ASSERT_TRUE(fsm.command(CmdReturnToIdle, clearAndWellPositioned, t += 100));
  TEST_ASSERT_EQUAL(GroundIdle, fsm.state());
}

static void test_recovered_vehicle_can_fly_again_only_through_preflight(void) {
  ShuttleFSM fsm;
  uint32_t t = driveTo(fsm, AbortSafeMode);
  ShuttleSensors clean;
  fsm.command(CmdRecover, clean, t += 100);
  fsm.command(CmdReturnToIdle, clean, t += 100);
  TEST_ASSERT_EQUAL(GroundIdle, fsm.state());

  // Back at GroundIdle the ordinary entry applies: nothing skips preflight.
  fsm.update(clean, t += 100);
  TEST_ASSERT_EQUAL(GroundIdle, fsm.state());
  ShuttleSensors ready;
  ready.preflightChecksPassed = true;
  fsm.update(ready, t += 100);
  TEST_ASSERT_EQUAL(PreFlightChecks, fsm.state());
}

static void test_update_alone_never_takes_a_commanded_step(void) {
  // The ground-commanded phases have no sensor trigger. Whatever the sensors
  // say -- including readings that look like the NEXT phase, which is exactly
  // where a sensor threshold would be added -- update() leaves them where
  // they are.
  struct Profile { float airspeed; float altitude; float mach; };
  const Profile profiles[] = {
    {0.0f, 0.0f, 0.0f},            // defaults: a vehicle at rest on the ground
    {999.0f, 400000.0f, 25.0f},    // everything high
    {70.0f, 0.0f, 0.2f},           // a landing: low, slow, on the runway
    {250.0f, 5000.0f, 0.5f},       // an approach
    {600.0f, 20000.0f, 2.0f},      // descending through the supersonic regime
    {7800.0f, 60000.0f, 20.0f},    // a reentry corridor
    {7600.0f, 400000.0f, 0.0f},    // orbit
  };

  const ShuttleState commanded[] = {OrbitalCruise, PayloadDeployment, Reentry,
                                    ApproachAndLanding, Maintenance};
  for (unsigned i = 0; i < sizeof(commanded) / sizeof(commanded[0]); ++i) {
    for (unsigned p = 0; p < sizeof(profiles) / sizeof(profiles[0]); ++p) {
      ShuttleFSM fsm;
      uint32_t t = driveTo(fsm, commanded[i]);

      ShuttleSensors s;
      s.airspeed_mps = profiles[p].airspeed;
      s.altitude_m = profiles[p].altitude;
      s.machNumber = profiles[p].mach;
      // Every "ready" flag asserted: nothing a sensor can report is withheld.
      s.preflightChecksPassed = true;
      s.mhdTransitionReady = true;
      s.plasmaStable = true;
      s.fieldAtTarget = true;
      for (int k = 0; k < 20; ++k) fsm.update(s, t += 1000);
      TEST_ASSERT_EQUAL(commanded[i], fsm.state());
    }
  }
}

static void test_command_and_state_names_are_defined(void) {
  TEST_ASSERT_EQUAL_STRING("CmdDeployPayload", groundCommandName(CmdDeployPayload));
  TEST_ASSERT_EQUAL_STRING("CmdRecover", groundCommandName(CmdRecover));
  TEST_ASSERT_EQUAL_STRING("CmdNone", groundCommandName(CmdNone));
  TEST_ASSERT_EQUAL_STRING("Unknown", groundCommandName(static_cast<GroundCommand>(99)));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_ground_idle_to_preflight);
  RUN_TEST(test_preflight_enforces_minimum_dwell);
  RUN_TEST(test_takeoff_roll_requires_speed_threshold);
  RUN_TEST(test_mhd_substate_progression_reaches_orbital_cruise);
  RUN_TEST(test_fault_preempts_and_is_sticky);
  RUN_TEST(test_drive_helper_reaches_every_state);
  RUN_TEST(test_the_documented_chain_runs_end_to_end);
  RUN_TEST(test_each_command_acts_from_exactly_one_state);
  RUN_TEST(test_cmdnone_and_unknown_commands_do_nothing);
  RUN_TEST(test_a_held_command_cannot_chain_phases);
  RUN_TEST(test_a_standing_fault_outranks_every_command);
  RUN_TEST(test_recovery_is_commanded_never_automatic_and_never_into_flight);
  RUN_TEST(test_recovered_vehicle_can_fly_again_only_through_preflight);
  RUN_TEST(test_update_alone_never_takes_a_commanded_step);
  RUN_TEST(test_command_and_state_names_are_defined);
  return UNITY_END();
}
