################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Game_Dino.c \
../Core/Src/Game_Dino_Data.c \
../Core/Src/Game_Snake.c \
../Core/Src/MENU.c \
../Core/Src/OLED.c \
../Core/Src/OLED_Data.c \
../Core/Src/TIM_EC11.c \
../Core/Src/esp_at.c \
../Core/Src/esp_driver.c \
../Core/Src/freertos.c \
../Core/Src/gpio.c \
../Core/Src/input.c \
../Core/Src/main.c \
../Core/Src/ringbuffer.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_hal_timebase_tim.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/tim.c \
../Core/Src/time_task.c \
../Core/Src/usart.c \
../Core/Src/weather.c \
../Core/Src/wifi_task.c 

OBJS += \
./Core/Src/Game_Dino.o \
./Core/Src/Game_Dino_Data.o \
./Core/Src/Game_Snake.o \
./Core/Src/MENU.o \
./Core/Src/OLED.o \
./Core/Src/OLED_Data.o \
./Core/Src/TIM_EC11.o \
./Core/Src/esp_at.o \
./Core/Src/esp_driver.o \
./Core/Src/freertos.o \
./Core/Src/gpio.o \
./Core/Src/input.o \
./Core/Src/main.o \
./Core/Src/ringbuffer.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_hal_timebase_tim.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/tim.o \
./Core/Src/time_task.o \
./Core/Src/usart.o \
./Core/Src/weather.o \
./Core/Src/wifi_task.o 

C_DEPS += \
./Core/Src/Game_Dino.d \
./Core/Src/Game_Dino_Data.d \
./Core/Src/Game_Snake.d \
./Core/Src/MENU.d \
./Core/Src/OLED.d \
./Core/Src/OLED_Data.d \
./Core/Src/TIM_EC11.d \
./Core/Src/esp_at.d \
./Core/Src/esp_driver.d \
./Core/Src/freertos.d \
./Core/Src/gpio.d \
./Core/Src/input.d \
./Core/Src/main.d \
./Core/Src/ringbuffer.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_hal_timebase_tim.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/tim.d \
./Core/Src/time_task.d \
./Core/Src/usart.d \
./Core/Src/weather.d \
./Core/Src/wifi_task.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -IC:/Users/10637/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/STM32F4xx_HAL_Driver/Inc -IC:/Users/10637/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -IC:/Users/10637/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/CMSIS/Device/ST/STM32F4xx/Include -IC:/Users/10637/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/CMSIS/Include -IC:/Users/10637/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Middlewares/Third_Party/FreeRTOS/Source/include -IC:/Users/10637/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -IC:/Users/10637/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/Game_Dino.cyclo ./Core/Src/Game_Dino.d ./Core/Src/Game_Dino.o ./Core/Src/Game_Dino.su ./Core/Src/Game_Dino_Data.cyclo ./Core/Src/Game_Dino_Data.d ./Core/Src/Game_Dino_Data.o ./Core/Src/Game_Dino_Data.su ./Core/Src/Game_Snake.cyclo ./Core/Src/Game_Snake.d ./Core/Src/Game_Snake.o ./Core/Src/Game_Snake.su ./Core/Src/MENU.cyclo ./Core/Src/MENU.d ./Core/Src/MENU.o ./Core/Src/MENU.su ./Core/Src/OLED.cyclo ./Core/Src/OLED.d ./Core/Src/OLED.o ./Core/Src/OLED.su ./Core/Src/OLED_Data.cyclo ./Core/Src/OLED_Data.d ./Core/Src/OLED_Data.o ./Core/Src/OLED_Data.su ./Core/Src/TIM_EC11.cyclo ./Core/Src/TIM_EC11.d ./Core/Src/TIM_EC11.o ./Core/Src/TIM_EC11.su ./Core/Src/esp_at.cyclo ./Core/Src/esp_at.d ./Core/Src/esp_at.o ./Core/Src/esp_at.su ./Core/Src/esp_driver.cyclo ./Core/Src/esp_driver.d ./Core/Src/esp_driver.o ./Core/Src/esp_driver.su ./Core/Src/freertos.cyclo ./Core/Src/freertos.d ./Core/Src/freertos.o ./Core/Src/freertos.su ./Core/Src/gpio.cyclo ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/input.cyclo ./Core/Src/input.d ./Core/Src/input.o ./Core/Src/input.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/ringbuffer.cyclo ./Core/Src/ringbuffer.d ./Core/Src/ringbuffer.o ./Core/Src/ringbuffer.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_hal_timebase_tim.cyclo ./Core/Src/stm32f4xx_hal_timebase_tim.d ./Core/Src/stm32f4xx_hal_timebase_tim.o ./Core/Src/stm32f4xx_hal_timebase_tim.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/tim.cyclo ./Core/Src/tim.d ./Core/Src/tim.o ./Core/Src/tim.su ./Core/Src/time_task.cyclo ./Core/Src/time_task.d ./Core/Src/time_task.o ./Core/Src/time_task.su ./Core/Src/usart.cyclo ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su ./Core/Src/weather.cyclo ./Core/Src/weather.d ./Core/Src/weather.o ./Core/Src/weather.su ./Core/Src/wifi_task.cyclo ./Core/Src/wifi_task.d ./Core/Src/wifi_task.o ./Core/Src/wifi_task.su

.PHONY: clean-Core-2f-Src

