/*
 * weather.c
 *
 *  Created on: Jan 5, 2026
 *      Author: 10637
 */

#include "weather.h"

#include "MENU.h"     // 使用 MENU_ReceiveInputEvent / MENU_UpdateActivity
#include "OLED.h"
#include "cmsis_os.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* 可支持的最大城市数（后续从 API 加城市也方便） */
#define WEATHER_MAX_CITIES 10

/* 动画速度（独立于 MENU.c，避免耦合） */
#define WEATHER_ANIMATION_SPEED 0.30f

static WeatherData_t s_weather_list[WEATHER_MAX_CITIES];
static uint8_t s_weather_count = 0;

/* --------------------------- QWeather JSON 解析（轻量） --------------------------- */

/* 在 [begin, end) 区间内查找 needle，返回首次命中位置或 NULL */
static const char* bounded_strstr(const char* begin, const char* end, const char* needle)
{
    if (!begin || !end || !needle) return NULL;
    size_t nlen = strlen(needle);
    if (nlen == 0) return begin;
    if ((size_t)(end - begin) < nlen) return NULL;

    const char* p = begin;
    const char* last = end - nlen;
    for (; p <= last; p++) {
        if (*p == *needle && memcmp(p, needle, nlen) == 0) {
            return p;
        }
    }
    return NULL;
}

/* 从一个 JSON 对象片段中取字符串字段："key":"value"（value 不含转义处理，适用于和风返回） */
static bool json_obj_get_string(const char* obj, size_t obj_len, const char* key, char* out, size_t out_sz)
{
    if (!obj || obj_len == 0 || !key || !out || out_sz == 0) return false;
    const char* begin = obj;
    const char* end = obj + obj_len;

    char pattern[64];
    int n = snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
    if (n <= 0 || (size_t)n >= sizeof(pattern)) return false;

    const char* p = bounded_strstr(begin, end, pattern);
    if (!p) return false;
    p += strlen(pattern);

    const char* q = p;
    while (q < end && *q != '\"') q++;
    if (q >= end) return false;

    size_t len = (size_t)(q - p);
    if (len >= out_sz) len = out_sz - 1;
    memcpy(out, p, len);
    out[len] = '\0';
    return true;
}

/* "1-3" -> 3, "3"->3；解析失败返回 0 */
static uint8_t parse_wind_scale_upper(const char* s)
{
    if (!s || !*s) return 0;

    // 读第一个数字
    while (*s && !isdigit((unsigned char)*s)) s++;
    if (!*s) return 0;
    int a = (int)strtol(s, (char**)&s, 10);

    // 跳到分隔符后的数字（- 或 ~）
    while (*s && *s != '-' && *s != '~') s++;
    if (*s == '-' || *s == '~') {
        s++;
        while (*s && !isdigit((unsigned char)*s)) s++;
        if (*s) {
            int b = (int)strtol(s, NULL, 10);
            if (b > a) a = b;
        }
    }

    if (a < 0) a = 0;
    if (a > 12) a = 12;
    return (uint8_t)a;
}

/* 找到 daily 数组第一个对象 { ... }，返回对象起始指针并输出长度 */
static const char* find_daily0_object(const char* json, size_t* out_obj_len)
{
    if (out_obj_len) *out_obj_len = 0;
    if (!json) return NULL;

    const char* daily = strstr(json, "\"daily\"");
    if (!daily) return NULL;

    const char* arr = strchr(daily, '[');
    if (!arr) return NULL;

    const char* p = strchr(arr, '{');
    if (!p) return NULL;

    // 计数匹配大括号，定位对象结束
    int depth = 0;
    const char* q = p;
    for (; *q; q++) {
        if (*q == '{') depth++;
        else if (*q == '}') {
            depth--;
            if (depth == 0) {
                if (out_obj_len) *out_obj_len = (size_t)(q - p + 1);
                return p;
            }
        }
    }
    return NULL;
}

bool Weather_ParseQWeather3dToday(const char* json, QWeatherDaily_t* out)
{
    if (!json || !out) return false;
    memset(out, 0, sizeof(*out));

    // 可选：快速检查 code==200
    if (!strstr(json, "\"code\":\"200\"")) {
        return false;
    }

    size_t obj_len = 0;
    const char* obj = find_daily0_object(json, &obj_len);
    if (!obj || obj_len == 0) return false;

    char tmp[32];

    if (json_obj_get_string(obj, obj_len, "fxDate", out->fxDate, sizeof(out->fxDate)) == false) {
        // fxDate 非关键字段，允许缺失
        out->fxDate[0] = '\0';
    }

    if (json_obj_get_string(obj, obj_len, "textDay", out->textDay, sizeof(out->textDay)) == false) {
        out->textDay[0] = '\0';
    }

    if (json_obj_get_string(obj, obj_len, "tempMax", tmp, sizeof(tmp))) {
        out->tempMax = (int8_t)atoi(tmp);
    }
    if (json_obj_get_string(obj, obj_len, "tempMin", tmp, sizeof(tmp))) {
        out->tempMin = (int8_t)atoi(tmp);
    }
    if (json_obj_get_string(obj, obj_len, "humidity", tmp, sizeof(tmp))) {
        int h = atoi(tmp);
        if (h < 0) h = 0;
        if (h > 100) h = 100;
        out->humidity = (uint8_t)h;
    }
    if (json_obj_get_string(obj, obj_len, "windScaleDay", tmp, sizeof(tmp))) {
        out->windLevel = parse_wind_scale_upper(tmp);
    }

    return true;
}

