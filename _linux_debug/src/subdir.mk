################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cFep.c \
../src/dictionary.c \
../src/ini.c \
../src/iniparser.c \
../src/lib.c \
../src/listLib.c \
../src/log.c \
../src/ptcl_62056-47.c \
../src/ptcl_698.c \
../src/ptcl_gw.c \
../src/ptcl_jl.c \
../src/ptcl_nw.c \
../src/ptcl_zj.c \
../src/socket.c \
../src/taskLib.c \
../src/ttynet.c 

OBJS += \
./src/cFep.o \
./src/dictionary.o \
./src/ini.o \
./src/iniparser.o \
./src/lib.o \
./src/listLib.o \
./src/log.o \
./src/ptcl_62056-47.o \
./src/ptcl_698.o \
./src/ptcl_gw.o \
./src/ptcl_jl.o \
./src/ptcl_nw.o \
./src/ptcl_zj.o \
./src/socket.o \
./src/taskLib.o \
./src/ttynet.o 

C_DEPS += \
./src/cFep.d \
./src/dictionary.d \
./src/ini.d \
./src/iniparser.d \
./src/lib.d \
./src/listLib.d \
./src/log.d \
./src/ptcl_62056-47.d \
./src/ptcl_698.d \
./src/ptcl_gw.d \
./src/ptcl_jl.d \
./src/ptcl_nw.d \
./src/ptcl_zj.d \
./src/socket.d \
./src/taskLib.d \
./src/ttynet.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -fsigned-char -ffunction-sections -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


