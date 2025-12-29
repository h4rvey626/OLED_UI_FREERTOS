#ifndef __GAME_DINO_DATA_H
#define __GAME_DINO_DATA_H

#include <stdint.h>

// 恐龙角色图像数据 (16x18像素)
// 三种状态：跑步1、跑步2、跳跃
extern const uint8_t Dino[3][36];

// 障碍物图像数据 (16x18像素)
// 三种不同高度的仙人掌
extern const uint8_t Barrier[3][36];

// 云朵图像数据 (16x8像素)
extern const uint8_t Cloud[16];

// 地面图像数据 (256像素宽，滚动显示)
extern const uint8_t Ground[256];

// 数学常量
extern const double pi;

#endif
