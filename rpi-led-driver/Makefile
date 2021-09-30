obj-m := rpi-led-driver.o

driver:
	make -C $(KERNEL_DIR) \
	     ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) \
		 modules

deploy:
	scp *.ko pi@raspberrypi:modules/.

clean:
	make -C $(KERNEL_DIR) \
		ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) \
		clean

