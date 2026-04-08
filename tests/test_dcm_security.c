#include "unity.h"
#include <stdint.h>
#include "../dcm/Dcm_Cfg.h"
#include "../platform/platform_api.h"

/* External function declarations to link to your core files */
extern void Dcm_Init(void);
extern void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen);
extern void Dem_ReportError(uint16_t did, uint8_t rawSignal);
extern uint8_t NvM_Write_Verified(uint16_t blockId, uint8_t* data, uint16_t size);
extern void Mock_Force_EEPROM_Stuck_Bit(uint16_t blockId);

/* =========================================================================
 * CORE SECURITY & NvM TESTS (From Doc A)
 * ========================================================================= */

void test_Dcm_Write_Fails_Without_Security_Unlock(void) {
    uint8_t req[] = {0x2E, 0xF3, 0x01}; /* Write DID */
    uint8_t resp[10];
    uint16_t respLen = 0;

    /* Step 1: Switch to Programming Session (10 03) */
    uint8_t sessionReq[] = {0x10, 0x03};
    Dcm_MainFunction(sessionReq, 2, resp, &respLen);
    
    /* Step 2: Attempt Write WITHOUT Security Unlock */
    Dcm_MainFunction(req, 3, resp, &respLen);

    /* Assert we get NRC 0x33 (Security Access Denied) */
    TEST_ASSERT_EQUAL_HEX8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x2E, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0x33, resp[2]); 
}

void test_Dem_Nvm_ReadBack_Verify_Detects_Corruption(void) {
    uint8_t testData[16] = {0xAA, 0xBB, 0xCC};
    
    /* Intentionally break the mock EEPROM to simulate a hardware failure */
    Mock_Force_EEPROM_Stuck_Bit(0x01); 
    
    /* Attempt to write */
    uint8_t result = NvM_Write_Verified(0x01, testData, 16);
    
    /* Assert the system caught the error and returned PLATFORM_NOT_OK */
    TEST_ASSERT_EQUAL_UINT8(PLATFORM_NOT_OK, result);
}

/* =========================================================================
 * UNIQUE DOC B TEST CASES (NRC Engine & Timing Guards)
 * ========================================================================= */

void test_NRC_0x13_IncorrectLength(void) {
    uint8_t req[] = {0x22, 0xF1}; /* Missing 1 byte */
    uint8_t resp[10]; uint16_t respLen = 0;
    Dcm_MainFunction(req, 2, resp, &respLen);
    TEST_ASSERT_EQUAL_HEX8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x13, resp[2]); 
}

void test_NRC_0x31_ROOR_InvalidDID(void) {
    uint8_t req[] = {0x22, 0x99, 0x99}; /* Non-existent DID */
    uint8_t resp[10]; uint16_t respLen = 0;
    Dcm_MainFunction(req, 3, resp, &respLen);
    TEST_ASSERT_EQUAL_HEX8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x31, resp[2]);
}

void test_NRC_0x31_ROOR_SessionViolation(void) {
    uint8_t req[] = {0x22, 0xF2, 0x00}; /* Session 2 DID */
    uint8_t resp[10]; uint16_t respLen = 0;
    /* Ensure we are in Default Session (0x01) */
    Dcm_Init(); 
    Dcm_MainFunction(req, 3, resp, &respLen);
    TEST_ASSERT_EQUAL_HEX8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x31, resp[2]);
}

void test_NRC_0x11_ServiceNotSupported(void) {
    uint8_t req[] = {0x99, 0x00, 0x00}; /* Undefined Service */
    uint8_t resp[10]; uint16_t respLen = 0;
    Dcm_MainFunction(req, 3, resp, &respLen);
    TEST_ASSERT_EQUAL_HEX8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x11, resp[2]);
}

void test_NvM_WriteReadBack_Success(void) {
    uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};
    uint8_t res = NvM_Write_Verified(0x05, data, 4);
    TEST_ASSERT_EQUAL_UINT8(PLATFORM_OK, res);
}

void test_NvM_ReadBack_Failure_Trigger_0xF10B(void) {
    uint8_t data[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    Mock_Force_EEPROM_Stuck_Bit(0x06); /* Force a mismatch */
    uint8_t res = NvM_Write_Verified(0x06, data, 4);
    TEST_ASSERT_EQUAL_UINT8(PLATFORM_NOT_OK, res);
}

void test_NvM_5ms_Timing_Guard(void) {
    uint32_t start = Platform_GetTick_ms();
    uint8_t data[1] = {0xAA};
    NvM_Write_Verified(0x07, data, 1);
    uint32_t end = Platform_GetTick_ms();
    /* Verify at least 5ms passed during the function call */
    TEST_ASSERT_TRUE((end - start) >= 5);
}

void test_Dem_Debounce_CRC_Bypass(void) {
    /* DID 0xF107 should log FAIL immediately without 50ms/100ms debounce */
    Dem_ReportError(0xF107, 1); 
    /* Verify bit 0 (TF) is set immediately */
    // Note: This requires an internal check of EventStatus[0xF107 % 20]
    // which would typically be exposed via an internal getter function for testing.
}
/* =========================================================================
 * UNITY TEST RUNNER (MAIN)
 * ========================================================================= */

/* Unity requires these to be defined, even if empty */
void setUp(void) {
    /* Optional: Reset session or mock memory before each test */
    Dcm_Init(); 
}

void tearDown(void) {
    /* Optional: Clean up after each test */
}

int main(void) {
    UNITY_BEGIN();

    /* Core Security & NvM Tests */
    RUN_TEST(test_Dcm_Write_Fails_Without_Security_Unlock);
    RUN_TEST(test_Dem_Nvm_ReadBack_Verify_Detects_Corruption);

    /* Doc B Unique Test Cases */
    RUN_TEST(test_NRC_0x13_IncorrectLength);
    RUN_TEST(test_NRC_0x31_ROOR_InvalidDID);
    RUN_TEST(test_NRC_0x31_ROOR_SessionViolation);
    RUN_TEST(test_NRC_0x11_ServiceNotSupported);
    RUN_TEST(test_NvM_WriteReadBack_Success);
    RUN_TEST(test_NvM_ReadBack_Failure_Trigger_0xF10B);
    RUN_TEST(test_NvM_5ms_Timing_Guard);
    RUN_TEST(test_Dem_Debounce_CRC_Bypass);

    /* Note: Add any other RUN_TEST() macros here for the rest of your 57 tests */

    return UNITY_END();
}
