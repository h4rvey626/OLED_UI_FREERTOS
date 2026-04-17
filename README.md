# OLED_UI_FREERTOS 智能手环

基于 STM32F411 和 FreeRTOS 构建的智能手环项目，集成了 OLED UI 菜单系统、WiFi 联网、MQTT 通信和网络时间同步等功能。项目实现了丝滑的 UI 动画效果和模块化的应用框架。

## 📸 项目概览

*   **核心平台**: STM32F411CEU6 (BlackPill)
*   **操作系统**: FreeRTOS
*   **显示**: 0.96寸 OLED (SSD1306, I2C)
*   **连接**: ESP8266 WiFi 模块 (AT指令集)
*   **交互**: 旋转编码器 (EC11)

## 🛠 技术栈

### 硬件层 (Hardware Layer)
*   **MCU**: STM32F411CEU6 (Cortex-M4F, 100MHz)
*   **Display**: SSD1306 OLED (I2C 驱动)
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

## ⚖️ 优缺点分析

### ✅ 优点 (Pros)
*   **架构清晰**: 典型的 RTOS 多任务架构，UI、网络、输入处理分离，易于扩展和维护。
*   **UI 体验优秀**: 实现了在资源受限的 MCU 上的平滑动画效果 (缓动函数)，交互体验接近现代设备。
*   **模块化程度高**: 菜单系统 (MENU.c) 与底层硬件解耦，便于移植到其他平台。
*   **实用性强**: 结合了 WiFi 和 MQTT，具备了物联网设备的基本雏形。

### ⚠️ 缺点 (Cons)
*   **内存限制**: STM32F411 RAM (128KB) 有限，复杂的 UI 缓冲和网络栈可能导致堆栈溢出风险 (需仔细调整 Stack Size)。
*   **ESP8266 AT 效率**: 使用 AT 指令进行网络通信相比原生 TCP/IP 栈 (如 LWIP) 效率较低，吞吐量受限于 UART 波特率。
*   **UI 扩展性**: 目前主要针对 128x64 单色屏优化，适配彩色屏或更高分辨率需要重写渲染层。

## 🚀 快速开始

1.  **硬件准备**: STM32F411 核心板, 0.96 OLED (I2C), ESP8266 (连接至 USART), EC11 编码器。
2.  **编译**: 使用 STM32CubeIDE 打开 `.cproject`，编译项目。
3.  **烧录**: 使用 ST-Link 或 DAP-Link 下载固件。
4.  **配置 WiFi**: 修改 `wifi_task.c` 中的 SSID 和密码，或通过代码中的配置模式进行设置。

## 👤 作者

*   **Harvey**
