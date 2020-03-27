################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/lib/zip/CrypFun.c \
../src/lib/zip/cceman.c \
../src/lib/zip/compressfun.c \
../src/lib/zip/compressfunnew.c 

OBJS += \
./src/lib/zip/CrypFun.o \
./src/lib/zip/cceman.o \
./src/lib/zip/compressfun.o \
./src/lib/zip/compressfunnew.o 

C_DEPS += \
./src/lib/zip/CrypFun.d \
./src/lib/zip/cceman.d \
./src/lib/zip/compressfun.d \
./src/lib/zip/compressfunnew.d 


# Each subdirectory must supply rules for building sources it contributes
src/lib/zip/%.o: ../src/lib/zip/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -fsigned-char -ffunction-sections -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


