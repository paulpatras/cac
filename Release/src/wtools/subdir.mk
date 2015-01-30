################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../src/wtools/iwlib.o 

C_SRCS += \
../src/wtools/iwlib.c 

OBJS += \
./src/wtools/iwlib.o 

C_DEPS += \
./src/wtools/iwlib.d 


# Each subdirectory must supply rules for building sources it contributes
src/wtools/%.o: ../src/wtools/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


