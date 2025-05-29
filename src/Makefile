obj-m += rootkit_module.o
EXTRA_CFLAGS += -DDEBUG
rootkit_module-objs := rootkit.o listener.o keylogger.o injector.o hider.o beacon.o
BUILD_DIR := $(PWD)/build

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
