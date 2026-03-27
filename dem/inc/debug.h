#ifndef DEBUG_H_
#define DEBUG_H_
/* Stub for PC simulation */
#include <stdio.h>
#define DEBUG_LOG(fmt, ...) printf("[DEM] " fmt "\n", ##__VA_ARGS__)
#endif /* DEBUG_H_ */
