# OLED_UI_FREERTOS 智能手环

基于 STM32F411 和 FreeRTOS 构建的智能手环项目，集成了 OLED UI 菜单系统、WiFi 联网、MQTT 通信和网络时间同步等功能。项目实现了丝滑的 UI 动画效果和模块化的应用框架。

## 📸 项目概览

*   **核心平台**: STM32F411CEU6 (BlackPill)
*   **操作系统**: FreeRTOS
*   **显示**: 0.96寸 OLED (SSD1306, SPI)
*   **连接**: ESP8266 WiFi 模块 (AT指令集)
*   **交互**: 旋转编码器 (EC11)

## 🛠 技术栈

### 硬件层 (Hardware Layer)
*   **MCU**: STM32F411CEU6 (Cortex-M4F, 100MHz)
*   **Display**: SSD1306 OLED (SPI 驱动)
*   **Network**: ESP8266 WiFi Module (UART 接口)
*   **Input**: EC11 旋转编码器 (定时器编码器模式 + 外部中断)

### 软件架构 (Software Architecture)
*   **RTOS**: FreeRTOS
    *   **多任务设计**:
        *   `InputTask`: 处理旋转编码器和按键输入 (高优先级)
        *   `MenuTask`: 负责 UI 渲染和页面逻辑 (中优先级)
        *   `WIFITask`: 处理 WiFi 连接和 MQTT 通信 (低优先级)
        *   `TimeTask`: 处理 SNTP 网络授时 (低优先级)
    *   **IPC (进程间通信)**:
        *   `MessageQueue`: 传递输入事件 (Input -> Menu) 和时间数据 (Time -> Menu)
        *   `EventFlags`: 同步任务状态 (如 WiFi 连接完成、SNTP 同步完成)
*   **UI 框架**: 自研 OLED_UI
    *   **特性**: 页面管理、平滑滚动动画 (光标/列表/滚动条)、弹窗机制、自动息屏
    *   **解耦**: 逻辑层与驱动层分离，通过回调函数 `menu_command_callback` 统一管理
*   **网络协议**:
    *   **MQTT**: 发布设备状态和时间信息到云端 (EMQX Broker)
    *   **SNTP**: 获取网络时间实现自动校时
    *   **AT指令驱动**: 封装 ESP8266 AT 指令集实现联网

### 开发工具
*   **IDE**: STM32CubeIDE
*   **HAL库**: STM32F4xx HAL Driver

## ✨ 功能特性

*   **丝滑 UI**: 带有惯性回弹的光标动画和列表滚动效果，支持动态 FPS 显示。
*   **网络时钟**: WiFi 自动连接，SNTP 校时，RTC 实时显示。
*   **物联网集成**: 支持 MQTT 协议，可发布设备在线状态和数据。
*   **应用扩展**:
    *   **游戏**: 内置贪吃蛇 (Snake)、恐龙跳跃 (Dino) 游戏。
    *   **工具**: 亮度调节、自动息屏设置。
*   **低功耗设计**: 支持自动息屏和唤醒机制。



## 🚀 快速开始

1.  **硬件准备**: STM32F411 核心板, 0.96 OLED (SPI), ESP8266 (连接至 USART), EC11 编码器。
2.  **编译**: 使用 STM32CubeIDE 打开 `.cproject`，编译项目。
3.  **烧录**: 使用 ST-Link 或 DAP-Link 下载固件。
4.  **配置 WiFi**: 修改 `wifi_task.c` 中的 SSID 和密码，或通过代码中的配置模式进行设置。

## 👤 作者

*   **Harvey**
