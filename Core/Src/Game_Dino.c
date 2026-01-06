#include "Game_Dino.h"
#include "Game_Dino_Data.h"
#include "OLED.h"
#include "encoder_driver.h"
#include "main.h"
#include <stdlib.h>
#include <math.h>
#include "MENU.h"
#include "input.h"
#include "cmsis_os.h"

/* 输入事件队列：由 InputTask 生产（读硬件），各 UI/游戏从队列消费 */
extern osMessageQueueId_t InputEventQueueHandle;

static void Game_FlushInputEvents(void)
{
    if (!InputEventQueueHandle) return;
    InputEvent evt;
    while (osMessageQueueGet(InputEventQueueHandle, &evt, NULL, 0) == osOK) {
        /* discard */
    }
}

static InputEvent Game_ReceiveInputEvent(uint32_t timeout_ms)
{
    InputEvent evt;
    evt.type = INPUT_NONE;
    evt.value = 0;

    if (!InputEventQueueHandle) return evt;
    if (osMessageQueueGet(InputEventQueueHandle, &evt, NULL, timeout_ms) == osOK) {
        return evt;
    }
    evt.type = INPUT_NONE;
    evt.value = 0;
    return evt;
}

// 游戏对象位置结构体
struct Object_Position{
    uint8_t minX,minY,maxX,maxY;
};

int Score;

// 显示分数
void Show_Score(void)
{
    OLED_ShowNum(98,0,Score,5,OLED_6X8);
}

uint16_t Ground_Pos;

// 显示地面，地面是存储在数组中的，直接写入OLED显存数组去
void Show_Ground(void)
{
    extern uint8_t OLED_DisplayBuf[8][128]; // 声明OLED显存
    
    if(Ground_Pos<128)
    {
        for(uint8_t i=0;i<128;i++)
        {
            OLED_DisplayBuf[7][i]=Ground[i+Ground_Pos];
        }
    }
    else
    {
        for(uint8_t i=0;i<255-Ground_Pos;i++)
        {
            OLED_DisplayBuf[7][i]=Ground[i+Ground_Pos];
        }
        for(uint8_t i=255-Ground_Pos;i<128;i++)
        {
            OLED_DisplayBuf[7][i]=Ground[i-(255-Ground_Pos)];
        }
    }
}

uint8_t barrier_flag;
uint8_t Barrier_Pos;

struct Object_Position barrier;

// 显示障碍物
void Show_Barrier(void)
{
    if(Barrier_Pos>=143)
    {
        barrier_flag=rand()%3;//在0，1，2中取随机数
    }
    
    OLED_ShowImage(127-Barrier_Pos,44,16,18,Barrier[barrier_flag]);
    
    barrier.minX=127-Barrier_Pos;
    barrier.maxX=143-Barrier_Pos;
    barrier.minY=44;
    barrier.maxY=62;
}

uint8_t Cloud_Pos;

// 显示云朵
void Show_Cloud(void)
{
    OLED_ShowImage(127-Cloud_Pos,9,16,8,Cloud);
}

uint8_t dino_jump_flag=0;//0:奔跑，1:跳跃
uint8_t KeyNum;
uint16_t jump_t;
uint8_t Jump_Pos;

struct Object_Position dino;

// 显示小恐龙
void Show_Dino(void)
{
    Jump_Pos=28*sin((float)(pi*jump_t/1000));
    
    if(dino_jump_flag==0)
    {
        if(Cloud_Pos%2==0)OLED_ShowImage(0,44,16,18,Dino[0]);
        else OLED_ShowImage(0,44,16,18,Dino[1]);
    }
    else
    {
        OLED_ShowImage(0,44-Jump_Pos,16,18,Dino[2]);
    }
    
    dino.minX=0;
    dino.maxX=16;
    dino.minY=44-Jump_Pos;
    dino.maxY=62-Jump_Pos;
}

