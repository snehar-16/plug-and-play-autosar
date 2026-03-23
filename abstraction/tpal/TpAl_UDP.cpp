#include "TpAl.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define UDP_RX_PORT     13400
#define UDP_TX_PORT     13401
#define UDP_BUF_SIZE    4096

static int                Udp_SockFd  = -1;
static struct sockaddr_in Udp_TxAddr;
static uint8_t            Udp_RxBuf[UDP_BUF_SIZE];
static uint16_t           Udp_RxLen   = 0U;
static uint8_t            Udp_RxReady = 0U;

static void UdpStub_Init(void)
{
    Udp_SockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (Udp_SockFd < 0) { return; }
    fcntl(Udp_SockFd, F_SETFL, O_NONBLOCK);
    struct sockaddr_in rx;
    memset(&rx, 0, sizeof(rx));
    rx.sin_family      = AF_INET;
    rx.sin_addr.s_addr = INADDR_ANY;
    rx.sin_port        = htons(UDP_RX_PORT);
    bind(Udp_SockFd, (struct sockaddr*)&rx, sizeof(rx));
    memset(&Udp_TxAddr, 0, sizeof(Udp_TxAddr));
    Udp_TxAddr.sin_family = AF_INET;
    Udp_TxAddr.sin_port   = htons(UDP_TX_PORT);
    inet_pton(AF_INET, "127.0.0.1", &Udp_TxAddr.sin_addr);
    printf("[TpAl_UDP] Listening on port %d\n", UDP_RX_PORT);
}

static Std_ReturnType UdpStub_Transmit(const uint8_t* data, uint16_t len)
{
    if (Udp_SockFd < 0) { return E_NOT_OK; }
    ssize_t sent = sendto(Udp_SockFd, data, len, 0,
                          (struct sockaddr*)&Udp_TxAddr,
                          sizeof(Udp_TxAddr));
    return (sent == (ssize_t)len) ? E_OK : E_NOT_OK;
}

static Std_ReturnType UdpStub_Receive(uint8_t* buf, uint16_t* len)
{
    if (Udp_RxReady == 0U) { return E_NOT_OK; }
    memcpy(buf, Udp_RxBuf, Udp_RxLen);
    *len        = Udp_RxLen;
    Udp_RxReady = 0U;
    Udp_RxLen   = 0U;
    return E_OK;
}

static void UdpStub_MainFunction(void)
{
    if (Udp_SockFd < 0 || Udp_RxReady != 0U) { return; }
    struct sockaddr_in src;
    socklen_t srcLen = sizeof(src);
    ssize_t n = recvfrom(Udp_SockFd,
                         Udp_RxBuf, UDP_BUF_SIZE, 0,
                         (struct sockaddr*)&src, &srcLen);
    if (n > 0) {
        Udp_RxLen           = (uint16_t)n;
        Udp_RxReady         = 1U;
        Udp_TxAddr.sin_addr = src.sin_addr;
        Udp_TxAddr.sin_port = src.sin_port;
    }
}

#ifdef __cplusplus
extern "C" {
#endif

const TpAl_DriverType TpAl_UDP_Driver = {
    UdpStub_Init,
    UdpStub_Transmit,
    UdpStub_Receive,
    UdpStub_MainFunction,
    "UDP"
};

#ifdef __cplusplus
}
#endif
