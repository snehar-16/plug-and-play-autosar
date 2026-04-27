/* USER CODE BEGIN Header */
/**
 * @file           : main.c
 * @brief          : Main program body (STM32 FreeRTOS Target)
 * @project        : SM-OCIP UDS DIAGNOSTIC TERMINAL (v2.0)
 * @compliance     : MISRA C:2012, SIL-4 
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h" /* STM32 FreeRTOS Wrapper */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* USER CODE BEGIN Includes */
#include "../dcm/Dcm_Cfg.h"
#include "../platform/platform_api.h"
#include "../dem/dem_event_logger.h"
#include "../dem/dem_core.h" 
#include "../dcm/dcm.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
/* USER CODE BEGIN PV */

/* FreeRTOS Handles */
TaskHandle_t DemTaskHandle = NULL;
TaskHandle_t DcmTaskHandle = NULL;
QueueHandle_t UdsRxQueue = NULL;

/* Structure to pass incoming UDS payloads from CAN/Ethernet to the DCM Task */
typedef struct {
    uint8_t data[256];
    uint16_t length;
} UdsMessage_t;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);

/* USER CODE BEGIN PFP */
void Start_Dem_Task(void *argument);
void Start_Dcm_Task(void *argument);
void System_Init_POST(void);
/* USER CODE END PFP */

/**
  * @brief  The application entry point.
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  
  /* 1. Run our Power-On Self-Test & Diagnostic Initialization */
  System_Init_POST();

  /* 2. Create the Queue to receive UDS packets from the hardware interface (e.g., CAN Rx Interrupt) */
  UdsRxQueue = xQueueCreate(5U, sizeof(UdsMessage_t));

  /* 3. Create SIL-4 Deterministic FreeRTOS Tasks */
  
  /* DEM Task: Highest Priority, strictly runs every 10ms */
  xTaskCreate(Start_Dem_Task, "DemTask", 256U, NULL, osPriorityRealtime, &DemTaskHandle);
  
  /* DCM Task: Normal Priority, waits for network requests */
  xTaskCreate(Start_Dcm_Task, "DcmTask", 512U, NULL, osPriorityNormal, &DcmTaskHandle);

  /* USER CODE END 2 */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  for(;;)
  {
      /* MISRA compliant infinite loop trap */
  }
}

/* USER CODE BEGIN 4 */

/**
 * @brief Automated Power-On Self-Test (POST) for Bare-Metal Target
 */
void System_Init_POST(void) {
    uint8_t eepromBuffer[4096] = {0U};

    Platform_Init();   
    Platform_RTC_Init();
    Dem_Init();        
    
    /* Executing IEC-61508 3-Step Boot Fallback */
    if (Dem_Nvm_Load(eepromBuffer, (uint16_t)sizeof(eepromBuffer)) != PLATFORM_OK) {
        /* NvM Load FAILED: Starting Clean. Logging Fault 0xF10B */
        Dem_SetEventStatus(0xF10BU, 1U); 
    }
    
    Dcm_Init();        
    DEM_EventLogger_Init();
}

/**
 * @brief DEM Cyclic Executive Task (10ms)
 * Evaluates debounce timers, manages fault aging, and triggers EEPROM writes.
 */
void Start_Dem_Task(void *argument) {
    (void)argument; /* MISRA: Acknowledge unused parameter */
    
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(10U);

    /* Initialize the xLastWakeTime variable with the current time. */
    xLastWakeTime = xTaskGetTickCount();

    for(;;) {
        /* Wait for the exact 10ms cycle */
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        /* Tick the UDS S3 Timer (10ms elapsed) */
        Dcm_ManageSessionTimer(10U);
        
        /* Run Core DEM Logic */
        Dem_MainFunction();
        
        /* Feed the SIL-4 Hardware Watchdog */
        Platform_WdgTrigger();
    }
}

/**
 * @brief DCM Event-Driven Task
 * Sleeps efficiently until a UDS packet arrives via the queue.
 */
void Start_Dcm_Task(void *argument) {
    (void)argument;
    
    UdsMessage_t rxMsg;
    uint8_t txBuffer[256] = {0U};
    uint16_t txLen = 0U;

    for(;;) {
        /* Block indefinitely until a UDS message is pushed to the queue by a CAN/Ethernet interrupt */
        if (xQueueReceive(UdsRxQueue, &rxMsg, portMAX_DELAY) == pdPASS) {
            
            txLen = 0U;
            
            /* Process the incoming request */
            Dcm_MainFunction(rxMsg.data, rxMsg.length, txBuffer, &txLen);
            
            /* If a response was generated, send it back to the network layer */
            if (txLen > 0U) {
                /* e.g., CAN_Transmit(txBuffer, txLen); */
            }
        }
    }
}

/* USER CODE END 4 */