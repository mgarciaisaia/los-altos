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
all: rfs

dependents:
	-cd /home/desert69/workspaces/operativos/2012-1c-los-altos/commons/Debug && $(MAKE) all

# Tool invocations
rfs: $(OBJS) $(USER_OBJS) /home/desert69/workspaces/operativos/2012-1c-los-altos/commons/Debug/libcommons.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/desert69/workspaces/operativos/2012-1c-los-altos/commons/Debug" -o "rfs" $(OBJS) $(USER_OBJS) $(LIBS) -pthread
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) rfs
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
/home/desert69/workspaces/operativos/2012-1c-los-altos/commons/Debug/libcommons.so:

-include ../makefile.targets
