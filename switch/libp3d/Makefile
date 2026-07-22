.SUFFIXES:

TOPDIR ?= $(CURDIR)
export DEVKITPRO := $(TOPDIR)/../../vendor/devkitpro
ORIGINAL_PATH := $(PATH)

include $(DEVKITPRO)/libnx/switch_rules

export PATH := $(DEVKITPRO)/devkitA64/bin;$(DEVKITPRO)/tools/bin;$(ORIGINAL_PATH)

PORTLIBS_PATH := $(DEVKITPRO)/portlibs
PORTLIBS := $(PORTLIBS_PATH)/switch

DEBUG	?=	0

ifeq ($(strip $(DEBUG)),1)
	TARGET		:=	p3d-debug
	BUILD		:=	build-debug
	OPTFLAGS	:=	-Og
else
	TARGET		:=	p3d
	BUILD		:=	build
	OPTFLAGS	:=	-O3
endif

SOURCES		:=	../../vendor/libp3d/p3d \
			../../vendor/libp3d/pddi/gles \
			../../vendor/libp3d/vendor/imgui
INCLUDES	:=	../../vendor/libp3d \
			../../vendor/libp3d/vendor/imgui

ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

# See switch/rechan/Makefile for why these defines are needed.
DEFINES	:=	-DRC_PLATFORM_SWITCH -DRC_PLATFORM_NULL -DIMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
ifneq ($(strip $(DEBUG)),1)
	DEFINES	+=	-DNDEBUG
endif

CFLAGS	:=	-g -Wall $(OPTFLAGS) -ffunction-sections \
			$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE) -D__SWITCH__

# Deliberately not using -fno-rtti -fno-exceptions -- see switch/rechan/Makefile.
CXXFLAGS	:=	$(CFLAGS) -std=gnu++20

ASFLAGS	:=	-g $(ARCH)

LIBDIRS	:=	$(PORTLIBS) $(LIBNX)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export TOPDIR	:=	$(CURDIR)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

.PHONY: $(BUILD) clean all

all: lib/lib$(TARGET).a

lib:
	@[ -d $@ ] || mkdir -p $@

lib/lib$(TARGET).a: lib $(BUILD)
	@$(MAKE) OUTPUT=$(CURDIR)/$@ --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@[ -d $@ ] || mkdir -p $@

clean:
	@echo clean ...
	@rm -fr build lib

else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

$(OUTPUT)	:	$(OFILES)

-include $(DEPENDS)

endif
