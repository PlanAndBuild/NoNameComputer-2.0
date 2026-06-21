################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/flash_w25q.c \
../Core/Src/flight_computer.c \
../Core/Src/gps_parser.c \
../Core/Src/lora_llcc68.c \
../Core/Src/main.c \
../Core/Src/pca9685.c \
../Core/Src/sd_card.c \
../Core/Src/sensor_bmp390.c \
../Core/Src/sensor_h3lis200.c \
../Core/Src/sensor_ina3221.c \
../Core/Src/sensor_lsm6dso.c \
../Core/Src/sensor_lsm6dsv.c \
../Core/Src/sensor_mmc5983.c \
../Core/Src/sensor_ms5611.c \
../Core/Src/spi_common.c \
../Core/Src/stm32h5xx_hal_msp.c \
../Core/Src/stm32h5xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32h5xx.c 

OBJS += \
./Core/Src/flash_w25q.o \
./Core/Src/flight_computer.o \
./Core/Src/gps_parser.o \
./Core/Src/lora_llcc68.o \
./Core/Src/main.o \
./Core/Src/pca9685.o \
./Core/Src/sd_card.o \
./Core/Src/sensor_bmp390.o \
./Core/Src/sensor_h3lis200.o \
./Core/Src/sensor_ina3221.o \
./Core/Src/sensor_lsm6dso.o \
./Core/Src/sensor_lsm6dsv.o \
./Core/Src/sensor_mmc5983.o \
./Core/Src/sensor_ms5611.o \
./Core/Src/spi_common.o \
./Core/Src/stm32h5xx_hal_msp.o \
./Core/Src/stm32h5xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32h5xx.o 

C_DEPS += \
./Core/Src/flash_w25q.d \
./Core/Src/flight_computer.d \
./Core/Src/gps_parser.d \
./Core/Src/lora_llcc68.d \
./Core/Src/main.d \
./Core/Src/pca9685.d \
./Core/Src/sd_card.d \
./Core/Src/sensor_bmp390.d \
./Core/Src/sensor_h3lis200.d \
./Core/Src/sensor_ina3221.d \
./Core/Src/sensor_lsm6dso.d \
./Core/Src/sensor_lsm6dsv.d \
./Core/Src/sensor_mmc5983.d \
./Core/Src/sensor_ms5611.d \
./Core/Src/spi_common.d \
./Core/Src/stm32h5xx_hal_msp.d \
./Core/Src/stm32h5xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32h5xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUX_INCLUDE_USER_DEFINE_FILE -DUSE_HAL_DRIVER -DSTM32H562xx -c -I../USBX/App -I../USBX/Target -I../Core/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H5xx/Include -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/flash_w25q.cyclo ./Core/Src/flash_w25q.d ./Core/Src/flash_w25q.o ./Core/Src/flash_w25q.su ./Core/Src/flight_computer.cyclo ./Core/Src/flight_computer.d ./Core/Src/flight_computer.o ./Core/Src/flight_computer.su ./Core/Src/gps_parser.cyclo ./Core/Src/gps_parser.d ./Core/Src/gps_parser.o ./Core/Src/gps_parser.su ./Core/Src/lora_llcc68.cyclo ./Core/Src/lora_llcc68.d ./Core/Src/lora_llcc68.o ./Core/Src/lora_llcc68.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/pca9685.cyclo ./Core/Src/pca9685.d ./Core/Src/pca9685.o ./Core/Src/pca9685.su ./Core/Src/sd_card.cyclo ./Core/Src/sd_card.d ./Core/Src/sd_card.o ./Core/Src/sd_card.su ./Core/Src/sensor_bmp390.cyclo ./Core/Src/sensor_bmp390.d ./Core/Src/sensor_bmp390.o ./Core/Src/sensor_bmp390.su ./Core/Src/sensor_h3lis200.cyclo ./Core/Src/sensor_h3lis200.d ./Core/Src/sensor_h3lis200.o ./Core/Src/sensor_h3lis200.su ./Core/Src/sensor_ina3221.cyclo ./Core/Src/sensor_ina3221.d ./Core/Src/sensor_ina3221.o ./Core/Src/sensor_ina3221.su ./Core/Src/sensor_lsm6dso.cyclo ./Core/Src/sensor_lsm6dso.d ./Core/Src/sensor_lsm6dso.o ./Core/Src/sensor_lsm6dso.su ./Core/Src/sensor_lsm6dsv.cyclo ./Core/Src/sensor_lsm6dsv.d ./Core/Src/sensor_lsm6dsv.o ./Core/Src/sensor_lsm6dsv.su ./Core/Src/sensor_mmc5983.cyclo ./Core/Src/sensor_mmc5983.d ./Core/Src/sensor_mmc5983.o ./Core/Src/sensor_mmc5983.su ./Core/Src/sensor_ms5611.cyclo ./Core/Src/sensor_ms5611.d ./Core/Src/sensor_ms5611.o ./Core/Src/sensor_ms5611.su ./Core/Src/spi_common.cyclo ./Core/Src/spi_common.d ./Core/Src/spi_common.o ./Core/Src/spi_common.su ./Core/Src/stm32h5xx_hal_msp.cyclo ./Core/Src/stm32h5xx_hal_msp.d ./Core/Src/stm32h5xx_hal_msp.o ./Core/Src/stm32h5xx_hal_msp.su ./Core/Src/stm32h5xx_it.cyclo ./Core/Src/stm32h5xx_it.d ./Core/Src/stm32h5xx_it.o ./Core/Src/stm32h5xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32h5xx.cyclo ./Core/Src/system_stm32h5xx.d ./Core/Src/system_stm32h5xx.o ./Core/Src/system_stm32h5xx.su

.PHONY: clean-Core-2f-Src

