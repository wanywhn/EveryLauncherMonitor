KERNEL_PATH ?= /lib/modules/$(shell uname -r)/build

obj-m += mftrace_hook.o
mftrace_hook-objs := ftrace_hook.o arg_extractor.o vfs_change.o

all:
	make -C $(KERNEL_PATH) M=$(PWD) modules
test:
	gcc -o proc_test proc_test.c

clean:
	make -C $(KERNEL_PATH) M=$(PWD) clean
