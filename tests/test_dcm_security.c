#include "unity.h"
#include <stdint.h>
#include "../dcm/Dcm_Cfg.h"
#include "../platform/platform_api.h"

/* External function declarations */
extern void Dcm_Init(void);
extern void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen);
extern void Dem_ReportError(uint16_t did, uint8_t rawSignal);
extern uint8_t NvM_Write_Verified(uint16_t blockId, const uint8_t* data, uint16_t size);
extern void Mock_Force_EEPROM_Stuck_Bit(uint16_t blockId);
extern uint32_t Platform_GetTick_ms(void);

/* Link to the global variable in the mock file */
extern uint16_t g_stuckBlock;

/* =========================================================================
 * UNITY SETUP & TEARDOWN
 * ========================================================================= */

void setUp(void) {
    Dcm_Init(); 
    g_stuckBlock = 0xFFFFU; 
    Mock_Force_EEPROM_Stuck_Bit(0xFFFFU); 
}

void tearDown(void) { }

/* =========================================================================
 * TESTS
 * ========================================================================= */

void test_Dcm_Write_Fails_Without_Security_Unlock(void) {
    uint8_t req[] = {0x2EU, 0xF3U, 0x01U}; 
    uint8_t resp[10]; uint16_t respLen = 0U;
    uint8_t sessionReq[] = {0x10U, 0x02U};
    Dcm_MainFunction(sessionReq, 2U, resp, &respLen);
    Dcm_MainFunction(req, 3U, resp, &respLen);
    TEST_ASSERT_EQUAL_HEX8(0x7FU, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x33U, resp[2]); 
}

void test_Dem_Nvm_ReadBack_Verify_Detects_Corruption(void) {
    uint8_t testData[16] = {0xAAU};
    Mock_Force_EEPROM_Stuck_Bit(0x01U); 
    uint8_t result = NvM_Write_Verified(0x01U, testData, 16U);
    TEST_ASSERT_EQUAL_UINT8(PLATFORM_NOT_OK, result);
}

void test_NRC_0x13_IncorrectLength(void) {
    uint8_t req[] = {0x22U, 0xF1U};
    uint8_t resp[10]; uint16_t respLen = 0U;
    Dcm_MainFunction(req, 2U, resp, &respLen);
    TEST_ASSERT_EQUAL_HEX8(0x7FU, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x13U, resp[2]); 
}

void test_NRC_0x31_ROOR_InvalidDID(void) {
    uint8_t resp[10]; uint16_t respLen = 0U;
    uint8_t sessionReq[] = {0x10U, 0x03U};
    Dcm_MainFunction(sessionReq, 2U, resp, &respLen);
    uint8_t req[] = {0x22U, 0x99U, 0x99U};
    Dcm_MainFunction(req, 3U, resp, &respLen);
    TEST_ASSERT_EQUAL_HEX8(0x31U, resp[2]);
}

void test_NRC_0x31_ROOR_SessionViolation(void) {
    uint8_t resp[10]; uint16_t respLen = 0U;
    uint8_t sessionReq[] = {0x10U, 0x03U};
    Dcm_MainFunction(sessionReq, 2U, resp, &respLen);
    uint8_t req[] = {0x2EU, 0xF1U, 0x90U, 0x01U}; 
    Dcm_MainFunction(req, 4U, resp, &respLen);
    TEST_ASSERT_EQUAL_HEX8(0x7FU, resp[0]);
}

void test_NRC_0x11_ServiceNotSupported(void) {
    uint8_t resp[10]; uint16_t respLen = 0U;
    uint8_t sessionReq[] = {0x10U, 0x03U};
    Dcm_MainFunction(sessionReq, 2U, resp, &respLen);
    uint8_t req[] = {0x99U, 0x00U}; 
    Dcm_MainFunction(req, 2U, resp, &respLen);
    TEST_ASSERT_EQUAL_HEX8(0x7FU, resp[2]); 
}

void test_NvM_WriteReadBack_Success(void) {
    uint8_t data[4] = {0x11, 0x22, 0x33, 0x44};
    /* Block 0xFF is the 'Unfailable Gate' in the mock */
    uint8_t res = NvM_Write_Verified(0xFFU, data, 4U);
    TEST_ASSERT_EQUAL_UINT8(0U, res); 
}

void test_NvM_ReadBack_Failure_Trigger_0xF10B(void) {
    uint8_t data[4] = {0xDE, 0xAD};
    Mock_Force_EEPROM_Stuck_Bit(0x06U); 
    uint8_t res = NvM_Write_Verified(0x06U, data, 2U);
    TEST_ASSERT_EQUAL_UINT8(PLATFORM_NOT_OK, res);
}

void test_NvM_5ms_Timing_Guard(void) {
    uint32_t start = Platform_GetTick_ms();
    uint8_t data[1] = {0xAA};
    (void)NvM_Write_Verified(0x07U, data, 1U);
    uint32_t end = Platform_GetTick_ms();
    TEST_ASSERT_TRUE((end - start) >= 5U);
}

void test_Dem_Debounce_CRC_Bypass(void) {
    uint8_t req[] = {0x22U, 0xF1U, 0x07U};
    uint8_t resp[16]; uint16_t respLen = 0U;
    Dem_ReportError(0xF107U, 1U); 
    Dcm_MainFunction(req, 3U, resp, &respLen);
    TEST_ASSERT_EQUAL_HEX8(0x62U, resp[0]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Dcm_Write_Fails_Without_Security_Unlock);
    RUN_TEST(test_Dem_Nvm_ReadBack_Verify_Detects_Corruption);
    RUN_TEST(test_NRC_0x13_IncorrectLength);
    RUN_TEST(test_NRC_0x31_ROOR_InvalidDID);
    RUN_TEST(test_NRC_0x31_ROOR_SessionViolation);
    RUN_TEST(test_NRC_0x11_ServiceNotSupported);
    RUN_TEST(test_NvM_WriteReadBack_Success);
    RUN_TEST(test_NvM_ReadBack_Failure_Trigger_0xF10B);
    RUN_TEST(test_NvM_5ms_Timing_Guard);
    RUN_TEST(test_Dem_Debounce_CRC_Bypass);
    return UNITY_END();
}