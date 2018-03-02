#Kernel location

TOPDIR 	?= $(shell pwd)/../../..
KERNELDIR ?= $(TOPDIR)/kernel
PLATDIR ?= $(TOPDIR)/platform



# Flags

INCLUDE    =	-I. \
		-I$(PLATDIR)/libplat/include \
		-I$(PLATDIR)/libplat/src/include \
		-I$(PLATDIR)/libmpi/include \
		-I$(PLATDIR)/libapp/include 

INCLUDE += $(GLB_INCLUDE)


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

