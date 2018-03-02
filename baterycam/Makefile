# Project name
PROJ := sky39ev200


#source code Locations
export TOPDIR      = $(shell pwd)
export KERNELDIR   = $(TOPDIR)/kernel
export PLATFORMDIR = $(TOPDIR)/platform

export IMAGEPATH   = $(TOPDIR)/image

include $(KERNELDIR)/board.cfg
include $(TOPDIR)/system.cfg

GLB_INCLUDE    =	-I$(KERNELDIR)/include \
		-I$(KERNELDIR)/include/lwip_$(LWIP_VER) \
		-I$(KERNELDIR)/include/lwip_$(LWIP_VER)/ipv4 \
		-I$(KERNELDIR)/include/os \
		-I$(KERNELDIR)/mm \
		-I$(KERNELDIR)/net/wifi \
		-I$(KERNELDIR)/include/driver 

DEFINE +=-DAK_RTOS
export DEFINE GLB_INCLUDE

#demo src
DEMODIR += $(PLATFORMDIR)/libapp/demo/ini
DEMODIR += $(PLATFORMDIR)/libapp/demo/dvr
DEMODIR += $(PLATFORMDIR)/libapp/demo/sysupd
DEMODIR += $(PLATFORMDIR)/libapp/demo/photo
DEMODIR += $(PLATFORMDIR)/libapp/demo/alarm
DEMODIR += $(PLATFORMDIR)/libapp/demo/version

ifeq ($(LWIP), y)
DEMODIR += $(PLATFORMDIR)/libapp/demo/net 
DEMODIR += $(PLATFORMDIR)/libapp/demo/uvc 
endif

ifeq ($(WIFI), y)
DEMODIR += $(PLATFORMDIR)/libapp/demo/wifi_$(WIFI_CHIP) 
endif

ifeq ($(LIBAPP), y)
PLATLIBS += $(shell find $(PLATFORMDIR)/libapp/lib/ -name "*.a")
endif

#libakstreamenclib should be placed behand libmpi_venc for link order
ifeq ($(LIBMPI), y)
DEMODIR += $(PLATFORMDIR)/libmpi/demo 
PLATLIBS += $(shell find $(PLATFORMDIR)/libmpi/lib/ -name "*.a")
endif

ifeq ($(LIBPLAT), y)
DEMODIR  += $(PLATFORMDIR)/libplat/demo 
PLATLIBS += $(shell find $(PLATFORMDIR)/libplat/lib/ -name "*.a")
endif

#main application
ifeq ($(RING), y)
APPDIR += $(PLATFORMDIR)/apps/ring_call
endif
ifeq ($(AKIPC), y)
APPDIR += $(PLATFORMDIR)/apps/akipc
APPDIR += $(PLATFORMDIR)/apps/test
endif

ifeq ($(RING_IPC), y)
APPDIR += $(PLATFORMDIR)/apps/ring_ipc
endif

APPDIR += $(PLATFORMDIR)/apps/main_ctrl
APPDIR += $(PLATFORMDIR)/apps/audio
APPDIR += $(PLATFORMDIR)/apps/video
APPDIR += $(PLATFORMDIR)/apps/spi_cmd
APPDIR += $(PLATFORMDIR)/apps/scheduler_layer
APPDIR += $(PLATFORMDIR)/apps/spi_layer
APPDIR += $(PLATFORMDIR)/apps/ringbuffer
APPDIR += $(PLATFORMDIR)/apps/fwupgrade
APPDIR += $(PLATFORMDIR)/apps/uploader
APPDIR += $(PLATFORMDIR)/apps/sdcard
APPDIR += $(PLATFORMDIR)/apps/p2p_sdcard_streaming
APPDIR += $(PLATFORMDIR)/apps/flv_recorder
ifeq ($(DEMO_SUPPORT), y)
APPDIR += $(DEMODIR)
endif

CSRCS += $(foreach d, $(APPDIR), $(wildcard $d/*.c))

APPOBJS += $(patsubst %.c, %.o, $(CSRCS))

#export for kernel use
export APPOBJS PLATLIBS


#build rule
.PHONY: all  apps kernel image  apps_clean clean 

all: prepare  apps kernel 

help:
	@echo '' 
	@echo 'Please use "make BOARD=xxxx WIFI_CHIP=xxxx" to make image'
	@echo 'eg. "make BOARD=demobd WIFI_CHIP=ap6212"'
	@echo '' 
	@echo 'Bynow cc3200bd/devbd/demobd three BOARD supported' 
	@echo 'cc3200bd is the black board with cc3200 wifi module'
	@echo 'devbd    is the big green board'
	@echo 'demobd   is the black board(composed by three boards) for ring call demo'
	@echo '' 
	@echo 'And ap6212/mv8801/sv6030 three WIFI_CHIP supported' 
	@echo 'ap6212   is the Broadcom AP6212 wifi module' 
	@echo 'mv8801   is the Marvell  MV8801 wifi module' 
	@echo 'sv6030   is the Icomm-sem sv6030 wifi module' 
	@echo '' 
	@echo 'Note: if you switch BOARD or WIFI_CHIP, Please make clean first' 
	@echo '' 

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

	
kernel: prepare
	$(MAKE) -C $(TOPDIR)/kernel 

apps: prepare
	@for i in $(APPDIR); \
	do \
		$(MAKE) -C $$i ; \
		if [ $$? -ne 0 ]; then\
		  exit 1;\
		fi \
	done

apps_clean:
	for i in $(APPDIR); \
	do \
		$(MAKE) -C $$i  clean; \
	done

	
image:
	$(MAKE) -C $(TOPDIR)/kernel 
dump:
	$(MAKE) -C $(TOPDIR)/kernel dump 

clean: apps_clean
	$(MAKE) -C $(TOPDIR)/kernel clean
	


#Rules
include $(TOPDIR)/rules.mk
