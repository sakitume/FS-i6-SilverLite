TARGET=fs-i6
EXECUTABLE=$(TARGET).elf

#-------------------------------------------------------------------------------
# GNU ARM Embedded Toolchain
#-------------------------------------------------------------------------------
CC=arm-none-eabi-gcc
CXX=arm-none-eabi-g++
LD=arm-none-eabi-ld
AR=arm-none-eabi-ar
AS=arm-none-eabi-as
CP=arm-none-eabi-objcopy
OD=arm-none-eabi-objdump
NM=arm-none-eabi-nm
SIZE=arm-none-eabi-size
A2L=arm-none-eabi-addr2line

#-------------------------------------------------------------------------------
# Working directories
#-------------------------------------------------------------------------------
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	# Careful! This folder will be destroyed when building "clean" target
	OBJECT_DIR	= debug
else
	# Careful! These folders are destroyed when building "clean" target
	OBJECT_DIR	= release
endif

BOARD_DIR	 	= board
SDK_DIR			= SDK
SRC_DIR		 	= source

#-------------------------------------------------------------------------------
# Source Files
#-------------------------------------------------------------------------------
SOURCES =  \
	${SDK_DIR}/devices/MKL16Z4/gcc/startup_MKL16Z4.S \
	${SDK_DIR}/devices/MKL16Z4/utilities/fsl_debug_console.c \
	${SDK_DIR}/devices/MKL16Z4/utilities/fsl_debug_console.h \
	${SDK_DIR}/devices/MKL16Z4/system_MKL16Z4.c \
	${SDK_DIR}/devices/MKL16Z4/system_MKL16Z4.h \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_adc16.h \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_adc16.c \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_clock.h \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_clock.c \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_common.h \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_common.c \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_lpsci.h \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_lpsci.c \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_uart.h \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_uart.c \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_flash.c \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_flash.h \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_gpio.h \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_gpio.c \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_pit.h \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_pit.c \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_smc.h \
	${SDK_DIR}/devices/MKL16Z4/drivers/fsl_smc.c \
	${BOARD_DIR}/board.c \
	${BOARD_DIR}/board.h \
	${BOARD_DIR}/clock_config.c \
	${BOARD_DIR}/clock_config.h \
	${BOARD_DIR}/pin_mux.c \
	${BOARD_DIR}/pin_mux.h \
	${SRC_DIR}/adc.cpp \
	${SRC_DIR}/adc.h \
	${SRC_DIR}/backlight.c \
	${SRC_DIR}/backlight.h \
	${SRC_DIR}/buttons.c \
	${SRC_DIR}/buttons.h \
	${SRC_DIR}/console.c \
	${SRC_DIR}/console.h \
	${SRC_DIR}/debug.c \
	${SRC_DIR}/debug.h \
	${SRC_DIR}/drv_time.c \
	${SRC_DIR}/drv_time.h \
	${SRC_DIR}/flash.cpp \
	${SRC_DIR}/flash.h \
	${SRC_DIR}/font.c \
	${SRC_DIR}/font.h \
	${SRC_DIR}/lcd.c \
	${SRC_DIR}/lcd.h \
	${SRC_DIR}/main.cpp \
	${SRC_DIR}/screen.c \
	${SRC_DIR}/screen.h \
	${SRC_DIR}/storage.cpp \
	${SRC_DIR}/storage.h \
	${SRC_DIR}/uart.c \
	${SRC_DIR}/uart.h \
	
#-------------------------------------------------------------------------------
# Include directories
#-------------------------------------------------------------------------------
INCLUDE_DIRS = \
	$(SDK_DIR)/CMSIS/Include \
	$(SDK_DIR)/devices/MKL16Z4 \
	$(SDK_DIR)/devices/MKL16Z4/drivers \
	$(SDK_DIR)/devices/MKL16Z4/utilities \
	$(BOARD_DIR)

#-------------------------------------------------------------------------------
# Object List
#-------------------------------------------------------------------------------
OBJECTS=$(addsuffix .o,$(addprefix $(OBJECT_DIR)/,$(basename $(SOURCES))))

