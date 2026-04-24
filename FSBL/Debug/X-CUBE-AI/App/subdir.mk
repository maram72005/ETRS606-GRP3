################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../X-CUBE-AI/App/meteo.c \
../X-CUBE-AI/App/meteo_data.c \
../X-CUBE-AI/App/meteo_data_params.c 

OBJS += \
./X-CUBE-AI/App/meteo.o \
./X-CUBE-AI/App/meteo_data.o \
./X-CUBE-AI/App/meteo_data_params.o 

C_DEPS += \
./X-CUBE-AI/App/meteo.d \
./X-CUBE-AI/App/meteo_data.d \
./X-CUBE-AI/App/meteo_data_params.d 


# Each subdirectory must supply rules for building sources it contributes
X-CUBE-AI/App/%.o X-CUBE-AI/App/%.su X-CUBE-AI/App/%.cyclo: ../X-CUBE-AI/App/%.c X-CUBE-AI/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32N657xx -DUSE_NUCLEO_64 -c -I../Core/Inc -I../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/BSP/STM32N6xx_Nucleo -I../X-CUBE-AI/App -I../../Middlewares/ST/AI/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-X-2d-CUBE-2d-AI-2f-App

clean-X-2d-CUBE-2d-AI-2f-App:
	-$(RM) ./X-CUBE-AI/App/meteo.cyclo ./X-CUBE-AI/App/meteo.d ./X-CUBE-AI/App/meteo.o ./X-CUBE-AI/App/meteo.su ./X-CUBE-AI/App/meteo_data.cyclo ./X-CUBE-AI/App/meteo_data.d ./X-CUBE-AI/App/meteo_data.o ./X-CUBE-AI/App/meteo_data.su ./X-CUBE-AI/App/meteo_data_params.cyclo ./X-CUBE-AI/App/meteo_data_params.d ./X-CUBE-AI/App/meteo_data_params.o ./X-CUBE-AI/App/meteo_data_params.su

.PHONY: clean-X-2d-CUBE-2d-AI-2f-App

