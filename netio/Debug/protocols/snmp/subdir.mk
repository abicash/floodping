################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../protocols/snmp/snmp.c \
../protocols/snmp/snmp_net.c 

OBJS += \
./protocols/snmp/snmp.o \
./protocols/snmp/snmp_net.o 

C_DEPS += \
./protocols/snmp/snmp.d \
./protocols/snmp/snmp_net.d 


# Each subdirectory must supply rules for building sources it contributes
protocols/snmp/%.o: ../protocols/snmp/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -g2 -gstabs -O0 -fpack-struct -fshort-enums -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega644p -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


