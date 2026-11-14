
.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is not set)
endif

include $(DEVKITPRO)/libnds/default_arm9/Makefile

TARGET := DSiCraft
BUILD := build
SOURCES := source
INCLUDES := include

ARCH := -mthumb -mthumb-interwork

CFLAGS := -g -Wall -O2 \
          -mcpu=arm946e-s \
          -mtune=arm946e-s \
          -ffast-math

CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g
LDFLAGS :=

LIBS := -lnds9
EOF :=

include $(DEVKITPRO)/libnds/nds_rules