// 碰撞检测函数，传入两个结构体的地址
int isColliding(struct Object_Position *a,struct Object_Position *b)
{
    if((a->maxX>b->minX)&&(a->minX<b->maxX)&&(a->maxY>b->minY)&&(a->minY<b->maxY))
    {
        OLED_Clear();
        OLED_ShowString(28,24,"Game Over",OLED_8X16);
        OLED_ShowString(24,40,"Score:",OLED_6X8);
        OLED_ShowNum(60,40,Score,3,OLED_6X8);
        OLED_Update();
        
        // 等待按键退出
        while(1)
        {
            InputEvent event = Game_ReceiveInputEvent(50);
            if(event.type == INPUT_ENTER)
            {
                MENU_UpdateActivity();
                break;
            }
        }
        return 1;
    }
    return 0;
}

// 游戏主循环
int DinoGame_Animation(void)
{
    while(1)
    {
        int return_flag;

        /* 输入从队列获取（避免与 InputTask 抢 Input_GetEvent） */
        InputEvent event = Game_ReceiveInputEvent(0);
        if (event.type != INPUT_NONE) {
            MENU_UpdateActivity();
        }

        // 检查是否按了返回键退出游戏
        if(event.type == INPUT_BACK || event.type == INPUT_ENTER)
        {
            return 0;
        }

        // 旋转触发跳跃（只在非跳跃状态时触发）
        if((event.type == INPUT_UP || event.type == INPUT_DOWN) && dino_jump_flag == 0)
        {
            dino_jump_flag = 1;
        }
        
        // 更新游戏状态 - 关键修复！
        Dino_Tick();
        
        OLED_Clear();
        Show_Score();
        Show_Ground();
        Show_Barrier();
        Show_Cloud();
        Show_Dino();
        OLED_Update();
        
        return_flag=isColliding(&dino,&barrier);
        if(return_flag==1)
        {
            return 0;
        }
        
        osDelay(40); // 控制游戏速度
    }
}

void Dino_Tick(void)
{
    static uint8_t Score_Count, Cloud_Count;
    Score_Count++;
    Cloud_Count++;

    // 分数：约每3帧增加一次 (原100次/1ms = 100ms，现3次/30ms = 90ms)
    if(Score_Count >= 5)
    {
        Score_Count = 0;
        Score++;
    }
    
    // 地面和障碍物：每帧都移动 (原20次/1ms = 20ms，现1次/30ms = 30ms)
    Ground_Pos++;
    Barrier_Pos++;
    if(Ground_Pos >= 256) Ground_Pos = 0;
    if(Barrier_Pos >= 144) Barrier_Pos = 0;
    
    // 云：约每2帧移动一次 (原50次/1ms = 50ms，现2次/30ms = 60ms)
    if(Cloud_Count >= 2)
    {
        Cloud_Count = 0;
        Cloud_Pos++;
        if(Cloud_Pos >= 200) Cloud_Pos = 0;
    }
    
    // 跳跃：每帧增加30 (原1000次/1ms = 1秒跳跃，现1000/30 ≈ 33帧 = 1秒跳跃)
    if(dino_jump_flag == 1)
    {
        jump_t += 30;
        if(jump_t >= 1000)
        {
            jump_t = 0;
            dino_jump_flag = 0;
        }
    }
}

void DinoGame_Pos_Init(void)
{
    Score=Ground_Pos=Barrier_Pos=Cloud_Pos=Jump_Pos=0;
    jump_t=0;
    dino_jump_flag=0;
    barrier_flag=0;
}

// 游戏初始化和启动
void Game_Dino_Init(void)
{
    /* 进入游戏前先清空积压输入，避免误触发 */
    Game_FlushInputEvents();

    // 初始化游戏状态
    DinoGame_Pos_Init();
    
    // 显示游戏开始画面
    OLED_Clear();
    OLED_ShowString(32, 16, "Dino Game", OLED_8X16);
    OLED_ShowString(8, 32, "Rotate to Jump", OLED_6X8);
    OLED_ShowString(8, 40, "PA2 to Start/Exit", OLED_6X8);
    OLED_Update();
    
    // 等待按键开始游戏
    while(1)
    {
        InputEvent event = Game_ReceiveInputEvent(50);
        if(event.type == INPUT_ENTER)
        {
            MENU_UpdateActivity();
            break;
        }
    }
    
    // 开始游戏
    DinoGame_Animation();
}
