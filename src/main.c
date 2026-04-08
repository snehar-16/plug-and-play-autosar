#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* --- Use Explicit Relative Paths to avoid linking old BSW files --- */
#include "../dcm/Dcm_Cfg.h"
#include "../platform/platform_api.h"
#include "../dem/dem_event_logger.h"

/* External Function Declarations */
extern void Platform_Init(void); 
extern void Platform_RTC_Init(void);    /* Required for SIL-4 Timestamps  */
extern void Platform_WdgTrigger(void);  /* Required for SIL-4 Safety  */
extern void Dem_Init(void);
extern void Dcm_Init(void);
extern void Dem_SetEventStatus(uint16_t did, uint8_t isFailed);
extern void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen);
extern uint8_t Dem_Nvm_Load(uint8_t *outData, uint16_t length);

/* Static Buffer for NvM Load to prevent stack smashing  */
static uint8_t eepromBuffer[4096];

/**
 * @brief Automated Power-On Self-Test (POST) & Boot Sequence
 * Follows the mandatory 5-step sequence defined in the Integration Plan.
 */
void System_Init_POST(void) {
    /* STEP 1: Platform Hardware Abstraction (I2C @ 0xA0) [cite: 17, 21] */
    printf("[POST] 1. Initializing Platform Hardware Abstraction...\n");
    Platform_Init();   
    
    /* STEP 2: Real-Time Clock (RTC) - Must be initialized before DEM  */
    printf("[POST] 2. Initializing Real-Time Clock (RTC)...\n");
    Platform_RTC_Init();

    /* STEP 3: DEM Engine RAM Structures  */
    printf("[POST] 3. Initializing DEM RAM Structures...\n");
    Dem_Init();        

    /* STEP 4: IEC-61508 3-Step Boot Fallback Chain  */
    printf("[POST] 4. Executing IEC-61508 3-Step Boot Fallback (Primary -> Mirror -> Clean)...\n");
    if (Dem_Nvm_Load(eepromBuffer, sizeof(eepromBuffer)) == PLATFORM_OK) {
        printf("[POST] NvM Load SUCCESS: EEPROM data safely restored to RAM.\n");
    } else {
        printf("[POST] NvM Load FAILED: Starting Clean. Logging Fault 0xF10B...\n");
        /* Trigger immediate fault for NvM failure per Doc B  */
        Dem_SetEventStatus(0xF10B, 1); 
    }

    /* STEP 5: DCM Initialization (Default Session 0x01)  */
    printf("[POST] 5. Initializing DCM (UDS Stack Ready)...\n");
    Dcm_Init();        

    /* STEP 6: Black Box Event Logger [cite: 18, 21] */
    printf("[POST] 6. Initializing Circular Black Box Logger (Pages 365-511)...\n");
    DEM_EventLogger_Init();
}

/**
 * @brief Main execution loop with SIL-4 Watchdog integration.
 */
int main(void) {
    /* Perform High-Integrity System Boot */
    System_Init_POST();

    /* Simulate Initial Fault: Comm Link Failure (0xF100) per SMOP_SWRS_13 [cite: 26] */
    printf("\n[System] Simulating Comm Failure (0xF100) per SMOP_SWRS_13...\n");
    Dem_SetEventStatus(0xF100, 1); 

    uint8_t request[3];
    uint8_t response[256]; 
    uint16_t respLen = 0;

    printf("\n============================================\n");
    printf("     SM-OCIP UDS DIAGNOSTIC TERMINAL (DCM)    \n");
    printf("     Ref: SMOP_SWRS v1.1 | SIL-4 Compliant    \n");
    printf("============================================\n");
    printf("Scanner ready (Use 3 bytes: Service ID + 2 bytes data)\n");

    while(1) {
        /* SIL-4 Watchdog Trigger: Must be fed from the diagnostic task  */
        Platform_WdgTrigger();

        printf("\nScanner Request > ");
        
        unsigned int b1, b2, b3;
        if (scanf("%x %x %x", &b1, &b2, &b3) == 3) {
            request[0] = (uint8_t)b1;
            request[1] = (uint8_t)b2;
            request[2] = (uint8_t)b3;
            respLen = 0;

            /* Pass 3-byte request to the DCM Gatekeeper [cite: 13] */
            Dcm_MainFunction(request, 3, response, &respLen);

            /* Display DCM Response with NRC Decoding  */
            if (respLen > 0) {
                printf("DCM Response    > ");
                for(int i = 0; i < respLen; i++) printf("%02X ", response[i]);
                printf("\n");

                if(response[0] == (request[0] + 0x40)) {
                    printf("[Status] Positive Response: Request Successful.\n");
                } else if(response[0] == 0x7F) {
                    /* NRC Decoding per Doc B requirements  */
                    switch(response[2]) {
                        case 0x11: printf("[Status] NRC 0x11: Service Not Supported.\n"); break;
                        case 0x13: printf("[Status] NRC 0x13: Incorrect Message Length.\n"); break;
                        case 0x31: printf("[Status] NRC 0x31: Request Out Of Range.\n"); break;
                        case 0x33: printf("[Status] NRC 0x33: Security Access Denied.\n"); break;
                        case 0x7F: printf("[Status] NRC 0x7F: Service Not Supported in Active Session.\n"); break;
                        default:   printf("[Status] Negative Response: Error Code 0x%02X\n", response[2]); break;
                    }
                }
            }
        } else {
            int c; while ((c = getchar()) != '\n' && c != EOF); 
            printf("Invalid Format. Please enter 3 hex bytes (e.g., 22 F1 00).\n");
        }
    }
    return 0;
}