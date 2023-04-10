################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../app/gnss.c \
../app/init.c \
../app/port.c \
../app/radio.c 

OBJS += \
./app/gnss.o \
./app/init.o \
./app/port.o \
./app/radio.o 

C_DEPS += \
./app/gnss.d \
./app/init.d \
./app/port.d \
./app/radio.d 


# Each subdirectory must supply rules for building sources it contributes
app/%.o app/%.su: ../app/%.c app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DSTM32L010x6 -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/CMSIS/Include -I../app -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM0 -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-app

clean-app:
	-$(RM) ./app/gnss.d ./app/gnss.o ./app/gnss.su ./app/init.d ./app/init.o ./app/init.su ./app/port.d ./app/port.o ./app/port.su ./app/radio.d ./app/radio.o ./app/radio.su

.PHONY: clean-app

