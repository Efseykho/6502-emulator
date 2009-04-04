################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../6502/em_6502.c \
../6502/harness.c \
../6502/unit_test.c 

OBJS += \
./6502/em_6502.o \
./6502/harness.o \
./6502/unit_test.o 

C_DEPS += \
./6502/em_6502.d \
./6502/harness.d \
./6502/unit_test.d 


# Each subdirectory must supply rules for building sources it contributes
6502/%.o: ../6502/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


