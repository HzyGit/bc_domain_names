.PHONY: clean all tar
obj-m+=bc_domain_search.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	make -f Makefile.us all
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	make -f Makefile.us clean

