################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/hts221_polling.c \
../Core/Src/hts221_reg.c \
../Core/Src/lps22hh_reg.c \
../Core/Src/main.c \
../Core/Src/pressure_polling.c \
../Core/Src/stm32n6xx_hal_msp.c \
../Core/Src/stm32n6xx_it.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32n6xx_fsbl.c 

OBJS += \
./Core/Src/hts221_polling.o \
./Core/Src/hts221_reg.o \
./Core/Src/lps22hh_reg.o \
./Core/Src/main.o \
./Core/Src/pressure_polling.o \
./Core/Src/stm32n6xx_hal_msp.o \
./Core/Src/stm32n6xx_it.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32n6xx_fsbl.o 

C_DEPS += \
./Core/Src/hts221_polling.d \
./Core/Src/hts221_reg.d \
./Core/Src/lps22hh_reg.d \
./Core/Src/main.d \
./Core/Src/pressure_polling.d \
./Core/Src/stm32n6xx_hal_msp.d \
./Core/Src/stm32n6xx_it.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32n6xx_fsbl.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32N657xx -DUSE_NUCLEO_64 -c -I../Core/Inc -I../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/BSP/STM32N6xx_Nucleo -I../X-CUBE-AI/App -I../../Middlewares/ST/AI/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/hts221_polling.cyclo ./Core/Src/hts221_polling.d ./Core/Src/hts221_polling.o ./Core/Src/hts221_polling.su ./Core/Src/hts221_reg.cyclo ./Core/Src/hts221_reg.d ./Core/Src/hts221_reg.o ./Core/Src/hts221_reg.su ./Core/Src/lps22hh_reg.cyclo ./Core/Src/lps22hh_reg.d ./Core/Src/lps22hh_reg.o ./Core/Src/lps22hh_reg.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/pressure_polling.cyclo ./Core/Src/pressure_polling.d ./Core/Src/pressure_polling.o ./Core/Src/pressure_polling.su ./Core/Src/stm32n6xx_hal_msp.cyclo ./Core/Src/stm32n6xx_hal_msp.d ./Core/Src/stm32n6xx_hal_msp.o ./Core/Src/stm32n6xx_hal_msp.su ./Core/Src/stm32n6xx_it.cyclo ./Core/Src/stm32n6xx_it.d ./Core/Src/stm32n6xx_it.o ./Core/Src/stm32n6xx_it.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32n6xx_fsbl.cyclo ./Core/Src/system_stm32n6xx_fsbl.d ./Core/Src/system_stm32n6xx_fsbl.o ./Core/Src/system_stm32n6xx_fsbl.su

.PHONY: clean-Core-2f-Src

