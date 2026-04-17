#include "flash_storage.h"

#include <string.h>

static FS_Status fs_flash_wait_ready(void)
{
    HAL_StatusTypeDef st = FLASH_WaitForLastOperation(1000U);
    return (st == HAL_OK) ? FS_OK : FS_ERR_HAL;
}

FS_Status FlashStorage_Init(void)
{
    return FS_OK;
}

bool FlashStorage_IsInConfigRange(uint32_t address, uint32_t length)
{
    uint32_t end;

    if (length == 0U)
    {
        return false;
    }

    end = address + length;
    if (end < address)
    {
        return false;
    }

    if (address < FS_CONFIG_START_ADDR)
    {
        return false;
    }

    if (end > FS_CONFIG_END_ADDR)
    {
        return false;
    }

    return true;
}

FS_Status FlashStorage_Read(uint32_t address, void *buffer, uint32_t length)
{
    if ((buffer == NULL) || (length == 0U))
    {
        return FS_ERR_PARAM;
    }

    if (!FlashStorage_IsInConfigRange(address, length))
    {
        return FS_ERR_RANGE;
    }

    (void)memcpy(buffer, (const void *)address, length);
    return FS_OK;
}

FS_Status FlashStorage_EraseConfigSector(void)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t sector_error = 0U;
    HAL_StatusTypeDef hal_st;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.Sector = FS_CONFIG_SECTOR;
    erase.NbSectors = 1U;

    hal_st = HAL_FLASHEx_Erase(&erase, &sector_error);
    if (hal_st != HAL_OK)
    {
        HAL_FLASH_Lock();
        return FS_ERR_HAL;
    }

    if (fs_flash_wait_ready() != FS_OK)
    {
        HAL_FLASH_Lock();
        return FS_ERR_HAL;
    }

    HAL_FLASH_Lock();
    return FS_OK;
}

FS_Status FlashStorage_Write(uint32_t address, const void *data, uint32_t length)
{
    const uint8_t *src = (const uint8_t *)data;
    uint32_t addr = address;
    uint32_t remain = length;
    HAL_StatusTypeDef hal_st;

    if ((data == NULL) || (length == 0U))
    {
        return FS_ERR_PARAM;
    }

    if (!FlashStorage_IsInConfigRange(address, length))
    {
        return FS_ERR_RANGE;
    }

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    while (remain > 0U)
    {
        uint32_t word = 0xFFFFFFFFU;
        uint32_t chunk = (remain >= 4U) ? 4U : remain;

        (void)memcpy(&word, src, chunk);

        hal_st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, (uint64_t)word);
        if (hal_st != HAL_OK)
        {
            HAL_FLASH_Lock();
            return FS_ERR_HAL;
        }

        if (*(volatile uint32_t *)addr != word)
        {
            HAL_FLASH_Lock();
            return FS_ERR_VERIFY;
        }

        addr += 4U;
        src += chunk;
        remain -= chunk;
    }

    HAL_FLASH_Lock();
    return FS_OK;
}
