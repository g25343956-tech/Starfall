# Makefile para devkitARM (devkitPro). Uso: make
.RECIPEPREFIX := >

ifeq ($(strip $(DEVKITARM)),)
$(error Define DEVKITARM)
endif

export PATH := $(DEVKITARM)/bin:$(DEVKITPRO)/tools/bin:$(PATH)

TARGET  := starfall
CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
ARCH    := -mthumb -mthumb-interwork -mcpu=arm7tdmi
CFLAGS  := $(ARCH) -O2 -Wall -Wextra -std=gnu99 -fno-strict-aliasing
LDFLAGS := $(ARCH) -specs=gba.specs

all: $(TARGET).gba

$(TARGET).elf: main.c
> $(CC) $(CFLAGS) main.c $(LDFLAGS) -o $@

$(TARGET).gba: $(TARGET).elf
> $(OBJCOPY) -O binary $< $@
> -gbafix $@ -tSTARFALL

clean:
> rm -f $(TARGET).elf $(TARGET).gba

.PHONY: all clean
