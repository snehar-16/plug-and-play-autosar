#include <stdint.h>

/* MISRA: Unsigned suffix for integer literals */
#define I2C_MEMADD_SIZE_16BIT 2U

/* MISRA: Replace basic 'int' with sized type */
typedef uint32_t I2C_HandleTypeDef; 

/* MISRA: Externs should ideally live in a shared header file */
extern I2C_HandleTypeDef hi2c1;

/* MISRA: Sized types, explicit uint8_t data pointers, and meaningful parameter names */
void HAL_I2C_Mem_Write(I2C_HandleTypeDef* hi2c, uint16_t devAddr, uint16_t memAddr, uint16_t memAddSize, uint8_t* pData, uint16_t size, uint32_t timeout);

void HAL_I2C_Mem_Read(I2C_HandleTypeDef* hi2c, uint16_t devAddr, uint16_t memAddr, uint16_t memAddSize, uint8_t* pData, uint16_t size, uint32_t timeout);