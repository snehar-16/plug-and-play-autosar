#include <stdint.h>
#define I2C_MEMADD_SIZE_16BIT 2
typedef int I2C_HandleTypeDef; extern I2C_HandleTypeDef hi2c1;
void HAL_I2C_Mem_Write(I2C_HandleTypeDef* a, int b, int c, int d, void* e, int f, int g);
void HAL_I2C_Mem_Read(I2C_HandleTypeDef* a, int b, int c, int d, void* e, int f, int g);
