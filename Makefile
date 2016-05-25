DIRS=drv aps cups sample

all:
	@for dir in $(DIRS); do\
		echo Making all in $$dir... ;\
		(cd $$dir ; $(MAKE) $(MFLAGS)) || exit 1;\
	done

clean:
	@for dir in $(DIRS); do\
		echo Cleaning in $$dir... ;\
		(cd $$dir ; $(MAKE) $(MFLAGS) clean) || exit 1;\
	done

install:
	@for dir in $(DIRS); do\
		echo Installing in $$dir... ;\
		(cd $$dir ; $(MAKE) $(MFLAGS) install) || exit 1;\
	done
	@echo "==========================================================="
	@echo "==========================================================="
	@echo "==========================================================="
	@echo "======"
	@echo "====== and this line in /etc/fstab for USB"
	@echo "======"
	@echo "====== usbfs           /proc/bus/usb   usbfs   devgid=7,devmode=0666 0 0 "
	@echo "======"
	@echo "==========================================================="
	@echo "==========================================================="
	@echo "==========================================================="

uninstall:
	@for dir in $(DIRS); do\
		echo Uninstalling in $$dir... ;\
		(cd $$dir ; $(MAKE) $(MFLAGS) uninstall) || exit 1;\
	done

