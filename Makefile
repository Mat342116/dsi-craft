.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

TARGET := DSiCraft
BUILD := build
SOURCES := source
INCLUDES := include

ARCH := -march=armv5te -mtune=arm946e-s -mthumb

CFLAGS := -g -Wall -O2 -ffunction-sections -fdata-sections $(ARCH)
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=ds_arm9.specs -g $(ARCH)

LIBS := -lnds9 -lfat
LIBDIRS := $(LIBNDS)

include $(DEVKITPRO)/libnds/nds_rules
