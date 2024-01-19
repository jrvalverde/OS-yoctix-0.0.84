#
#  Makefile for Yoctix
#

help:
	@echo "Type 'make boot' to make a bootdisk."
	@echo "Type 'cd src/sys; make' to simply build a kernel."
	@echo "Type 'make clean' to clean up."

clean:
	rm -rf bootdisk/*

	cd src/sys; make clean; cd ../..
	cd src/bin/login; make clean; cd ../../..
	cd src/bin/misc; make clean; cd ../../..
	cd src/sbin/getty; make clean; cd ../../..
	cd src/sbin/init; make clean; cd ../../..
	cd src/MISC; make clean; cd ../..

boot:
	rm -rf bootdisk/*

	cd src/sys; make; cd ../..
	cp src/sys/yoctix bootdisk/

	mkdir bootdisk/bin
	cd src/bin/login; make; cd ../../..
	cp src/bin/login/login bootdisk/bin/

	cd src/bin/misc; make; cd ../../..
	cp src/bin/misc/misc bootdisk/bin/arch
	ln bootdisk/bin/arch bootdisk/bin/cat
	ln bootdisk/bin/arch bootdisk/bin/date
	ln bootdisk/bin/arch bootdisk/bin/echo
	ln bootdisk/bin/arch bootdisk/bin/hdump
	ln bootdisk/bin/arch bootdisk/bin/hostname
	ln bootdisk/bin/arch bootdisk/bin/mkdir
	ln bootdisk/bin/arch bootdisk/bin/ps
	ln bootdisk/bin/arch bootdisk/bin/pwd
	ln bootdisk/bin/arch bootdisk/bin/sleep
	ln bootdisk/bin/arch bootdisk/bin/sync
	ln bootdisk/bin/arch bootdisk/bin/uname

	mkdir bootdisk/sbin
	cd src/sbin/getty; make; cd ../../..
	cp src/sbin/getty/getty bootdisk/sbin/
	cd src/sbin/init; make; cd ../../..
	cp src/sbin/init/init bootdisk/sbin/
	ln bootdisk/sbin/init bootdisk/sbin/halt
	ln bootdisk/sbin/init bootdisk/sbin/reboot

	mkdir -p bootdisk/usr/share/games/fortune
	cp src/usr_share/games_fortune bootdisk/usr/share/games/fortune/fortunes

	cp -r src/etc bootdisk
	pwd_mkdb -p -d bootdisk/etc `pwd`/bootdisk/etc/master.passwd

	mkdir bootdisk/dev
	mkdir bootdisk/mnt
	mkdir bootdisk/proc
	mkdir bootdisk/tmp
	mkdir -p bootdisk/home/guest

	@echo "Note 1: You should check ownership and modes on all files. (For example"
	@echo "        bootdisk/etc/*pwd.db and such)"
	@echo "Note 2: Place additional files on the bootdisk, such as /bin/ls and /bin/sh."
	@echo "The 'bootdisk' directory should then contain a semi-usable bootdisk tree."
	@echo "Don't forget to install working bootblock(s) on the boot disk."