DEPS=$(addsuffix .d,$(addprefix $(OBJECT_DIR)/,$(basename $(SOURCES))))

#-------------------------------------------------------------------------------
# Target Output Files
#-------------------------------------------------------------------------------
TARGET_ELF=$(OBJECT_DIR)/$(TARGET).elf
TARGET_HEX=$(OBJECT_DIR)/$(TARGET).hex


#-------------------------------------------------------------------------------
# Flags
#-------------------------------------------------------------------------------
ifeq ($(DEBUG), 1)
	# Debug
	OPTIMIZE = -g -O0
else
	# Release
	OPTIMIZE = -Os
endif


COMMON_FLAGS=-Wall -mfloat-abi=soft -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mapcs

MCFLAGS= -mcpu=cortex-m0plus -mthumb
DEFS=-DCPU_MKL16Z64VLH4

CFLAGS=  -c $(MCFLAGS) $(DEFS) $(OPTIMIZE) $(addprefix -I,$(INCLUDE_DIRS)) $(COMMON_FLAGS) -MMD -MP -std=gnu99
CXXFLAGS=-c $(MCFLAGS) $(DEFS) $(OPTIMIZE) $(addprefix -I,$(INCLUDE_DIRS)) $(COMMON_FLAGS) -MMD -MP -std=c++11
CXXFLAGS+=-U__STRICT_ANSI__ -fno-rtti -fno-exceptions

ASMFLAGS=-D__STARTUP_CLEAR_BSS $(MCFLAGS) $(COMMON_FLAGS) -std=gnu99

LDFLAGS=${MCFLAGS} ${COMMON_FLAGS} --specs=nano.specs --specs=nosys.specs -Xlinker --gc-sections -Xlinker -static -Xlinker -z -Xlinker muldefs
LDFLAGS+=-T${SDK_DIR}/devices/MKL16Z4/gcc/MKL16Z64xxx4_flash.ld -static


ifeq ($(DEBUG), 1)
	# Debug
	ASMFLAGS+= -DDEBUG -g
	CFLAGS+= -DDEBUG
	CXXFLAGS+= -DDEBUG
else
	# Release
	ASMFLAGS+= -DNDEBUG
	CFLAGS+= -DNDEBUG
	CXXFLAGS+= -DNDEBUG
endif

#-------------------------------------------------------------------------------
# Build
#
#	$@ The name of the target file (the one before the colon)
#	$< The name of the first (or only) prerequisite file (the first one after the colon)
#	$^ The names of all the prerequisite files (space separated)
#	$* The stem (the bit which matches the % wildcard in the rule definition.
#
#-------------------------------------------------------------------------------
#$(TARGET_HEX): $(TARGET_ELF)
#	$(CP) -O ihex --set-start 0x8000000 $< $@

$(TARGET_ELF): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	$(SIZE) $(TARGET_ELF)

$(OBJECT_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo %% $(notdir $<)
	@$(CXX) -c -o $@ $(CXXFLAGS) $<

$(OBJECT_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo %% $(notdir $<)
	@$(CC) -c -o $@ $(CFLAGS) $<

$(OBJECT_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo %% $(notdir $<)
	@$(CC) -c -o $@ $(ASMFLAGS) $<

$(OBJECT_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@echo %% $(notdir $<)
	@$(CC) -c -o $@ $(ASMFLAGS) $<

#-------------------------------------------------------------------------------
# Recipes
#-------------------------------------------------------------------------------
.PHONY: all flash clean

clean:
	rm -rf $(OBJECTS) $(DEPS) $(TARGET_ELF) $(TARGET_HEX) $(OBJECT_DIR)/output.map
#clean:
#	rm -rf $(OBJECT_DIR)

flash: $(TARGET_ELF)
	openocd -f interface/stlink.cfg -f target/klx.cfg -c "program $(TARGET_ELF) verify reset exit"

#all: $(TARGET_HEX)

all: $(TARGET_ELF)
