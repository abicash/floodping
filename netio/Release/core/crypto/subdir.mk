################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../core/crypto/cast5.c \
../core/crypto/md5.c 

S_UPPER_SRCS += \
../core/crypto/sha1-asm.S 

OBJS += \
./core/crypto/cast5.o \
./core/crypto/md5.o \
./core/crypto/sha1-asm.o 

C_DEPS += \
./core/crypto/cast5.d \
./core/crypto/md5.d 

S_UPPER_DEPS += \
./core/crypto/sha1-asm.d 


# Each subdirectory must supply rules for building sources it contributes
core/crypto/%.o: ../core/crypto/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -I"/root/workspace1/netio" -I"/root/workspace1/netio/core/portio" -Wall -Os -fpack-struct -fshort-enums -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega644p -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

core/crypto/%.o: ../core/crypto/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Assembler'
	avr-gcc -x assembler-with-cpp -mmcu=atmega644p -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


