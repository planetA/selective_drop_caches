obj-m += selective_drop_caches.o

KERNEL_SRC=~/src/linux-2.6

all:
	make -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean
