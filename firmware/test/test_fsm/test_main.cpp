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

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_ground_idle_to_preflight);
  RUN_TEST(test_preflight_enforces_minimum_dwell);
  RUN_TEST(test_takeoff_roll_requires_speed_threshold);
  RUN_TEST(test_mhd_substate_progression_reaches_orbital_cruise);
  RUN_TEST(test_fault_preempts_and_is_sticky);
  return UNITY_END();
}
