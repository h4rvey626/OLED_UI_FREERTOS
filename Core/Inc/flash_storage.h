#ifndef __FLASH_STORAGE_H
#define __FLASH_STORAGE_H

#include "main.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * STM32F411 512KB Flash sector layout (sector 7 is the last 128KB):
 * 0x08060000 ~ 0x0807FFFF
 */
#define FS_CONFIG_START_ADDR   (0x08060000U)
#define FS_CONFIG_SIZE_BYTES   (128U * 1024U)
#define FS_CONFIG_END_ADDR     (FS_CONFIG_START_ADDR + FS_CONFIG_SIZE_BYTES)
#define FS_CONFIG_SECTOR       (FLASH_SECTOR_7)

typedef enum
{
    FS_OK = 0,
    FS_ERR_PARAM,
    FS_ERR_RANGE,
    FS_ERR_HAL,
    FS_ERR_VERIFY
} FS_Status;

FS_Status FlashStorage_Init(void);

bool FlashStorage_IsInConfigRange(uint32_t address, uint32_t length);

FS_Status FlashStorage_Read(uint32_t address, void *buffer, uint32_t length);

FS_Status FlashStorage_EraseConfigSector(void);

/*
 * Programs bytes into Flash.
 * Notes:
 * 1) Flash bits can only transition from 1 -> 0 by programming.
 * 2) If data requires 0 -> 1, erase sector first.
 */
FS_Status FlashStorage_Write(uint32_t address, const void *data, uint32_t length);

#endif
