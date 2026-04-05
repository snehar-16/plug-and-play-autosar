#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "Dcm.h"
#include "Dem.h"
#include "Dem_Cfg.h"
#include "main.h"

/* External NvM and HAL providers */
extern I2C_HandleTypeDef hi2c1;
extern void NvM_Table_Init(void); 
extern void Run_Internal_SelfDiag(void);

/**
 * @brief Automated Power-On Self-Test (POST)
 * Verifies diagnostic stack health before starting the scheduler.
 */
void System_Init_POST(void) {
    /* 1. Initialize Stack Layers [cite: 78, 82, 84] */
    Platform_Init();   /* Setup Mutexes and Timers [cite: 72] */
    Dem_Init();        /* Zero event tables [cite: 86] */
    Dcm_Init();        /* Start TCP server logic [cite: 88] */

    /* 2. Automated POST: Check EEPROM Hardware Health */
    uint8_t testByte = 0xAA;
    uint8_t readByte = 0;
    
    /* Test a reserved diagnostic page for "stuck-at" bits [cite: 68] */
    /* Using address 0x1FE00 to avoid primary log sectors [cite: 68] */
    HAL_I2C_Mem_Write(&hi2c1, EEPROM_DEV_ADDR, 0x1FE00, 2, &testByte, 1, 100);
    vTaskDelay(pdMS_TO_TICKS(5)); /* Mandatory EEPROM write cycle  */
    HAL_I2C_Mem_Read(&hi2c1, EEPROM_DEV_ADDR, 0x1FE00, 2, &readByte, 1, 100);

    if (testByte == readByte) {
        /* 3. Execute Routine 0x0100: Internal Logic Check [cite: 44] */
        Run_Internal_SelfDiag();
    } else {
        /* Stack is unhealthy: Trigger Critical Fail DID 0xF10B [cite: 37] */
        Dem_SetEventStatus(DID_EEPROM_WRITE_FAILURE, 1);
    }
}

int main(void) {
    /* 1. Perform High-Integrity System Boot */
    NvM_Table_Init();
    System_Init_POST();

    /* 2. Simulate a Fault for Testing Initial Connectivity */
    printf("\n[System] Simulating Comm Failure (0xF100) for Testing...\n");
    Dem_Report_With_Count(0xF100); /* Use incrementing counter logic */

    uint8_t request[16];
    uint8_t response[64];
    uint16_t respLen;

    printf("\n============================================\n");
    printf("   SM-OCIP UDS DIAGNOSTIC TERMINAL (DCM)    \n");
    printf("============================================\n");
    printf("Ready for Scanner Input (e.g., 22 F1 00)\n");

    while(1) {
        printf("\nScanner Request > ");
        
        unsigned int b1, b2, b3;
        /* Accept 3 hex bytes representing a UDS SID and DID */
        if (scanf("%x %x %x", &b1, &b2, &b3) == 3) {
            request[0] = (uint8_t)b1;
            request[1] = (uint8_t)b2;
            request[2] = (uint8_t)b3;

            /* 3. Pass request to the DCM Main Function [cite: 14] */
            /* This now supports Multi-DID adaptive responses */
            Dcm_MainFunction(request, 3, response, &respLen);

            /* 4. Display Raw DCM Response */
            printf("DCM Response    > ");
            for(int i = 0; i < respLen; i++) {
                printf("%02X ", response[i]);
            }
            printf("\n");

            /* 5. Human-Readable Status Decode */
            if(response[0] == 0x62) {
                printf("[Status] Positive Response: Data Retrieved Successfully.\n");
            } else if(response[0] == 0x7F) {
                /* Return NRC (Negative Response Code) [cite: 14] */
                printf("[Status] Negative Response: Error Code 0x%02X\n", response[2]);
            }
        } else {
            /* Clear buffer on invalid input */
            while(getchar() != '\n'); 
            printf("Invalid Hex Format. Use: 22 F1 00\n");
        }
    }

    return 0;
}