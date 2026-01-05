#include "Game_Snake.h"
#include "Game_Snake_Data.h"
#include "OLED.h"
#include "TIM_EC11.h"
#include "main.h"
#include "MENU.h"
#include "input.h"
#include <stdlib.h>
#include <stdio.h>
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

extern uint8_t OLED_DisplayBuf[8][128];		//把OLED显存拿过来

Tile Map[8][16];
uint8_t Game_Speed = 200;		//游戏速度(延时)
uint8_t Game_Credits = 0;	//游戏积分

void Game_Snake_Show_Tile_8x8(uint8_t Y, uint8_t X, Tile Tile)
{
	for(int8_t i = 0; i < 8; i++)
	{
		OLED_DisplayBuf[Y][X * 8 + i] = (Game_Snake_Tile_8x8[Tile][i]);			//显示区块
	}
}

void Map_Clear(void)	//清除地图
{
	int8_t i, j;
	for(i=0; i<8; i++){
		for(j=0; j<16; j++){
			Map[i][j] = air;
		}
	}
}

void Map_Update(void)		//上传地图
{
	int8_t i, j;
	for(i=0; i<8; i++){
		for(j=0; j<16; j++){
			Game_Snake_Show_Tile_8x8(i, j, Map[i][j]);
		}
	}
	OLED_DrawRectangle(0, 0, 128, 64, 0);
}

void RandFood(void)	//随机放置食物
{
	srand(HAL_GetTick()); // 使用HAL_GetTick()作为随机种子
	int8_t Y = rand()%8;
	int8_t X = rand()%16;
	while(Map[Y][X] != air)
	{
		Y = rand()%8;
		X = rand()%16;
	}
	Map[Y][X] = food;
}

Tile Game_Snake_GetFront(Game_Snake_Class* Snake)	//获取前方信息
{
	switch(Snake->Heading)
	{
		case up:	{return Map[Snake->H_Y-1][Snake->H_X];}
		case right:	{return Map[Snake->H_Y][Snake->H_X+1];}
		case down:	{return Map[Snake->H_Y+1][Snake->H_X];}
		case left:	{return Map[Snake->H_Y][Snake->H_X-1];}
	}
	return air;
}

uint8_t Game_Snake_Advance(Game_Snake_Class* Snake)//把蛇映射到地图
{
	uint8_t Front_X = Snake->H_X, Front_Y = Snake->H_Y;
	
	switch(Snake->Heading)
	{
		case up:	{Front_Y = Snake->H_Y-1; break;}
		case right:	{Front_X = Snake->H_X+1; break;}
		case down:	{Front_Y = Snake->H_Y+1; break;}
		case left:	{Front_X = Snake->H_X-1; break;}
	}
	Front_X %= 16;
	Front_Y %= 8;
	
	if(Map[Front_Y][Front_X] == air)							//如果前方为空气		
	{
		*Snake->node[Snake->Head_i] = SnakeBody;				//蛇头节点指向的地图方块变为蛇身
		Snake->Head_i = (Snake->Head_i + 1) % 128;				//蛇头节点下标前进1格
		Snake->node[Snake->Head_i] = &Map[Front_Y][Front_X];	//蛇头节点指向到前方地图方块
		*Snake->node[Snake->Head_i] = SnakeHead;				//蛇头节点指向的地图方块变为蛇头
		
		*Snake->node[Snake->Tail_i] = air;						//消除蛇尾地图方块
		Snake->Tail_i = (Snake->Tail_i + 1) % 128;				//蛇尾节点下标前进1格
		*Snake->node[Snake->Tail_i] = SnakeTail;				//蛇尾节点指向的地图方块变为蛇尾
		
		Snake->H_X = Front_X;									//蛇头坐标更新
		Snake->H_Y = Front_Y;
	}
	else if(Map[Front_Y][Front_X] == food)						//如果前方为食物
	{
		*Snake->node[Snake->Head_i] = SnakeBody;				//蛇头节点指向的地图方块变为蛇身
		Snake->Head_i = (Snake->Head_i + 1) % 128;				//蛇头节点下标前进1格
		Snake->node[Snake->Head_i] = &Map[Front_Y][Front_X];	//蛇头节点指向到前方地图方块
		*Snake->node[Snake->Head_i] = SnakeHead;				//蛇头节点指向的地图方块变为蛇头

		RandFood();											//随机放置食物
		Game_Credits += 1;									//加积分
		if(Game_Speed > 50) Game_Speed -= Game_Speed/8;		//减延时，最小50ms
		
		Snake->H_X = Front_X;								//蛇头坐标更新
		Snake->H_Y = Front_Y;
	}
	else				//前方有障碍
	{
		return 0;		//前进失败
	}
	
	return 1;			//前进成功
}