void Weather_InitDefault(void)
{
    /* 默认模拟数据（后续可由 API 覆盖） */
    memset(s_weather_list, 0, sizeof(s_weather_list));
    s_weather_count = 5;

    strncpy(s_weather_list[0].city, "Beijing", sizeof(s_weather_list[0].city) - 1);
    strncpy(s_weather_list[0].weather, "Sunny", sizeof(s_weather_list[0].weather) - 1);
    s_weather_list[0].temp_high = 5;  s_weather_list[0].temp_low = -3;
    s_weather_list[0].humidity = 35;  s_weather_list[0].wind_level = 3;

    strncpy(s_weather_list[1].city, "Shanghai", sizeof(s_weather_list[1].city) - 1);
    strncpy(s_weather_list[1].weather, "Cloudy", sizeof(s_weather_list[1].weather) - 1);
    s_weather_list[1].temp_high = 8;  s_weather_list[1].temp_low = 2;
    s_weather_list[1].humidity = 65;  s_weather_list[1].wind_level = 2;

    strncpy(s_weather_list[2].city, "Shenzhen", sizeof(s_weather_list[2].city) - 1);
    strncpy(s_weather_list[2].weather, "Rainy", sizeof(s_weather_list[2].weather) - 1);
    s_weather_list[2].temp_high = 18; s_weather_list[2].temp_low = 12;
    s_weather_list[2].humidity = 85;  s_weather_list[2].wind_level = 4;

    strncpy(s_weather_list[3].city, "Guangzhou", sizeof(s_weather_list[3].city) - 1);
    strncpy(s_weather_list[3].weather, "Overcast", sizeof(s_weather_list[3].weather) - 1);
    s_weather_list[3].temp_high = 16; s_weather_list[3].temp_low = 10;
    s_weather_list[3].humidity = 70;  s_weather_list[3].wind_level = 2;

    strncpy(s_weather_list[4].city, "Hangzhou", sizeof(s_weather_list[4].city) - 1);
    strncpy(s_weather_list[4].weather, "Sunny", sizeof(s_weather_list[4].weather) - 1);
    s_weather_list[4].temp_high = 6;  s_weather_list[4].temp_low = 0;
    s_weather_list[4].humidity = 45;  s_weather_list[4].wind_level = 1;
}

size_t Weather_GetCityCount(void)
{
    return (size_t)s_weather_count;
}

const WeatherData_t* Weather_GetCity(size_t idx)
{
    if (idx >= s_weather_count) return NULL;
    return &s_weather_list[idx];
}

bool Weather_SetCity(size_t idx, const WeatherData_t* w)
{
    if (!w) return false;
    if (idx >= WEATHER_MAX_CITIES) return false;

    /* 如果 idx 超过当前 count，则扩展 count */
    if (idx >= s_weather_count) {
        s_weather_count = (uint8_t)(idx + 1);
    }

    s_weather_list[idx] = *w;
    /* 确保字符串以 \0 结尾 */
    s_weather_list[idx].city[sizeof(s_weather_list[idx].city) - 1] = '\0';
    s_weather_list[idx].weather[sizeof(s_weather_list[idx].weather) - 1] = '\0';
    return true;
}

bool Weather_AddCity(const WeatherData_t* w)
{
    if (!w) return false;
    if (s_weather_count >= WEATHER_MAX_CITIES) return false;
    return Weather_SetCity((size_t)s_weather_count, w);
}

/* 绘制单个天气卡片（offset_x: 0 为居中，±128 用于左右滑动） */
static void Weather_DrawCard(const WeatherData_t *w, int16_t offset_x)
{
    if (!w) return;

    char buf[32];
    int16_t x = offset_x;

    /* 城市名 - 大字体居中 */
    int city_len = (int)strlen(w->city);
    int16_t city_x = x + (128 - city_len * 8) / 2;
    OLED_ShowString(city_x, 0, (char*)w->city, OLED_8X16);

    /* 天气描述 */
    int weather_len = (int)strlen(w->weather);
    int16_t weather_x = x + (128 - weather_len * 6) / 2;
    OLED_ShowString(weather_x, 18, (char*)w->weather, OLED_6X8);

    /* 温度显示 */
    snprintf(buf, sizeof(buf), "%d~%dC", (int)w->temp_low, (int)w->temp_high);
    int temp_len = (int)strlen(buf);
    int16_t temp_x = x + (128 - temp_len * 8) / 2;
    OLED_ShowString(temp_x, 30, buf, OLED_8X16);

    /* 湿度和风力 */
    snprintf(buf, sizeof(buf), "Hum:%d%%  Wind:%d", (int)w->humidity, (int)w->wind_level);
    OLED_ShowString(x + 10, 50, buf, OLED_6X8);
}

