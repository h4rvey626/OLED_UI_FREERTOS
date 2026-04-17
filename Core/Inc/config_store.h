#ifndef __CONFIG_STORE_H
#define __CONFIG_STORE_H

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_MAGIC_VALUE   (0x434F4E46U)
#define CONFIG_VERSION       (2U)
#define CONFIG_FLUSH_DELAY   (4000U)

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint8_t  brightness;
    uint8_t  reserved8;
    uint16_t auto_sleep_sec;
    uint16_t timer_default_sec;
    char     wifi_ssid[32];
    char     wifi_password[64];
    uint32_t crc32;
} AppConfig;

void Config_Load(void);
void Config_MarkDirty(void);
void Config_FlushIfNeeded(void);
void Config_ResetDefaults(void);
const AppConfig *Config_Get(void);
bool Config_SetWiFiCredentials(const char *ssid, const char *password);

#endif
