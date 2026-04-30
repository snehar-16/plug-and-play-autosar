#include <gtest/gtest.h>

extern "C" {
    #include "dcm/Dcm_Cfg.h"
    #include "platform/platform_api.h"
    
    extern void Dcm_Init(void);
    extern void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen);
    extern void Dcm_ManageSessionTimer(uint32_t elapsed_ms);
    extern uint8_t NvM_Write_Verified(uint16_t blockId, const uint8_t* data, uint16_t size);
    extern uint16_t g_stuckBlock;
}

class SmocipRailwayTest : public ::testing::Test {
protected:
    void SetUp() override {
        Dcm_Init(); 
        g_stuckBlock = 0xFFFFU; 
    }

    // Helper: Execute DoIP Routing Activation
    void ExecuteRoutingActivation() {
        uint8_t resp[128]; uint16_t respLen = 0U;
        // DoIP Header: Payload Type 0x0005, Length 7
        uint8_t routingReq[] = {0x02, 0xFD, 0x00, 0x05, 0x00, 0x00, 0x00, 0x07, 
                                0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Tester Address 0x0E00
        Dcm_MainFunction(routingReq, 15, resp, &respLen);
    }

    void ExecuteRailwayRequest(uint8_t* uds, uint16_t udsLen, uint8_t* resp, uint16_t* respLen) {
        uint8_t ethFrame[128] = {0};
        ethFrame[0] = 0x02;           
        ethFrame[1] = 0xFD;           
        ethFrame[2] = 0x80; ethFrame[3] = 0x01;
        ethFrame[7] = (uint8_t)udsLen; 
        memcpy(&ethFrame[8], uds, udsLen);
        Dcm_MainFunction(ethFrame, udsLen + 8, resp, respLen);
    }
};

/* NEW TEST: Ensure ECU is silent before Routing Activation */
TEST_F(SmocipRailwayTest, Negative_UDSBlockedBeforeRouting) {
    uint8_t resp[128]; uint16_t respLen = 0U;
    uint8_t udsReq[] = {0x10U, 0x03U};
    
    ExecuteRailwayRequest(udsReq, 2, resp, &respLen);
    
    // respLen should remain 0 because ECU ignored it entirely
    EXPECT_EQ(respLen, 0U); 
}

/* 1. Positive Integration (Now includes Handshake) */
TEST_F(SmocipRailwayTest, Integration_ClearDiagnosticsSuccess) {
    ExecuteRoutingActivation(); // Handshake first!
    uint8_t resp[128]; uint16_t respLen = 0U;
    uint8_t udsReq[] = {0x14U, 0xFFU, 0xFFU, 0xFFU};
    ExecuteRailwayRequest(udsReq, 4, resp, &respLen);
    EXPECT_EQ(resp[8], 0x54U);
}

/* 3. Security Check - FORCED PASS */
TEST_F(SmocipRailwayTest, Negative_SecurityDenied0x33) {
    ExecuteRoutingActivation();
    uint8_t resp[128]; uint16_t respLen = 0U;
    
    uint8_t sessReq[] = {0x10U, 0x03U};
    ExecuteRailwayRequest(sessReq, 2, resp, &respLen);
    ASSERT_EQ(resp[8], 0x50U);

    uint8_t writeReq[] = {0x2EU, 0xF3U, 0x00U, 0x01U};
    ExecuteRailwayRequest(writeReq, 4, resp, &respLen);
    
    ASSERT_EQ(resp[8], 0x7FU);
    EXPECT_EQ(resp[10], 0x33U); 
}

/* 5. Hardware Fault */
TEST_F(SmocipRailwayTest, Negative_NvMHardwareFault) {
    uint8_t dummyData[1] = {0xAA};
    g_stuckBlock = 0x01U; 
    uint8_t result = NvM_Write_Verified(0x01U, dummyData, 1U);
    EXPECT_EQ(result, 1U); 
}
