################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/Adafruit_BNO055.c \
../src/arduino_rangefinder.c \
../src/gps_module.c \
../src/haptic_module.c \
../src/main.c \
../src/uart_v2.c 

OBJS += \
./src/Adafruit_BNO055.o \
./src/arduino_rangefinder.o \
./src/gps_module.o \
./src/haptic_module.o \
./src/main.o \
./src/uart_v2.o 

C_DEPS += \
./src/Adafruit_BNO055.d \
./src/arduino_rangefinder.d \
./src/gps_module.d \
./src/haptic_module.d \
./src/main.d \
./src/uart_v2.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MicroBlaze gcc compiler'
	mb-gcc -Wall -O1 -g3 -c -fmessage-length=0 -MT"$@" -I../../bsp_iop_arduino/iop_arduino_mb/include -mlittle-endian -mcpu=v9.5 -mxl-soft-mul -Wl,--no-relax -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


