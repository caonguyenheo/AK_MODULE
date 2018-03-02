#
#Makefile
#
CUR_DIR:=$(shell pwd)

#dir_y := ring_call
#dir_y += akipc

dir_y := main_ctrl
dir_y += video
dir_y += audio
dir_y += spi_cmd
dir_y += fwupgrade
dir_y += scheduler_layer
dir_y += debug_latency
dir_y += record
dir_y += akipc
dir_y += uploader
dir_y += sdcard
dir_y += p2p_sdcard_streaming
dir_y += ring_call
dir_y += flv_recorder

all: 
	@for i in $(dir_y); \
	do \
		$(MAKE) -C $$i  ; \
		if [ $$? -ne 0 ]; then\
		  exit 1;\
		fi \
	done

install:
	@for i in $(dir_y); \
	do \
		$(MAKE) -C $$i  install; \
	done

clean:
	@for i in $(dir_y); \
	do \
		$(MAKE) -C $$i  clean; \
	done

.PHONY: all install clean 