void Game_Snake_Play(Game_Snake_Class* Snake)		//开始游戏
{
	while(Snake->Head_i - Snake->Tail_i < 3)		//出身点随机方向强制移动三格;
	{
		Snake->H_X++;
		if(Snake->H_X >= 16) Snake->H_X = 0; // 防止越界
		*Snake->node[Snake->Head_i] = SnakeBody;					//蛇头节点指向的地图方块变为蛇身
		Snake->Head_i = (Snake->Head_i + 1) % 128;					//蛇头节点下标前进1格
		Snake->node[Snake->Head_i] = &Map[Snake->H_Y][Snake->H_X];	//蛇头节点指向到前方地图方块
		*Snake->node[Snake->Head_i] = SnakeHead;					//蛇头节点指向的地图方块变为蛇头
		
		Map_Update();
		OLED_Update();
		osDelay(Game_Speed);
	}
	WSAD Heading_Previous = Snake->Heading; 
	int16_t temp = 0;

	
	while(1)	//主循环
	{		
		InputEvent event = Game_ReceiveInputEvent(0);
		if (event.type != INPUT_NONE) {
			MENU_UpdateActivity();
		}
		if(event.type == INPUT_BACK || event.type == INPUT_ENTER) {return;}	//退出游戏

		// 旋转编码器改变方向：顺时针(Down)右转，逆时针(Up)左转
		if(event.type == INPUT_DOWN)
		{
			Snake->Heading = (Snake->Heading + (event.value % 4)) % 4;
		}
		else if(event.type == INPUT_UP)
		{
			Snake->Heading = (Snake->Heading + 4 - (event.value % 4)) % 4;
		}
		
		if(Game_Snake_Advance(Snake)){Heading_Previous = Snake->Heading;}	//如果前进成功则记录方向
		else
		{
			Snake->Heading = Heading_Previous; 		//如果前进失败尝试之前的方向再试一次
			if(Game_Snake_Advance(Snake) == 0)		//如果仍然失败则游戏结束
			{
				OLED_Clear();
				OLED_ShowString(20, 10, "Game Over!", OLED_8X16);
				char score_str[32];
				sprintf(score_str, "Score: %d", Game_Credits);
				OLED_ShowString(30, 30, score_str, OLED_8X16);
				OLED_ShowString(15, 45, "Press to exit", OLED_6X8);
				OLED_Update();
				
				while(1){
				InputEvent evt = Game_ReceiveInputEvent(50);
				if(evt.type == INPUT_BACK || evt.type == INPUT_ENTER) {return;}	//退出游戏
				if (evt.type != INPUT_NONE) MENU_UpdateActivity();
				}
			}
		}
		
		Map_Update();
		OLED_Update();
		osDelay(Game_Speed);
	}
}

void Game_Snake_Init(void)
{
	/* 进入游戏前先清空积压输入，避免误触发 */
	Game_FlushInputEvents();

	Game_Credits = 0;
	Game_Speed = 200;	
	Map_Clear();		//清除蛇尸
	
	// 初始化随机种子
	srand(HAL_GetTick());
	
	Game_Snake_Class Snake_1;
		Snake_1.Head_i = 0;
		Snake_1.Tail_i = 0;
		Snake_1.H_X = rand()%16;
		Snake_1.H_Y = rand()%8;
		Snake_1.Heading = right;
		Snake_1.node[Snake_1.Head_i] = &Map[Snake_1.H_Y][Snake_1.H_X];

	Map[Snake_1.H_Y][Snake_1.H_X] = SnakeHead;
	
	RandFood();
	
	Map_Update();
	OLED_Update();
	Game_Snake_Play(&Snake_1);
	
	// 清理按键状态，避免退出后误触发
	Game_FlushInputEvents();
	return;
}
