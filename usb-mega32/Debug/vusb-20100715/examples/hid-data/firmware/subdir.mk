################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vusb-20100715/examples/hid-data/firmware/main.c 

OBJS += \
./vusb-20100715/examples/hid-data/firmware/main.o 

C_DEPS += \
./vusb-20100715/examples/hid-data/firmware/main.d 


# Each subdirectory must supply rules for building sources it contributes
vusb-20100715/examples/hid-data/firmware/%.o: ../vusb-20100715/examples/hid-data/firmware/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -g2 -gstabs -O0 -fpack-struct -fshort-enums -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega32 -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

