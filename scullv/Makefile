#Comment/uncomment the following line to disable/enable debugging
#DEBUG=y

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
	DEBFLAGS = -O -g -DSCULLV_DEBUG # "-O" is needed to expand inlines
else
	DEBFLAGS = -O2
endif

LDDINC = $(PWD)/../include

EXTRA_CFLAGS += $(DEBFLAGS) -I$(LDDINC)

TARGET = scullv

ifneq ($(KERNELRELEASE),)
# call from kernel build system
scullv-objs := main.o mmap.o scull-shared/scull-async.o
obj-m := scullv.o

else
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD 	  := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

install:
	install -d $(INSTALLDIR)
	install -c $(TARGET).o $(INSTALLDIR)

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.mod modules.order *.symvers scull-shared/scull-async.o


depend .depend dep:
	$(CC) $(EXTRA_CFLAGS) -M *.c > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
