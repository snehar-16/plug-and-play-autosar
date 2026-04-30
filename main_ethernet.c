#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "dcm/Dcm_Cfg.h"

extern void Dcm_Init(void);
extern void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen);

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    uint8_t rxBuffer[1024];
    uint8_t txBuffer[1024];

    Dcm_Init();
    printf("SM-OCIP Railway ECU Started. Listening on Port 13400...\n");

    // Create TCP Socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(13400);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    while(1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        printf("[Ethernet] Tester Connected.\n");

        while(1) {
            ssize_t valread = read(new_socket, rxBuffer, 1024);
            if (valread <= 0) break;

            uint16_t txLen = 0;
            // Pass the raw Ethernet frame to your DCM
            Dcm_MainFunction(rxBuffer, (uint16_t)valread, txBuffer, &txLen);

            if (txLen > 0) {
                send(new_socket, txBuffer, txLen, 0);
            }
        }
        close(new_socket);
        printf("[Ethernet] Tester Disconnected.\n");
    }
    return 0;
}
