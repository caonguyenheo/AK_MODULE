# Project name
PROJ := sky39ev200


# Flags

include board.cfg

export TOPDIR 	 ?= $(shell pwd)/..

export KERNELDIR := $(shell pwd)

export INCLUDE    =	-I. \
		-I$(KERNELDIR)/include \
		-I$(KERNELDIR)/include/driver \
		-I$(KERNELDIR)/include/os \
		-I$(KERNELDIR)/include/lwip_$(LWIP_VER) \
		-I$(KERNELDIR)/include/lwip_$(LWIP_VER)/ipv4 \
		-I$(KERNELDIR)/include/fs \
		-I$(KERNELDIR)/mach/mach-$(chips) \
		-I$(KERNELDIR)/common \
		-I$(KERNELDIR)/fs \
		-I$(KERNELDIR)/include/partition \
		-I$(KERNELDIR)/partition \
		-I$(KERNELDIR)/net/wifi \
		-I$(KERNELDIR)/driver/include 



export DEFINE		:= $(DEFINE) 

LDFLAGS	= -p -X -Map maps.txt -T section.lds

WRAP_FUNC = malloc calloc realloc free

LDFLAGS += $(addprefix --wrap=, $(WRAP_FUNC))

#source code Locations
SRCDIR += common
SRCDIR += init
SRCDIR += mm
SRCDIR += partition

ifeq ($(WIFI), y)
SRCDIR += net/wifi/$(WIFI_CHIP)
SRCDIR += net/wifi
endif

ifeq ($(LWIP), y)
SRCDIR += net/lwip/lwip_$(LWIP_VER)
endif

ifeq ($(FS), y)
SRCDIR += fs
endif

IMAGEPATH  ?= $(KERNELDIR)/image
BUILDPATH  ?= $(KERNELDIR)/build

CSRCS += $(foreach d, $(SRCDIR), $(wildcard $d/*.c))

COBJS += $(patsubst %.c, $(BUILDPATH)/%.o, $(CSRCS))

COBJS +=  $(BUILDPATH)/mach/mach-$(chips)/mach-$(MACH).o

SOBJS =  $(BUILDPATH)/init/boot.o

BUILDDIR +=  $(SRCDIR)
BUILDDIR += mach/mach-$(chips)


lib-$(IPERF3)   += ./lib/lib_iperf3.a
lib-$(WIFI)	+= ./lib/libwifi_$(WIFI_CHIP).a
lib-$(LWIP)	+= ./lib/liblwip_$(LWIP_VER).a
lib-$(MEMORY)	+= ./lib/memory.a
lib-$(SYSDRV)	+= ./lib/drv_ak39xx.a
lib-$(AKOS)	+= ./lib/akos.a
lib-$(FS)	+= ./lib/mount.a
lib-$(FS)	+= ./lib/fat32fs.a
lib-$(ISPDRV)	+= ./lib/libispdrv_rtos.a
lib-$(PARTITION) += ./lib/libpartition_uclibc.a

lib-kernel += ./lib/libkernel_common.a
lib-kernel += ./lib/libdev_drv.a

#build rule
.PHONY: all prepare   target clean 

all: prepare   target 

prepare:
ifneq ($(BOARD), cc3200bd)
ifneq ($(BOARD), devbd)
ifneq ($(BOARD), demobd)
ifneq ($(BOARD), tx6630)
	$(error BOARD undefined!  Please use "make BOARD=demobd WIFI_CHIP=ap6212" cc3200bd/devbd/demobd BOARD  and ap6212/mv8801/sv6030 WIFI_CHIP  supported )
endif
endif
endif

ifneq ($(WIFI_CHIP), ap6212)
ifneq ($(WIFI_CHIP), mv8801)
ifneq ($(WIFI_CHIP), sv6030)
ifneq ($(WIFI_CHIP), ky801w)
	$(error WIFI_CHIP undefined!   WIFI_CHIP=ap6212 or mv8801 or sv6030)
endif
endif
endif
endif
endif

ifneq ($(BUILDPATH),)
	for i in $(BUILDDIR); \
	do mkdir -p $(BUILDPATH)/$$i; \
	done
endif
	mkdir -p $(IMAGEPATH)

target: $(IMAGEPATH)/$(PROJ).bin



$(IMAGEPATH)/$(PROJ).bin: $(IMAGEPATH)/$(PROJ).elf 
	@echo ---------------------[build binary]----------------------------------
	$(OBJCOPY) -O binary  $<  $@
	

$(IMAGEPATH)/$(PROJ).elf:  $(SOBJS) $(COBJS) $(lib-kernel) $(lib-y) $(APPOBJS) $(PLATLIBS)
	@echo ---------------------[build elf]----------------------------------
	$(LD) $(LDFLAGS)  -o $@ $(SOBJS) $(COBJS) $(APPOBJS) "-(" $(PLATLIBS) "-)" --whole-archive $(lib-kernel) --no-whole-archive $(lib-y) $(CLIB)

dump:		
	$(OBJDUMP)  -h -D -Mreg-names-raw --show-raw-insn $(IMAGEPATH)/$(PROJ).elf > $(IMAGEPATH)/$(PROJ).txt


clean : 
	rm -rf $(BUILDPATH) $(IMAGEPATH)

#Rules
include $(KERNELDIR)/rules.mk
