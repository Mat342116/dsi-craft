.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=$(DEVKITPRO)/devkitARM")
endif

TARGET := DSiCraft
BUILD := build
SOURCES := source
INCLUDES := $(DEVKITPRO)/libnds/include

# libnds 2.x uses the modern devkitARM rules (Calico-based).
include $(DEVKITARM)/ds_rules

ARCH := -march=armv5te -mtune=arm946e-s -mthumb
CFLAGS := -g -Wall -O2 -ffunction-sections -fdata-sections $(ARCH)
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=ds_arm9.specs -g $(ARCH)

LIBS := -lnds9 -lfat
LIBDIRS := $(DEVKITPRO)/libnds
