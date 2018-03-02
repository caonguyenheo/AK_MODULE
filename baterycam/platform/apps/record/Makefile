#Kernel location

TOPDIR 	?= $(shell pwd)/../../..
KERNELDIR ?= $(TOPDIR)/kernel
PLATDIR ?= $(TOPDIR)/platform



# Flags

INCLUDE    =	-I. \
		-I$(KERNELDIR)/include \
		-I$(KERNELDIR)/include/driver \
		-I$(PLATDIR)/libplat/include \
		-I$(PLATDIR)/libmpi/include \
		-I$(PLATDIR)/libapp/include \
		-I$(PLATDIR)/apps/config \
		-I$(PLATDIR)/apps/audio \
		-I$(PLATDIR)/apps/video \
		-I$(PLATDIR)/apps/queue \
		-I$(PLATDIR)/apps/ringbuffer \
		-I$(PLATDIR)/apps/scheduler_layer \
		-I$(PLATDIR)/apps/spi_cmd \
		-I$(PLATDIR)/apps/spi_layer \
        -I$(PLATDIR)/apps/fwupgrade \
		-I$(PLATDIR)/apps/main_ctrl \
		-I$(PLATDIR)/apps/record \

#Src locations


SRCDIR += .

CSRCS += $(foreach d, $(SRCDIR), $(wildcard $d/*.c))

COBJS += $(patsubst %.c, %.o, $(CSRCS))



#build rule
.PHONY: all target clean

all: target 


target: $(COBJS) 
	@echo ---------------------[build obj complete]----------------------------------	


clean : 
	$(RM) $(COBJS)


# Rules
include $(TOPDIR)/rules.mk

