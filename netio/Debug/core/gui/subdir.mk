################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../core/gui/font.c \
../core/gui/geometric.c 

OBJS += \
./core/gui/font.o \
./core/gui/geometric.o 

C_DEPS += \
./core/gui/font.d \
./core/gui/geometric.d 


# Each subdirectory must supply rules for building sources it contributes
core/gui/%.o: ../core/gui/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -g2 -gstabs -O0 -fpack-struct -fshort-enums -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega644p -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


