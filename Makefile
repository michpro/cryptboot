################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL := cmd.exe
RM := rm -rf

USER_OBJS :=

LIBS := 
PROJ := 

O_SRCS := 
C_SRCS := 
S_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
PREPROCESSING_SRCS := 
OBJS := 
OBJS_AS_ARGS := 
C_DEPS := 
C_DEPS_AS_ARGS := 
EXECUTABLES := 
OUTPUT_FILE_PATH :=
OUTPUT_FILE_PATH_AS_ARGS :=
AVR_APP_PATH :=$$$AVR_APP_PATH$$$
QUOTE := "
ADDITIONAL_DEPENDENCIES:=
OUTPUT_FILE_DEP:=
LIB_DEP:=
LINKER_SCRIPT_DEP:=

# Every subdirectory with source files must be described here
SUBDIRS := 


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
cryptboot.c
#src/xtea.c
#../i2c_slave.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
cryptboot.o
#src/xtea.c
#i2c_slave.o

OBJS_AS_ARGS +=  \
cryptboot.o
#src/xtea.c
#i2c_slave.o

C_DEPS +=  \
cryptboot.d
#src/xtea.c
#i2c_slave.d

C_DEPS_AS_ARGS +=  \
cryptboot.d
#src/xtea.c
#i2c_slave.d

OUTPUT_FILE_PATH +=cryptboot.elf

OUTPUT_FILE_PATH_AS_ARGS +=cryptboot.elf

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

LIB_DEP+= 

LINKER_SCRIPT_DEP+= 


# AVR32/GNU C Compiler
./cryptboot.o: ./cryptboot.c
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler : 5.4.0
	$(QUOTE)d:\Work\arduino\portable\packages\DxCore\tools\avr-gcc\7.3.0-atmel3.6.1-azduino4b\bin\avr-gcc.exe$(QUOTE) -x c -funsigned-char -funsigned-bitfields -DNDEBUG -Os -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -Wall -mmcu=attiny817 -c -std=gnu99 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -o "$@" "$<" 
	@echo Finished building: $<
	

#./i2c_slave.o: .././i2c_slave.c
#	@echo Building file: $<
#	@echo Invoking: AVR/GNU C Compiler : 5.4.0
#	$(QUOTE)C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-gcc.exe$(QUOTE)  -x c -funsigned-char -funsigned-bitfields -DNDEBUG  -I"C:\Program Files (x86)\Atmel\Studio\7.0\Packs\Atmel\ATtiny_DFP\1.3.229\include"  -Os -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -Wall -mmcu=attiny817 -B "C:\Program Files (x86)\Atmel\Studio\7.0\Packs\Atmel\ATtiny_DFP\1.3.229\gcc\dev\attiny817" -c -std=gnu99 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
#	@echo Finished building: $<
	

# AVR32/GNU Preprocessing Assembler

# AVR32/GNU Assembler

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(OUTPUT_FILE_PATH) $(ADDITIONAL_DEPENDENCIES)

$(OUTPUT_FILE_PATH): $(OBJS) $(USER_OBJS) $(OUTPUT_FILE_DEP) $(LIB_DEP) $(LINKER_SCRIPT_DEP)
	@echo Building target: $@
	@echo Invoking: AVR/GNU Linker : 5.4.0
	$(QUOTE)d:\Work\arduino\portable\packages\DxCore\tools\avr-gcc\7.3.0-atmel3.6.1-azduino4b\bin\avr-gcc.exe$(QUOTE) -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -nostartfiles -Wl,-Map="cryptboot.map" -Wl,--start-group -Wl,-lm  -Wl,--end-group -Wl,--gc-sections -mmcu=attiny817  
	@echo Finished building target: $@
	"d:\Work\arduino\portable\packages\DxCore\tools\avr-gcc\7.3.0-atmel3.6.1-azduino4b\bin\avr-objcopy.exe" -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures  "cryptboot.elf" "cryptboot.hex"
	"d:\Work\arduino\portable\packages\DxCore\tools\avr-gcc\7.3.0-atmel3.6.1-azduino4b\bin\avr-objcopy.exe" -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex "cryptboot.elf" "cryptboot.eep" || exit 0
	"d:\Work\arduino\portable\packages\DxCore\tools\avr-gcc\7.3.0-atmel3.6.1-azduino4b\bin\\avr-size.exe" "cryptboot.elf"

# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS) $(EXECUTABLES)  
	-$(RM) $(C_DEPS_AS_ARGS)   
	rm -rf "cryptboot.elf" "cryptboot.a" "cryptboot.hex" "cryptboot.lss" "cryptboot.eep" "cryptboot.map" "cryptboot.srec" "cryptboot.usersignatures"
	