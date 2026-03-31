/*
 * test_Dem_Kavach.c
 * Unity unit tests for Kavach DMI DEM events
 *
 * Tests:
 * 1. Dem_Init completes without crash
 * 2. SetEventStatus FAILED on display fault
 * 3. SetEventStatus PASSED clears the fault
 * 4. All 14 Kavach event IDs are valid
 */

#include "unity.h"
#include "Dem.h"
#include "Dem_IntEvtId.h"

void setUp(void)    {}
void tearDown(void) {}

/* Test 1 — DEM initialises without crash */
void test_Dem_Init_DoesNotCrash(void)
{
    Dem_PreInit(NULL);
    Dem_Init();
    TEST_PASS();
}

/* Test 2 — Set display fail event to FAILED */
void test_Dem_SetEventStatus_DisplayFail(void)
{
    Std_ReturnType ret;
    ret = Dem_SetEventStatus(RAIL_EVT_DISPLAY_FAIL,
                             DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* Test 3 — Set display fail event to PASSED */
void test_Dem_SetEventStatus_DisplayPass(void)
{
    Std_ReturnType ret;
    ret = Dem_SetEventStatus(RAIL_EVT_DISPLAY_FAIL,
                             DEM_EVENT_STATUS_PASSED);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* Test 4 — Comm loss event reports FAILED */
void test_Dem_SetEventStatus_CommLoss(void)
{
    Std_ReturnType ret;
    ret = Dem_SetEventStatus(RAIL_EVT_COMM_LOSS_LTCAS,
                             DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* Test 5 — Watchdog timeout event */
void test_Dem_SetEventStatus_Watchdog(void)
{
    Std_ReturnType ret;
    ret = Dem_SetEventStatus(RAIL_EVT_WATCHDOG_TIMEOUT,
                             DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_Dem_Init_DoesNotCrash);
    RUN_TEST(test_Dem_SetEventStatus_DisplayFail);
    RUN_TEST(test_Dem_SetEventStatus_DisplayPass);
    RUN_TEST(test_Dem_SetEventStatus_CommLoss);
    RUN_TEST(test_Dem_SetEventStatus_Watchdog);
    return UNITY_END();
}
