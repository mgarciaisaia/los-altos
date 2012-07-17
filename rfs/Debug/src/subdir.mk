################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/administracion.c \
../src/funciones_rfs.c \
../src/main_rfs.c 

OBJS += \
./src/administracion.o \
./src/funciones_rfs.o \
./src/main_rfs.o 

C_DEPS += \
./src/administracion.d \
./src/funciones_rfs.d \
./src/main_rfs.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"../../commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" -pthread
	@echo 'Finished building: $<'
	@echo ' '


