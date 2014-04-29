obj-m := kthreadwork.o
all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
