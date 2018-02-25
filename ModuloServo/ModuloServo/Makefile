
obj-m += pushbutton_irq_handler.o

#cleanbuild:
#	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
#	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

