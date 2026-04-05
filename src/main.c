#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "Dcm.h"
#include "Dem.h"
#include "Dem_Cfg.h"
#include "main.h"

/* External NvM and HAL providers */
extern int hi2c1; // Mocked I2C handle
extern void NvM_Table_Init(void); 
extern void Run_Internal_SelfDiag(void);
extern void Platform_Init(void);

/**
 * @brief Automated Power-On Self-Test (POST)
 * Verifies diagnostic stack health and EEPROM connectivity[cite: 6, 19].
 */
void System_Init_POST(void) {
    /* 1. Initialize Stack Layers  */
    Platform_Init();   
    Dem_Init();        
    Dcm_Init();        /* Initialize Session to DEFAULT (0x01) */

    /* 2. Automated POST: Check 1Mbit EEPROM Hardware Health  */
    uint8_t testByte = 0xAA;
    uint8_t readByte = 0;
    
    /* SMOP_SWRS_30: Verify EEPROM write/read cycle  */
    HAL_I2C_Mem_Write(&hi2c1, EEPROM_DEV_ADDR, 0x1FE00, 2, &testByte, 1, 100);
    vTaskDelay(pdMS_TO_TICKS(5)); 
    HAL_I2C_Mem_Read(&hi2c1, EEPROM_DEV_ADDR, 0x1FE00, 2, &readByte, 1, 100);

    if (testByte == readByte) {
        printf("[POST] EEPROM Health OK. Starting Internal Self-Diag...\n");
        Run_Internal_SelfDiag();
    } else {
        /* Trigger Session 1 Fault: EEPROM Write Failure (0xF10B)  */
        printf("[POST] EEPROM ERROR! Logging Fault 0xF10B...\n");
        Dem_SetEventStatus(0xF10B, 1);
    }
}

int main(void) {
    /* 1. Perform High-Integrity System Boot  */
    NvM_Table_Init();
    System_Init_POST();

    /* 2. Simulate Initial Fault: Comm Link Failure (0xF100)  */
    printf("\n[System] Simulating Comm Failure (0xF100) per SMOP_SWRS_13...\n");
    Dem_SetEventStatus(0xF100, 1); 

    uint8_t request[16];
    uint8_t response[256]; /* Expanded buffer for multi-DID responses */
    uint16_t respLen;

    printf("\n============================================\n");
    printf("    SM-OCIP UDS DIAGNOSTIC TERMINAL (DCM)    \n");
    printf("    Ref: SMOP_SWRS v1.1 | SIL-4 Compliant    \n");
    printf("============================================\n");
    printf("Ready for Scanner Input (e.g., 22 F1 00)\n");

    while(1) {
        printf("\nScanner Request > ");
        
        unsigned int b1, b2, b3;
        /* Scanner captures 3 bytes (Service ID + 2-byte DID) [cite: 14, 17] */
        if (scanf("%x %x %x", &b1, &b2, &b3) == 3) {
            request[0] = (uint8_t)b1;
            request[1] = (uint8_t)b2;
            request[2] = (uint8_t)b3;

            /* 3. Pass request to the DCM Layer  */
            /* Enforces Session 2 Lockout if not in Extended Mode */
            Dcm_MainFunction(request, 3, response, &respLen);

            /* 4. Display Raw DCM Response in HEX */
            printf("DCM Response    > ");
            for(int i = 0; i < respLen; i++) {
                printf("%02X ", response[i]);
            }
            printf("\n");

            /* 5. UDS Response Status Decoder  */
            if(response[0] == (request[0] + 0x40)) {
                printf("[Status] Positive Response: Request Successful.\n");
            } else if(response[0] == 0x7F) {
                /* Decode Negative Response Codes (NRC)  */
                if(response[2] == 0x31) {
                    printf("[Status] NRC 0x31: Request Out Of Range (DID Invalid or Session Locked).\n");
                } else if(response[2] == 0x11) {
                    printf("[Status] NRC 0x11: Service Not Supported.\n");
                } else {
                    printf("[Status] Negative Response: Error Code 0x%02X\n", response[2]);
                }
            }
        } else {
            /* Clear stdin buffer on formatting error */
            int c; while ((c = getchar()) != '\n' && c != EOF); 
            printf("Invalid Format. Please enter 3 hex bytes (e.g., 22 F1 00).\n");
        }
    }

    return 0;
}