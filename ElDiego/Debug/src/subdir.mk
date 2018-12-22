################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/DMA.c \
../src/DMAFunctions.c \
../src/ServerThread.c 

OBJS += \
./src/DMA.o \
./src/DMAFunctions.o \
./src/ServerThread.o 

C_DEPS += \
./src/DMA.d \
./src/DMAFunctions.d \
./src/ServerThread.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Bibliotecas" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


