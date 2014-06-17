obj-m += selective_drop_caches.o

KERNEL_SRC=~/src/linux-2.6

all:
	make -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean

deliver:
	scp -P2222 selective_drop_caches.ko root@localhost:~/

kernel_deliver:
	scp -P2222 $(KERNEL_SRC)/arch/x86_64/boot/bzImage root@localhost:/boot
