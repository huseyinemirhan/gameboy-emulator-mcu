################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cpu/cpu.c 

C_DEPS += \
./src/cpu/cpu.d 

OBJS += \
./src/cpu/cpu.o 


# Each subdirectory must supply rules for building sources it contributes
src/cpu/%.o: ../src/cpu/%.c src/cpu/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src-2f-cpu

clean-src-2f-cpu:
	-$(RM) ./src/cpu/cpu.d ./src/cpu/cpu.o

.PHONY: clean-src-2f-cpu

