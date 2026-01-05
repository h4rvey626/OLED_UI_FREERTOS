#ifndef INC_WEATHER_H_
#define INC_WEATHER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* 天气数据结构体（后续对接 API 时复用） */
typedef struct {
    char city[16];          // 城市名
    char weather[16];       // 天气描述 (Sunny, Cloudy, Rainy...)
    int8_t temp_high;       // 最高温度
    int8_t temp_low;        // 最低温度
    uint8_t humidity;       // 湿度 (0-100)
    uint8_t wind_level;     // 风力等级 (0-12)
} WeatherData_t;

/* 和风天气 3 天预报 daily 单天数据（仅保留常用字段） */
typedef struct {
    char fxDate[11];        // "YYYY-MM-DD"
    char textDay[16];       // 白天天气文字（如：晴）
    int8_t tempMax;         // 最高温（摄氏）
    int8_t tempMin;         // 最低温（摄氏）
    uint8_t humidity;       // 湿度 (0-100)
    uint8_t windLevel;      // 风力等级（从 windScaleDay 解析，取上限）
} QWeatherDaily_t;

/* 初始化默认数据（可选：Weather_Run 内部也会懒初始化） */
void Weather_InitDefault(void);

/* 运行天气全屏界面（旋钮左右切换城市，按键退出） */
void Weather_Run(void);

/* 数据访问接口：便于后续 API 更新天气 */
size_t Weather_GetCityCount(void);
const WeatherData_t* Weather_GetCity(size_t idx);
bool Weather_SetCity(size_t idx, const WeatherData_t* w);
bool Weather_AddCity(const WeatherData_t* w);

/* 解析和风天气 3 天预报 JSON，提取 daily[0]（当天） */
bool Weather_ParseQWeather3dToday(const char* json, QWeatherDaily_t* out);

#endif /* INC_WEATHER_H_ */


