#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "Dcm.h"
#include "Dem.h"

// External function from your NvM logic to clear the "SQL" name
extern void NvM_Table_Init(void); 

int main(void) {
    /* 1. Initialize the Stack */
    NvM_Table_Init();
    Dem_Init();
    Dcm_Init();

    /* 2. Simulate a Fault so there is data in the EEPROM */
    printf("\n[System] Simulating Comm Failure (0xF100) for Testing...\n");
    Dem_ReportEvent(0xF100);

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
        // This takes 3 hex bytes from your keyboard
        if (scanf("%x %x %x", &b1, &b2, &b3) == 3) {
            request[0] = (uint8_t)b1;
            request[1] = (uint8_t)b2;
            request[2] = (uint8_t)b3;

            /* 3. Pass the data to the DCM */
            Dcm_MainFunction(request, 3, response, &respLen);

            /* 4. Display the Raw Response from the DCM */
            printf("DCM Response    > ");
            for(int i = 0; i < respLen; i++) {
                printf("%02X ", response[i]);
            }
            printf("\n");

            // Quick Human-Readable Decode for you
            if(response[0] == 0x62) {
                printf("[Status] Positive Response: Data Retrieved Successfully.\n");
            } else if(response[0] == 0x7F) {
                printf("[Status] Negative Response: Error Code 0x%02X\n", response[2]);
            }
        } else {
            // Clear buffer if user types something weird
            while(getchar() != '\n'); 
            printf("Invalid Hex Format. Use: 22 F1 00\n");
        }
    }

    return 0;
}
