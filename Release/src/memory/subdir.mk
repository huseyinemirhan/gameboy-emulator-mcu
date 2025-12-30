################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/memory/memory.c 

C_DEPS += \
./src/memory/memory.d 

OBJS += \
./src/memory/memory.o 


# Each subdirectory must supply rules for building sources it contributes
src/memory/%.o: ../src/memory/%.c src/memory/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src-2f-memory

clean-src-2f-memory:
	-$(RM) ./src/memory/memory.d ./src/memory/memory.o

.PHONY: clean-src-2f-memory

