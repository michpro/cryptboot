PROGRAM := cryptboot

ifneq ($(TARGET),)
MCU_TARGET = $(TARGET)
else
MCU_TARGET = attiny1604
endif

ifneq ($(TOOL),)
GCCROOT = $(TOOL)
else
GCCROOT = ..\..\packages\DxCore\tools\avr-gcc\7.3.0-atmel3.6.1-azduino4b\bin
endif

QUOTE := "
CC          = $(QUOTE)$(GCCROOT)\avr-gcc$(QUOTE)
OBJCOPY     = $(QUOTE)$(GCCROOT)\avr-objcopy$(QUOTE)
PROGSIZE    = $(QUOTE)$(GCCROOT)\avr-size$(QUOTE)

SHELL := cmd.exe
RM := rm -rf

BUILD_DIR := ./build
SRC_DIR := ./src

OPTIONS := -x c -funsigned-char -funsigned-bitfields -DNDEBUG -Os -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -Wall -c -std=gnu99 -MD -MP -MF

FILES := cryptboot twi_1 xtea
OBJS :=  $(addsuffix .o, $(addprefix $(BUILD_DIR)/, $(FILES)))

OUTPUT_FILE := $(BUILD_DIR)/$(PROGRAM)

$(BUILD_DIR)/cryptboot.o: $(SRC_DIR)/cryptboot.c
	mkdir $(BUILD_DIR:./%=%) || exit 0
	@echo Building file: $<
	$(CC) $(OPTIONS) "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -o "$@" "$<" -mmcu=$(MCU_TARGET)
	@echo Finished building: $<
	
$(BUILD_DIR)/twi_1.o: $(SRC_DIR)/twi_1.c
	@echo Building file: $<
	$(CC) $(OPTIONS) "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -o "$@" "$<" -mmcu=$(MCU_TARGET)
	@echo Finished building: $<

$(BUILD_DIR)/xtea.o: $(SRC_DIR)/xtea.c
	@echo Building file: $<
	$(CC) $(OPTIONS) "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -o "$@" "$<" -mmcu=$(MCU_TARGET)
	@echo Finished building: $<


# All Target
all: $(PROGRAM) 

$(PROGRAM): $(OBJS)
	@echo Building target: $@
	$(CC) -o$(OUTPUT_FILE).elf $(OBJS) -nostartfiles -Wl,-Map="$(OUTPUT_FILE).map" -Wl,--start-group -Wl,-lm  -Wl,--end-group -Wl,--gc-sections -mmcu=$(MCU_TARGET)  
	@echo Finished building target: $@
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures  "$(OUTPUT_FILE).elf" "$(OUTPUT_FILE).hex"
	$(OBJCOPY) -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex "$(OUTPUT_FILE).elf" "$(OUTPUT_FILE).eep" || exit 0
	$(PROGSIZE) "$(OUTPUT_FILE).elf"

.PHONY: clean
clean:
	-$(RM) $(BUILD_DIR)/*

	