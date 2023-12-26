KERNEL_SRC = /lib/modules/$(shell uname -r)/source
BUILD_DIR := $(shell pwd)
DTC_DIR = /lib/modules/$(shell uname -r)/build/scripts/dtc/
VERBOSE = 0

OBJS    = rpi-es9018k2m-dac.o  es9018k2m.o

obj-m := $(OBJS)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(BUILD_DIR) KBUILD_VERBOSE=$(VERBOSE) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(BUILD_DIR) clean
	rm -f rpi-es9018k2m-dac-overlay.dtb

dtbs:
	$(DTC_DIR)/dtc -@ -I dts -O dtb -o rpi-es9018k2m-dac-overlay.dtb rpi-es9018k2m-dac-overlay.dts

modules_install:
	install -m 0644 es9018k2m.ko     	/lib/modules/$(shell uname -r)/kernel/sound/soc/codecs/
	install -m 0644 rpi-es9018k2m-dac.ko /lib/modules/$(shell uname -r)/kernel/sound/soc/bcm/
	depmod -a

modules_remove:
	rm /lib/modules/$(shell uname -r)/kernel/sound/soc/codecs/es9018k2m.ko 
	rm /lib/modules/$(shell uname -r)/kernel/sound/soc/bcm/rpi-es9018k2m-dac.ko 
	depmod -a

install: all modules_install dtbs install_dtb
	modprobe es9018k2m
	modprobe rpi-es9018k2m-dac

remove:
	modprobe -r es9018k2m
	modprobe -r rpi-es9018k2m-dac


install_dtb:
	cp rpi-es9018k2m-dac-overlay.dtb /boot/overlays/

remove_dtb:
	rm /boot/overlays/rpi-es9018k2m-dac-overlay.dtb
