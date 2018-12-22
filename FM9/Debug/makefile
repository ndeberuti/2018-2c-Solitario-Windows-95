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
all: FM9

dependents:
	-cd /home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Bibliotecas/Debug && $(MAKE) all

# Tool invocations
FM9: $(OBJS) $(USER_OBJS) /home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Bibliotecas/Debug/libBibliotecas.a
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Bibliotecas/Debug" -o "FM9" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) FM9
	-@echo ' '

.PHONY: all clean dependents
/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Bibliotecas/Debug/libBibliotecas.a:

-include ../makefile.targets
