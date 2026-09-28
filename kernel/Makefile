obj-m += pulsemeter_driver.o

# Overridable so it can be built against any target kernel's headers, e.g.:
#   make KDIR=/lib/modules/6.8.0-142-generic/build
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod pulsemeter_driver.ko

unload:
	sudo rmmod pulsemeter_driver

.PHONY: all clean load unload
