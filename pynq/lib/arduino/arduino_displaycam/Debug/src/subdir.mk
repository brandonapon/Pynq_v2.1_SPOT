################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/arduino_displaycam.c \
../src/spi_v2.c \
../src/uart_v2.c 

OBJS += \
./src/arduino_displaycam.o \
./src/spi_v2.o \
./src/uart_v2.o 

C_DEPS += \
./src/arduino_displaycam.d \
./src/spi_v2.d \
./src/uart_v2.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MicroBlaze gcc compiler'
	mb-gcc -Wall -O0 -g3 -I"D:\Pynq\PYNQ_v2.1_SPOT\pynq\lib\arduino\bsp_iop_arduino\iop_arduino_mb\include" -c -fmessage-length=0 -MT"$@" -mlittle-endian -mcpu=v9.6 -mxl-soft-mul -Wl,--no-relax -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

