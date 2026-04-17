#include "config_store.h"
#include "flash_storage.h"
#include "main.h"
#include <string.h>

extern uint8_t  oled_brightness;
extern uint16_t auto_sleep_seconds;
extern uint16_t timer_seconds;

static AppConfig g_config;
static volatile uint8_t g_dirty = 0;
static uint32_t g_dirty_tick = 0;

static uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFU;
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint32_t j = 0; j < 8; j++)
        {
            crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1U));
        }
    }
    return ~crc;
}

static size_t bounded_strnlen(const char *s, size_t max_len)
{
    size_t n = 0U;

    if (s == NULL)
    {
        return 0U;
    }

    while ((n < max_len) && (s[n] != '\0'))
    {
        n++;
    }

    return n;
}

static void set_defaults(void)
{
    (void)memset(&g_config, 0, sizeof(g_config));
    g_config.magic = CONFIG_MAGIC_VALUE;
    g_config.version = CONFIG_VERSION;
    g_config.brightness = 128;
    g_config.auto_sleep_sec = 120;
    g_config.timer_default_sec = 60;
    g_config.wifi_ssid[0] = '\0';
    g_config.wifi_password[0] = '\0';
}

static uint32_t config_crc(void)
{
    uint32_t payload_len = (uint32_t)((uintptr_t)&g_config.crc32 - (uintptr_t)&g_config);
    return crc32_calc((const uint8_t *)&g_config, payload_len);
}

static void sync_to_globals(void)
{
    g_config.wifi_ssid[sizeof(g_config.wifi_ssid) - 1U] = '\0';
    g_config.wifi_password[sizeof(g_config.wifi_password) - 1U] = '\0';
    oled_brightness = g_config.brightness;
    auto_sleep_seconds = g_config.auto_sleep_sec;
    timer_seconds = g_config.timer_default_sec;
}

static void sync_from_globals(void)
{
    g_config.brightness = oled_brightness;
    g_config.auto_sleep_sec = auto_sleep_seconds;
    g_config.timer_default_sec = timer_seconds;
}

bool Config_SetWiFiCredentials(const char *ssid, const char *password)
{
    size_t ssid_len;
    size_t pwd_len;

    if ((ssid == NULL) || (password == NULL))
    {
        return false;
    }

    ssid_len = bounded_strnlen(ssid, sizeof(g_config.wifi_ssid));
    pwd_len = bounded_strnlen(password, sizeof(g_config.wifi_password));

    if ((ssid_len == 0U) || (ssid_len >= sizeof(g_config.wifi_ssid)))
    {
        return false;
    }

    if (pwd_len >= sizeof(g_config.wifi_password))
    {
        return false;
    }

    (void)memset(g_config.wifi_ssid, 0, sizeof(g_config.wifi_ssid));
    (void)memset(g_config.wifi_password, 0, sizeof(g_config.wifi_password));
    (void)memcpy(g_config.wifi_ssid, ssid, ssid_len);
    (void)memcpy(g_config.wifi_password, password, pwd_len);

    Config_MarkDirty();
    return true;
}

void Config_Load(void)
{
    set_defaults();

    AppConfig tmp;
    FS_Status st = FlashStorage_Read(FS_CONFIG_START_ADDR, &tmp, sizeof(tmp));
    if (st != FS_OK)
    {
        sync_to_globals();
        return;
    }

    if (tmp.magic != CONFIG_MAGIC_VALUE)
    {
        sync_to_globals();
        return;
    }

    if (tmp.version != CONFIG_VERSION)
    {
        sync_to_globals();
        return;
    }

    uint32_t payload_len = (uint32_t)((uintptr_t)&tmp.crc32 - (uintptr_t)&tmp);
    uint32_t expected = crc32_calc((const uint8_t *)&tmp, payload_len);
    if (tmp.crc32 != expected)
    {
        sync_to_globals();
        return;
    }

    (void)memcpy(&g_config, &tmp, sizeof(g_config));
    sync_to_globals();
}

void Config_MarkDirty(void)
{
    g_dirty = 1;
    g_dirty_tick = HAL_GetTick();
}

void Config_FlushIfNeeded(void)
{
    if (!g_dirty)
    {
        return;
    }

    if ((HAL_GetTick() - g_dirty_tick) < CONFIG_FLUSH_DELAY)
    {
        return;
    }

    g_dirty = 0;

    sync_from_globals();
    g_config.magic = CONFIG_MAGIC_VALUE;
    g_config.version = CONFIG_VERSION;
    g_config.crc32 = config_crc();

    (void)FlashStorage_EraseConfigSector();
    (void)FlashStorage_Write(FS_CONFIG_START_ADDR, &g_config, sizeof(g_config));
}

void Config_ResetDefaults(void)
{
    set_defaults();
    sync_to_globals();
    (void)FlashStorage_EraseConfigSector();
    g_config.crc32 = config_crc();
    (void)FlashStorage_Write(FS_CONFIG_START_ADDR, &g_config, sizeof(g_config));
}

const AppConfig *Config_Get(void)
{
    return &g_config;
}
