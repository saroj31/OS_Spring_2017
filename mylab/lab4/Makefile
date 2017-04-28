
obj-m += mymod.o

default:
	$(MAKE) -C /usr/src/linux M=$(PWD) modules

clean:
	rm -f *.ko
	rm -f *.o
	rm -f *.mod.c
	rm -f *.order
	rm -f *.symvers
	rm -f *.order
	rm -f .*.c.swl
	rm -f .*.ko.cmd
	rm -f .*.c.swm
	rm -f .*.c.swn
	rm -f .*.c.swo
	rm -f .*.mod.o.cmd
	rm -f .*.o.cmd
	rm -f .*.c.swp
	rm -rf .tmp_versions
	rm -f a.out




	