/* 绘制页面指示器（底部方块） */
static void Weather_DrawIndicator(uint8_t current, uint8_t total)
{
    if (total == 0) return;
    int16_t start_x = (128 - total * 8) / 2;
    for (uint8_t i = 0; i < total; i++) {
        int16_t cx = start_x + i * 8 + 3;
        if (i == current) {
            OLED_DrawRectangle(cx, 60, 4, 3, OLED_FILLED);
        } else {
            OLED_DrawRectangle(cx, 60, 4, 3, OLED_UNFILLED);
        }
    }
}

void Weather_Run(void)
{
    if (s_weather_count == 0) {
        Weather_InitDefault();
    }

    uint8_t current_idx = 0;
    uint8_t from_idx = 0;         // 动画开始时的旧页面
    uint8_t to_idx = 0;           // 动画目标页面（current_idx 同步为 to_idx）
    int16_t anim_offset = 0;      // 动画偏移量
    int16_t target_offset = 0;    // 目标偏移量（始终为 0）
    uint8_t is_animating = 0;     // 是否正在动画中
    int8_t swipe_dir = 0;         // 滑动方向 (-1=左, 1=右)

    while (1)
    {
        InputEvent event = MENU_ReceiveInputEvent();

        if (event.type == INPUT_ENTER || event.type == INPUT_BACK) {
            MENU_UpdateActivity();
            return;
        }

        /* 旋转切换城市（动画期间不响应，避免状态叠加） */
        if (!is_animating) {
            if (event.type == INPUT_DOWN) { // 顺时针 -> 下一个城市
                if (s_weather_count > 1) {
                    from_idx = current_idx;
                    to_idx = (uint8_t)((current_idx + 1) % s_weather_count); // 环形：最后一个也能到第一个
                    current_idx = to_idx;
                    swipe_dir = -1;      // 新卡片从右往左滑入
                    is_animating = 1;
                    anim_offset = 128;   // 新卡片从右边进入
                    MENU_UpdateActivity();
                }
            } else if (event.type == INPUT_UP) { // 逆时针 -> 上一个城市
                if (s_weather_count > 1) {
                    from_idx = current_idx;
                    to_idx = (uint8_t)((current_idx + s_weather_count - 1) % s_weather_count); // 环形：第一个也能到最后一个
                    current_idx = to_idx;
                    swipe_dir = 1;       // 新卡片从左往右滑入
                    is_animating = 1;
                    anim_offset = -128;  // 新卡片从左边进入
                    MENU_UpdateActivity();
                }
            }
        }

        /* 动画更新：确保至少移动 1 像素，避免卡死 */
        if (is_animating) {
            int16_t diff = (int16_t)(target_offset - anim_offset);
            if (diff > 2 || diff < -2) {
                int16_t step = (int16_t)((float)diff * WEATHER_ANIMATION_SPEED);
                if (step == 0) step = (diff > 0) ? 1 : -1;
                anim_offset = (int16_t)(anim_offset + step);
            } else {
                anim_offset = target_offset;
                is_animating = 0;
                swipe_dir = 0;
            }
        }

        /* 绘制界面 */
        OLED_Clear();

        if (s_weather_count == 0) {
            OLED_ShowString(10, 24, "No city data", OLED_6X8);
            OLED_Update();
            osDelay(50);
            continue;
        }

        if (is_animating && swipe_dir != 0) {
            /* current_idx 此时是新页面（to_idx） */
            Weather_DrawCard(&s_weather_list[to_idx], anim_offset);

            /* 旧页面（from_idx）滑出：根据方向放到相反侧 */
            if (swipe_dir == -1) {
                // 新从右进(128->0)，旧从中间滑到左侧(0->-128)
                Weather_DrawCard(&s_weather_list[from_idx], (int16_t)(anim_offset - 128));
            } else if (swipe_dir == 1) {
                // 新从左进(-128->0)，旧从中间滑到右侧(0->128)
                Weather_DrawCard(&s_weather_list[from_idx], (int16_t)(anim_offset + 128));
            }
        } else {
            Weather_DrawCard(&s_weather_list[current_idx], 0);
        }

        Weather_DrawIndicator(current_idx, s_weather_count);
        OLED_Update();
        osDelay(16); // ~60fps
    }
}


