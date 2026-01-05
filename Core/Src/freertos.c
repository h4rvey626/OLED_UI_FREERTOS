/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "input.h"
#include "MENU.h"
#include "OLED.h"
#include "esp_driver.h"
#include "app_events.h"
#include "wifi_task.h"
#include "time_task.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* 输入事件队列 - 在 USER CODE 区域定义，不会被 CubeMX 覆盖 */
osMessageQueueId_t InputEventQueueHandle;
static const osMessageQueueAttr_t InputEventQueue_attributes = {
  .name = "InputEventQueue"
};

/* App 事件标志：用于 WiFi 启动完成后再进入菜单 */
osEventFlagsId_t g_appEventFlags;
static const osEventFlagsAttr_t g_appEventFlags_attr = {
  .name = "AppEventFlags"
};
/* USER CODE END Variables */
/* Definitions for InputTask */
osThreadId_t InputTaskHandle;
const osThreadAttr_t InputTask_attributes = {
  .name = "InputTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for MenuTask */
osThreadId_t MenuTaskHandle;
const osThreadAttr_t MenuTask_attributes = {
  .name = "MenuTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for WIFITask */
osThreadId_t WIFITaskHandle;
const osThreadAttr_t WIFITask_attributes = {
  .name = "WIFITask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for TimeTask */
osThreadId_t TimeTaskHandle;
const osThreadAttr_t TimeTask_attributes = {
  .name = "TimeTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for TimeQueue */
osMessageQueueId_t TimeQueueHandle;
const osMessageQueueAttr_t TimeQueue_attributes = {
  .name = "TimeQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern void WIFITask_Entry(void *argument);
/* USER CODE END FunctionPrototypes */

void StartInputTask(void *argument);
void StartMenuTask(void *argument);
void StartWIFITask(void *argument);
void StartTimeTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of TimeQueue */
  TimeQueueHandle = osMessageQueueNew (16, 32, &TimeQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* 创建输入事件队列 - 使用 sizeof(InputEvent) 确保大小正确 */
  InputEventQueueHandle = osMessageQueueNew(16, sizeof(InputEvent), &InputEventQueue_attributes);
  /* 创建事件标志（必须在任务创建之前！）*/
  g_appEventFlags = osEventFlagsNew(&g_appEventFlags_attr);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of InputTask */
  InputTaskHandle = osThreadNew(StartInputTask, NULL, &InputTask_attributes);

  /* creation of MenuTask */
  MenuTaskHandle = osThreadNew(StartMenuTask, NULL, &MenuTask_attributes);

  /* creation of WIFITask */
  WIFITaskHandle = osThreadNew(StartWIFITask, NULL, &WIFITask_attributes);

  /* creation of TimeTask */
  TimeTaskHandle = osThreadNew(StartTimeTask, NULL, &TimeTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* 事件标志已在 RTOS_QUEUES 区域创建 */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartInputTask */
/**
  * @brief  Function implementing the InputTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartInputTask */
void StartInputTask(void *argument)
{
  /* USER CODE BEGIN StartInputTask */
  /* Infinite loop */
  for(;;)
  {
	InputEvent event = Input_GetEvent();
    if(event.type != INPUT_NONE)
    {
      // 发送事件到队列，Menu任务会接收
      osMessageQueuePut(InputEventQueueHandle, &event, 0, 0);
    }
    osDelay(10);
  }
  /* USER CODE END StartInputTask */
}

/* USER CODE BEGIN Header_StartMenuTask */
/**
* @brief Function implementing the MenuTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartMenuTask */
void StartMenuTask(void *argument)
{
  /* USER CODE BEGIN StartMenuTask */
	OLED_Init();
	OLED_Clear();
  UI_State ui_state = UI_MENU;
  /* 等待 WiFi 启动阶段结束（成功/失败都会置位），避免菜单抢占启动提示 */
  if (g_appEventFlags) {
    uint32_t flags = osEventFlagsWait(g_appEventFlags,
                                     APP_EVT_WIFI_DONE,
                                     osFlagsWaitAny | osFlagsNoClear,  // 不清除标志，让其他任务也能读到
                                     30000);
    (void)flags;
  }
  /* Infinite loop */
  for(;;)
  {
    if (ui_state == UI_CLOCK) {
      CLOCK_Draw();
      if (InputEventQueueHandle) {
        InputEvent event;
        uint8_t enter_pressed = 0;

        while (osMessageQueueGet(InputEventQueueHandle, &event, NULL, 0) == osOK) {
          if (event.type != INPUT_NONE) {
            MENU_UpdateActivity(); /* 有输入就算“活动”，顺便可唤醒屏幕 */
          }
          if (event.type == INPUT_ENTER) {
            enter_pressed = 1;
          }
        }

        if (enter_pressed) {
          ui_state = UI_MENU;
        }
      }
    } else if (ui_state == UI_MENU) {
      MENU_RunMainMenu();
      ui_state = UI_CLOCK;
    }
    osDelay(10);
  }
  /* USER CODE END StartMenuTask */
}

/* USER CODE BEGIN Header_StartWIFITask */
/**
* @brief Function implementing the WIFITask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartWIFITask */
void StartWIFITask(void *argument)
{
  /* USER CODE BEGIN StartWIFITask */
  WIFITask_Entry(argument);
  /* USER CODE END StartWIFITask */
}

/* USER CODE BEGIN Header_StartTimeTask */
/**
* @brief Function implementing the TimeTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTimeTask */
void StartTimeTask(void *argument)
{
  /* USER CODE BEGIN StartTimeTask */
  bool sntp_ok = false;

  /* 等待 WiFi 连接完成 */
  if (g_appEventFlags) {
    uint32_t flags = osEventFlagsWait(g_appEventFlags,
                                     APP_EVT_WIFI_OK,
                                     osFlagsWaitAny | osFlagsNoClear,  // 不清除标志
                                     60000);

    if ((flags & osFlagsError) || !(flags & APP_EVT_WIFI_OK)) {
      // WiFi 连接失败或超时，通知 SNTP 失败并退出
      goto sntp_exit;
    }
  }

  /* 配置 SNTP */
  if (SNTP_Config()) {
    sntp_ok = true;
    WIFI_MQTT_Publish("RADAR/TIME", "SNTP Config Success");
  }

sntp_exit:
  /* 通知其他任务：SNTP 阶段已结束 */
  if (g_appEventFlags) {
    uint32_t set = APP_EVT_SNTP_DONE | (sntp_ok ? APP_EVT_SNTP_OK : 0U);
    osEventFlagsSet(g_appEventFlags, set);
  }

  if (!sntp_ok) {
    WIFI_MQTT_Publish("RADAR/TIME", "SNTP Config Failed");
    osThreadExit();
  }

  char time_str[32] = {0};  // 初始化为空，避免发送垃圾数据

  /* Infinite loop - 每 10 秒上传一次时间 */
  for(;;)
  {
    if (SNTP_GetTimeString(time_str, sizeof(time_str))) {
      osMessageQueuePut(TimeQueueHandle, time_str, 0, 0);
      WIFI_MQTT_Publish("RADAR/TIME", time_str);
    } else {
      WIFI_MQTT_Publish("RADAR/TIME", "Get Time Failed");

    }
    osDelay(10000);  // 10 秒
  }
  /* USER CODE END StartTimeTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

