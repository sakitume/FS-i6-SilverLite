################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables
C_SRCS += \
../source/adc.c \
../source/backlight.c \
../source/console.c \
../source/debug.c \
../source/font.c \
../source/lcd.c \
../source/main.c \
../source/screen.c \
../source/drv_time.c \
../source/uart.c

OBJS += \
./source/adc.o \
./source/backlight.o \
./source/console.o \
./source/debug.o \
./source/font.o \
./source/lcd.o \
./source/main.o \
./source/screen.o \
./source/drv_time.o \
./source/uart.o

C_DEPS += \
./source/adc.d \
./source/backlight.d \
./source/console.d \
./source/debug.d \
./source/font.d \
./source/lcd.d \
./source/main.d \
./source/screen.d \
./source/drv_time.d \
./source/uart.d

# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -Os $(PARAMS) -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Werror=implicit-function-declaration -g -D"CPU_MKL16Z64VLH4" -I../startup -I../board -I../utilities -I../CMSIS -I../drivers -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/%.o: ../source/%.asm
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall  -g -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
