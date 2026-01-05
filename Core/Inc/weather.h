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


/* 初始化默认数据（可选：Weather_Run 内部也会懒初始化） */
void Weather_InitDefault(void);

/* 运行天气全屏界面（旋钮左右切换城市，按键退出） */
void Weather_Run(void);

/* 数据访问接口：便于后续 API 更新天气 */
size_t Weather_GetCityCount(void);
const WeatherData_t* Weather_GetCity(size_t idx);
bool Weather_SetCity(size_t idx, const WeatherData_t* w);
bool Weather_AddCity(const WeatherData_t* w);


#endif /* INC_WEATHER_H_ */


