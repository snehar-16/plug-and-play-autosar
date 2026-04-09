#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h> /* Added for strtol (Dynamic Parsing) */

/* --- Explicit Relative Paths --- */
#include "../dcm/Dcm_Cfg.h"
#include "../platform/platform_api.h"
#include "../dem/dem_event_logger.h"

/* External Function Declarations */
extern void Platform_Init(void); 
extern void Platform_RTC_Init(void);
extern void Platform_WdgTrigger(void);
extern void Dem_Init(void);
extern void Dcm_Init(void);
extern void Dem_SetEventStatus(uint16_t did, uint8_t isFailed);
extern void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen);
extern uint8_t Dem_Nvm_Load(uint8_t *outData, uint16_t length);

static uint8_t eepromBuffer[4096];

/**
 * @brief Automated Power-On Self-Test (POST)
 */
void System_Init_POST(void) {
    printf("[POST] 1. Initializing Platform Hardware Abstraction...\n");
    Platform_Init();   
    printf("[POST] 2. Initializing Real-Time Clock (RTC)...\n");
    Platform_RTC_Init();
    printf("[POST] 3. Initializing DEM RAM Structures...\n");
    Dem_Init();        
    printf("[POST] 4. Executing IEC-61508 3-Step Boot Fallback...\n");
    if (Dem_Nvm_Load(eepromBuffer, sizeof(eepromBuffer)) == PLATFORM_OK) {
        printf("[POST] NvM Load SUCCESS: EEPROM data safely restored to RAM.\n");
    } else {
        printf("[POST] NvM Load FAILED: Starting Clean. Logging Fault 0xF10B...\n");
        Dem_SetEventStatus(0xF10B, 1); 
    }
    printf("[POST] 5. Initializing DCM (UDS Stack Ready)...\n");
    Dcm_Init();        
    printf("[POST] 6. Initializing Circular Black Box Logger...\n");
    DEM_EventLogger_Init();
}

int main(void) {
    System_Init_POST();

    /* Simulate Initial Fault */
    printf("\n[System] Simulating Comm Failure (0xF100) per SMOP_SWRS_13...\n");
    Dem_SetEventStatus(0xF100, 1); 

    uint8_t request[256]; /* Expanded buffer for variable length payloads */
    uint8_t response[256]; 
    uint16_t respLen = 0;
    char inputLine[512];

    /* Integrated Terminal Header */
    printf("\n=======================================================\n");
    printf("        SM-OCIP UDS DIAGNOSTIC TERMINAL (v2.0)         \n");
    printf("        Ref: SMOP_SWRS v1.1 | ISO 14229-1 SIL-4        \n");
    printf("=======================================================\n");
    printf(" Sessions: 10 01 (Default) | 10 02 (Prog) | 10 03 (Ext)\n");
    printf(" Examples:\n");
    printf("  - Read Fault : 22 F1 00\n");
    printf("  - Read Audit : 22 F2 00 (Requires Session 03)\n");
    printf("  - IO Control : 2F 01 03 (Requires Session 03 + Unlocked)\n");
    printf("  - Tester Pres: 3E 00\n");
    printf("=======================================================\n");

    while(1) {
        Platform_WdgTrigger();

        printf("\nScanner Request > ");
        
        if (fgets(inputLine, sizeof(inputLine), stdin) != NULL) {
            if (inputLine[0] == '\n') continue; /* Ignore empty "Enter" presses */
            
            uint16_t reqLen = 0;
            char *token = strtok(inputLine, " \n");
            
            /* DYNAMIC PARSER: Reads however many bytes you type */
            while (token != NULL && reqLen < 256) {
                request[reqLen++] = (uint8_t)strtol(token, NULL, 16);
                token = strtok(NULL, " \n");
            }

            if (reqLen > 0) {
                respLen = 0;
                Dcm_MainFunction(request, reqLen, response, &respLen);

                if (respLen > 0) {
                    printf("DCM Response    > ");
                    for(int i = 0; i < respLen; i++) printf("%02X ", response[i]);
                    printf("\n");

                    if(response[0] == (request[0] + 0x40)) {
                        printf("[Status] Positive Response: Request Successful.\n");
                    } else if(response[0] == 0x7F) {
                        switch(response[2]) {
                            case 0x11: printf("[Status] NRC 0x11: Service Not Supported.\n"); break;
                            case 0x12: printf("[Status] NRC 0x12: SubFunction Not Supported.\n"); break;
                            case 0x13: printf("[Status] NRC 0x13: Incorrect Message Length.\n"); break;
                            case 0x31: printf("[Status] NRC 0x31: Request Out Of Range (Locked/Invalid).\n"); break;
                            case 0x33: printf("[Status] NRC 0x33: Security Access Denied.\n"); break;
                            case 0x7F: printf("[Status] NRC 0x7F: Service Not Supported in Active Session.\n"); break;
                            default:   printf("[Status] Negative Response: Error Code 0x%02X\n", response[2]); break;
                        }
                    }
                }
            }
        }
    }
    return 0;
}