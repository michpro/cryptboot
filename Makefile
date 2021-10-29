# Makefile for tinyAVR 0-, 1- and 2-series, and megaAVR 0-series version of CryptBoot bootloader
#
#	@File:      Makefile
#	@Author:    Michal Protasowicki
# 	@Copyright: SPDX-FileCopyrightText: Copyright 2021 by Michal Protasowicki
#
#	@license SPDX-License-Identifier: MIT
#

PROGRAM := cryptboot_x

ifneq ($(DOWNGRADE),)
DOWNGRADE_ALLOWED = -DDOWNGRADE_ALLOWED
else
DOWNGRADE_ALLOWED = 
endif

ifneq ($(TARGET),)
MCU_TARGET = $(TARGET)
else
MCU_TARGET = attiny1604
endif

ifneq ($(TARGET_NAME),)
TARGET = $(TARGET_NAME)
else
TARGET = $(MCU_TARGET)
endif

ifneq ($(TOOL),)
GCCROOT = $(TOOL)
else
GCCROOT = ..\..\packages\DxCore\tools\avr-gcc\7.3.0-atmel3.6.1-azduino4b\bin
endif

QUOTE := "
CC          = $(QUOTE)$(GCCROOT)\avr-gcc$(QUOTE)
OBJCOPY     = $(QUOTE)$(GCCROOT)\avr-objcopy$(QUOTE)
OBJDUMP     = $(QUOTE)$(GCCROOT)\avr-objdump$(QUOTE)
PROGSIZE    = $(QUOTE)$(GCCROOT)\avr-size$(QUOTE)

RM := rm -rf

BUILD_DIR := ./build
SRC_DIR := ./src

OPTIMIZE = -Os -fno-split-wide-types -mrelax -fpack-struct -fshort-enums
OPTIONS := -x c -funsigned-char -funsigned-bitfields -ffunction-sections -fdata-sections $(OPTIMIZE) $(DOWNGRADE_ALLOWED) -Wall -c -std=gnu99 -MD -MP -MF

FILES := $(PROGRAM)
OBJS :=  $(addsuffix .o, $(addprefix $(BUILD_DIR)/, $(FILES)))

OUTPUT_FILE := $(addprefix $(BUILD_DIR)/, $(PROGRAM))

$(PROGRAM).o: $(addprefix $(SRC_DIR)/, $(PROGRAM).c)
	mkdir $(BUILD_DIR:./%=%) || exit 0
	@echo ************************************************************
	@echo Building file: $<
	@echo ************************************************************
	$(CC) $(OPTIONS) "$(subst _x,_$(TARGET).d,$(OUTPUT_FILE))" -o "$(subst _x,_$(TARGET).o,$(OUTPUT_FILE))" "$<" -mmcu=$(MCU_TARGET)
	@echo ************************************************************
	@echo Finished building: $<
	@echo ************************************************************

$(PROGRAM): $(addsuffix .o, $(FILES))
	@echo ************************************************************
	@echo Building target: $(subst _x,_$(TARGET),$(PROGRAM))
	@echo ************************************************************
	$(CC) -o "$(subst _x,_$(TARGET).elf,$(OUTPUT_FILE))" $(subst _x,_$(TARGET),$(OBJS)) -nostartfiles -Wl,-Map="$(subst _x,_$(TARGET).map,$(OUTPUT_FILE))" -Wl,--start-group -Wl,-lm  -Wl,--end-group -Wl,--gc-sections -Wl,--relax -mmcu=$(MCU_TARGET)
	@echo ************************************************************
	@echo Finished building target: $(subst _x,_$(TARGET),$(PROGRAM))
	@echo ************************************************************
	$(OBJDUMP) -d -M intel -S "$(subst _x,_$(TARGET).o,$(OUTPUT_FILE))" > "$(subst _x,_$(TARGET).lst,$(OUTPUT_FILE))"
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures "$(subst _x,_$(TARGET).elf,$(OUTPUT_FILE))" "$(subst _x,_$(TARGET).hex,$(OUTPUT_FILE))"
	$(OBJCOPY) -O binary -j .text "$(subst _x,_$(TARGET).elf,$(OUTPUT_FILE))" "$(subst _x,_$(TARGET).bin,$(OUTPUT_FILE))"
	$(PROGSIZE) "$(subst _x,_$(TARGET).elf,$(OUTPUT_FILE))"
	@echo ************************************************************

.PHONY: clean all

attiny160x:
	$(MAKE) $(PROGRAM) MCU_TARGET=attiny1607 TARGET=$@

attiny161x:
	$(MAKE) $(PROGRAM) MCU_TARGET=attiny1617 TARGET=$@

attiny162x:
	$(MAKE) $(PROGRAM) MCU_TARGET=attiny1627 TARGET=$@

attiny321x:
	$(MAKE) $(PROGRAM) MCU_TARGET=attiny3217 TARGET=$@

attiny322x:
	$(MAKE) $(PROGRAM) MCU_TARGET=attiny3227 TARGET=$@

atmega480x:
	$(MAKE) $(PROGRAM) MCU_TARGET=atmega4809 TARGET=$@

atmega320x:
	$(MAKE) $(PROGRAM) MCU_TARGET=atmega3209 TARGET=$@

atmega160x:
	$(MAKE) $(PROGRAM) MCU_TARGET=atmega1609 TARGET=$@


all: clean attiny160x attiny161x attiny162x attiny321x attiny322x atmega480x atmega320x atmega160x

clean:
	-$(RM) $(BUILD_DIR)/*

	