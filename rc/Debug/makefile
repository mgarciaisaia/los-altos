################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: librc.so

dependents:
	-cd ../../commons/Debug && $(MAKE) clean all

# Tool invocations
librc.so: $(OBJS) $(USER_OBJS) ../../commons/Debug/libcommons.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"../../commons/Debug" -shared -o "librc.so" $(OBJS) $(USER_OBJS) $(LIBS)
	cp -u ../rc.conf ../../memcached-1.6/.libs/
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(LIBRARIES) librc.so ../../memcached-1.6/.libs/rc.conf
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
../../commons/Debug/libcommons.so:

-include ../makefile.targets
