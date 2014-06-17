TARGET := selective_drop_caches
obj-m += $(TARGET).o

KERNEL_SRC ?= ~/src/linux-2.6

SSH = ssh -p2222 root@localhost

all:
	make -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean

deliver:
	scp -P2222 selective_drop_caches.ko root@localhost:~/

kernel_deliver:
	scp -P2222 $(KERNEL_SRC)/arch/x86_64/boot/bzImage root@localhost:/boot/vmlinuz-3.16-amd64

qemu:
	kvm -s -m 512 -hda ../debian.img -redir tcp:2222::22 &

ssh:
	$(SSH)

RMMOD = $(SSH) rmmod $(TARGET).ko
INSMOD = $(SSH) insmod $(TARGET).ko

insmod:
	$(INSMOD)

rmmod:
	$(RMMOD)

COMMAND = $(SSH) cat /sys/module/selective_drop_caches/sections/

TEXT := $(COMMAND).text
DATA := $(COMMAND).data
BSS :=  $(COMMAND).bss

gdb: insmod
	gdb $(KERNEL_SRC)/vmlinux -ex "target remote localhost:1234" -ex \
	 "add-symbol-file ./$(TARGET).ko $$($(TEXT)) -s .data $$($(DATA)) -s .bss $$($(BSS))"
	$(RMMOD)

reboot:
	$(SSH) reboot
