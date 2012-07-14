################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/manage.c \
../src/our_engine.c \
../src/vector.c 

OBJS += \
./src/manage.o \
./src/our_engine.o \
./src/vector.o 

C_DEPS += \
./src/manage.d \
./src/our_engine.d \
./src/vector.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspaces/2012-1c-los-altos/memcached-1.6/include" -I"/home/utnso/workspaces/2012-1c-los-altos/commons" -Ipthread -Irt -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


