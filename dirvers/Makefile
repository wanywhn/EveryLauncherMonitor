KERNEL_MODULES :=/lib/modules/$(shell uname -r)/build

obj-m += everylauncher_monitor_drv.o
everylauncher_monitor_drv-objs :=everylauncher_monitor.o arg_extractor.o vfs_change.o

.PHONY: all clean 

all:
	make -C $(KERNEL_MODULES) M=$(PWD) modules

clean:
	make -C $(KERNEL_MODULES) M=$(PWD) clean
