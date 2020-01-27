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
	OBJECT_DIR	= gcc_debug
else
	# Careful! These folders are destroyed when building "clean" target
	OBJECT_DIR	= gcc_release
endif

BOARD_DIR	 	= board
SDK_DIR			= SDK
DRIVERS_DIR		= drivers
SRC_DIR		 	= source
GEM_DIR			= GEM
CMSIS_DIR		= CMSIS
GCC_DIR			= gcc
UTILITIES_DIR	= utilities

#-------------------------------------------------------------------------------
# Source Files
#-------------------------------------------------------------------------------
SOURCES =  \
	${GCC_DIR}/startup_MKL16Z4.S \
	${UTILITIES_DIR}/fsl_debug_console.c \
	${UTILITIES_DIR}/fsl_debug_console.h \
	${CMSIS_DIR}/system_MKL16Z4.c \
	${CMSIS_DIR}/system_MKL16Z4.h \
	${DRIVERS_DIR}/fsl_adc16.h \
	${DRIVERS_DIR}/fsl_adc16.c \
	${DRIVERS_DIR}/fsl_clock.h \
	${DRIVERS_DIR}/fsl_clock.c \
	${DRIVERS_DIR}/fsl_common.h \
	${DRIVERS_DIR}/fsl_common.c \
	${DRIVERS_DIR}/fsl_lpsci.h \
	${DRIVERS_DIR}/fsl_lpsci.c \
	${DRIVERS_DIR}/fsl_uart.h \
	${DRIVERS_DIR}/fsl_uart.c \
	${DRIVERS_DIR}/fsl_flash.c \
	${DRIVERS_DIR}/fsl_flash.h \
	${DRIVERS_DIR}/fsl_gpio.h \
	${DRIVERS_DIR}/fsl_gpio.c \
	${DRIVERS_DIR}/fsl_pit.h \
	${DRIVERS_DIR}/fsl_pit.c \
	${DRIVERS_DIR}/fsl_smc.h \
	${DRIVERS_DIR}/fsl_smc.c \
	${DRIVERS_DIR}/fsl_tpm.h \
	${DRIVERS_DIR}/fsl_tpm.c \
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
	${SRC_DIR}/gui_calibrate.cpp \
	${SRC_DIR}/gui_main.cpp \
	${SRC_DIR}/gui_models.cpp \
	${SRC_DIR}/gui_sliders.cpp \
	${SRC_DIR}/gui_tx.cpp \
	${SRC_DIR}/lcd.c \
	${SRC_DIR}/lcd.h \
	${SRC_DIR}/main.cpp \
	${SRC_DIR}/multiprotocol.cpp \
	${SRC_DIR}/multiprotocol.h \
	${SRC_DIR}/screen.c \
	${SRC_DIR}/screen.h \
	${SRC_DIR}/sound.cpp \
	${SRC_DIR}/sound.h \
	${SRC_DIR}/storage.cpp \
	${SRC_DIR}/storage.h \
	${SRC_DIR}/timer.cpp \
	${SRC_DIR}/timer.h \
	${SRC_DIR}/tx_interface.cpp \
	${GEM_DIR}/GEMItem.h \
	${GEM_DIR}/GEMItem.cpp \
	${GEM_DIR}/GEMSelect.cpp \
	${GEM_DIR}/GEMPage.h \
	${GEM_DIR}/GEMPage.cpp \
	${GEM_DIR}/GEM.h \
	${GEM_DIR}/GEM.cpp \
	${GEM_DIR}/FakeGLCD.h \
	${GEM_DIR}/FakeGLCD.cpp \
	
#-------------------------------------------------------------------------------
# Include directories
#-------------------------------------------------------------------------------
INCLUDE_DIRS = \
	$(DRIVERS_DIR) \
	$(CMSIS_DIR) \
	$(UTILITIES_DIR) \
	$(BOARD_DIR) \
	$(SRC_DIR) \
	$(GEM_DIR)

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
TARGET_BIN=$(OBJECT_DIR)/$(TARGET).bin


#-------------------------------------------------------------------------------
# Flags
#-------------------------------------------------------------------------------
ifeq ($(DEBUG), 1)
	# Debug
	OPTIMIZE = -g -O1
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

LDFLAGS=${MCFLAGS} ${COMMON_FLAGS} --specs=nano.specs --specs=nosys.specs -Xlinker --gc-sections -Xlinker -static -Xlinker -z -Xlinker muldefs -Xlinker -print-memory-usage
LDFLAGS+=-T${GCC_DIR}/MKL16Z64xxx4_flash.ld -static


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
#	$(CP) -O ihex --set-start 0x0000000 $< $@

$(TARGET_BIN): $(TARGET_ELF)
	$(CP) -O binary $< $@

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

all: $(TARGET_BIN)

#all: $(TARGET_HEX)

$(DEPS):

include $(wildcard $(DEPS))