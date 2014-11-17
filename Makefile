.PHONY: clean all tar init
obj-m+=bc_domain_mem.o
bc_domain_mem-y:=bc_domain_search.o bc_domain_db.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	make -f Makefile.us all
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	make -f Makefile.us clean

init:
	ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .
