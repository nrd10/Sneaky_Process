ifeq ($(KERNELRELEASE),)  

KERNELDIR ?= /lib/modules/$(shell uname -r)/build 
PWD := $(shell pwd)  

.PHONY: build clean  

build:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules  
runhello: runhello.o
	gcc -o runhello runhello.o
runhello.o: runhello.c
	gcc -std=gnu99 -pedantic -Wall -c runhello.c

clean:
	rm -rf *.o *~ core .depend .*.cmd *.order *.symvers *.ko *.mod.c *.c~ runhello
else  

$(info Building with KERNELRELEASE = ${KERNELRELEASE}) 
obj-m :=    hello.o  

endif
