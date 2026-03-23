#include "OsAl.h"
#include "TpAl.h"
#include "MemAl.h"
#include "Hal.h"
#include <stdio.h>
#include <stdint.h>

int main(void)
{
    printf("=================================\n");
    printf(" AUTOSAR Diagnostic Stack v1.0\n");
    printf(" Profile : CLASSIC\n");
    printf(" Target  : SIM\n");
    printf("=================================\n");

    OsAl_Init();
    printf("[Main] OsAl tick: %u ms\n", (unsigned)OsAl_GetTickMs());

    Hal_Init();

    MemAl_Register(&MemAl_File_Driver);
    TpAl_Register(&TpAl_UDP_Driver);

    printf("[Main] Transport : %s\n", TpAl_GetName());
    printf("[Main] Memory    : %s\n", MemAl_GetName());
    printf("[Main] Stack ready\n");
    printf("[Main] Send UDS bytes to UDP port 13400\n");
    printf("=================================\n");

    uint32_t lastTick = OsAl_GetTickMs();

    while (1)
    {
        uint32_t now = OsAl_GetTickMs();
        TpAl_MainFunction();

        uint8_t  rxBuf[256];
        uint16_t rxLen = 0U;

        if (TpAl_Receive(rxBuf, &rxLen) == E_OK)
        {
            printf("[Main] RX %u bytes: ", rxLen);
            for (uint16_t i = 0U; i < rxLen; i++) {
                printf("%02X ", rxBuf[i]);
            }
            printf("\n");

            uint8_t txBuf[256];
            for (uint16_t i = 0U; i < rxLen; i++) {
                txBuf[i] = rxBuf[i];
            }
            txBuf[0] = (uint8_t)(rxBuf[0] + 0x40U);

            if (TpAl_Transmit(txBuf, rxLen) == E_OK) {
                printf("[Main] TX %u bytes: ", rxLen);
                for (uint16_t i = 0U; i < rxLen; i++) {
                    printf("%02X ", txBuf[i]);
                }
                printf("\n");
            }
        }

        if ((now - lastTick) >= 10U) { lastTick = now; }
        OsAl_Delay(10U);
    }

    return 0;
}
