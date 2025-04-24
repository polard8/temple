# It builds the whole opearating system.

# License: MIT License
# Compiling on gcc 11.4.0 (Ubuntu on wsl)
# Linking on ld 2.38

# Building full distributions into this directory.
DISTROS = 0000dist

# Target directory for the binaries.
# The binaries compiled here will go to this directory.
BASE = $(DISTROS)/base00

# Quote:
# "Do Oiapoque ao Chuí"
# The kernel lives in a shell.

## =================================
## BACK-END:
# Kernel Core: Boot, kernel and ring 0 modules.
DEP_L0 = kernel
# Kernel Services: Init process, ring 3 drivers and ring 3 servers.
DEP_L1 = misc

## =================================
## FRONT-END:
# Shell Pre-UI: The display server.
DEP_L2 = adam
# Shell UI: Client-side GUI applications.
DEP_L3 = apps

# --------------------------
# Display servers
DISPLAY_SERVERS = $(DEP_L2)/ds
# windowing system with 3D demos
GAMES           = $(DEP_L2)/gram3d

# --------------------------
# Client-side GUI applications
APPLICATIONS = $(DEP_L3)/osshell

# Make variables (CC, etc...)
AS      = as
LD      = ld
CC      = gcc
AR      = ar
MAKE    = make
NASM    = nasm
PYTHON  = python
PYTHON2 = python2
PYTHON3 = python3

#
# Config
#

# verbose
# Quiet compilation or not.
ifndef CONFIG_USE_VERBOSE
	CONFIG_USE_VERBOSE = 1
endif

ifeq ($(CONFIG_USE_VERBOSE),1)
# Not silent. It prints the commands.
	Q =
else
# silent
	Q = @
endif

# --------------------------------------
# == Start ====
# build: User command.
PHONY := all
all:  \
build-gramado-os \
copy-extras \
$(DEP_L3)/gramvd \
vhd-mount \
vhd-copy-files \
vhd-unmount \
clean    

# Giving permitions to run.
	chmod 755 ./run
	chmod 755 ./runnokvm

# tests
	chmod 755 ./runt1
	chmod 755 ./runt2
	@echo "Done?"

# --------------------------------------
# build: Developer comand 1.
# install
# Build the images and put them all into $(BASE)/ folder.
PHONY := install
install: do_install
do_install: \
build-gramado-os  


# --------------------------------------
# build: Developer comand 2.
# image
# Copy all the files from $(BASE)/ to the VHD.
PHONY := image
image: do_image
do_image: \
$(DEP_L3)/gramvd    \
vhd-mount          \
vhd-copy-files     \
vhd-unmount        \

# --------------------------------------
#::0
# ~ Step 0: gramado files.
PHONY := build-gramado-os  
build-gramado-os:     
	@echo ":: [] Building VHD, bootloaders and kernel image."
# options: 
# main.asm and main2.asm
# O mbr só consegue ler o root dir para pegar o BM.BIN
# See: stage1.asm
# O BM.BIN só consegue ler o root dir pra pegar o BL.BIN
# See: main.asm
# the kernel image
# O BL.BIN procura o kernel no diretorio GRAMADO/
# See: fs/loader.c

#===================================
# (1) $(DEP_L0)/boot/ 

# ::Build stuuf in $(DEP_L0)/boot/
	$(Q)$(MAKE) -C $(DEP_L0)/boot/

# Copy the virtual disk into the rootdir.
	cp $(DEP_L0)/boot/GRAMHV.VHD  .
# Copy $(DEP_L0)/bootloader stuff into rootdir.
	cp $(DEP_L0)/boot/x86/bsp/bin/BM.BIN      $(BASE)/
	cp $(DEP_L0)/boot/x86/bsp/bin/BM2.BIN     $(BASE)/
	cp $(DEP_L0)/boot/x86/bsp/bin/BLGRAM.BIN  $(BASE)/
	cp $(DEP_L0)/boot/MBR0.BIN                $(BASE)/
# Copy bootloader stuff into GRAMADO/ folder.
	cp $(DEP_L0)/boot/x86/bsp/bin/BM.BIN      $(BASE)/GRAMADO
	cp $(DEP_L0)/boot/x86/bsp/bin/BM2.BIN     $(BASE)/GRAMADO
	cp $(DEP_L0)/boot/x86/bsp/bin/BLGRAM.BIN  $(BASE)/GRAMADO
	cp $(DEP_L0)/boot/MBR0.BIN                $(BASE)/GRAMADO

#===================================
# (2) $(DEP_L0)/kernel/

# ::Build kernel image.
	$(Q)$(MAKE) -C $(DEP_L0)/kernel/

# Copy the kernel to the standard system folder.
	cp $(DEP_L0)/kernel/KERNEL.BIN  $(BASE)/GRAMADO
# Create a backup; The bootloder expects this.
	cp $(DEP_L0)/kernel/KERNEL.BIN  $(BASE)/DE

#===================================
# (3) $(DEP_L0)/modules/

# ::Build the ring0 module image.
	$(Q)$(MAKE) -C $(DEP_L0)/modules/

# Copy the ring0 module image.
# It is loadable, but it's not a dynlinked format.
	cp $(DEP_L0)/modules/bin/HVMOD0.BIN  $(BASE)/
	cp $(DEP_L0)/modules/bin/HVMOD0.BIN  $(BASE)/GRAMADO

# Copy the ring0 module image.
# It is loadable, but it's not a dynlinked format.
#	cp $(DEP_L0)/modules/bin/HVMOD1.BIN  $(BASE)/
#	cp $(DEP_L0)/modules/bin/HVMOD1.BIN  $(BASE)/GRAMADO

# ...

# ------------------------
# LEVEL : kbase
	$(Q)$(MAKE) -C $(DEP_L1)

#===================================
# $(DEP_L1)/
# Build and copy init process and some other programs.

#	$(Q)$(MAKE) -C $(DEP_L1)/

# Copy the init process.
	cp $(DEP_L1)/bin/INIT.BIN  $(BASE)/
#	cp $(DEP_L1)/bin/INIT.BIN  $(BASE)/GRAMADO/

# Well consolidated programs.
	-cp $(DEP_L1)/bin/PUBSH.BIN    $(BASE)/GRAMADO/
	-cp $(DEP_L1)/bin/PUBSH.BIN    $(BASE)/DE/
	-cp $(DEP_L1)/bin/SHELL.BIN    $(BASE)/GRAMADO/
	-cp $(DEP_L1)/bin/SHELL.BIN    $(BASE)/DE/
	-cp $(DEP_L1)/bin/SHELL00.BIN  $(BASE)/GRAMADO/
	-cp $(DEP_L1)/bin/SHELL00.BIN  $(BASE)/DE/

# Experimental programs.
	-cp $(DEP_L1)/bin/SH7.BIN        $(BASE)/GRAMADO/
#	-cp $(DEP_L1)/bin/SHELLXXX.BIN   $(BASE)/GRAMADO/

#===================================
# $(DEP_L1)/commands/

# Copy well consolidated commands.
	-cp $(DEP_L1)/commands/base/bin/CAT.BIN       $(BASE)/
	-cp $(DEP_L1)/commands/base/bin/CAT00.BIN     $(BASE)/
	-cp $(DEP_L1)/commands/base/bin/REBOOT.BIN    $(BASE)/
	-cp $(DEP_L1)/commands/base/bin/REBOOT.BIN    $(BASE)/GRAMADO/
	-cp $(DEP_L1)/commands/base/bin/SHUTDOWN.BIN  $(BASE)/
	-cp $(DEP_L1)/commands/base/bin/SHUTDOWN.BIN  $(BASE)/GRAMADO/

#===================================
# $(DEP_L1)/drivers/ in kernel project

#===================================
# $(DEP_L1)/servers/ in kernel project

#===================================
# Install BMPs from cali assets.
# Copy the $(DEP_L3)/assets/
# We can't survive without this one.
#	cp $(DEP_L3)/assets/themes/theme01/*.BMP  $(BASE)/
	cp $(DEP_L3)/assets/themes/theme01/*.BMP  $(BASE)/DE

	@echo "~build-gramado-os end?"

# --------------------------------------
# Let's add a bit of shame in the project.
PHONY := copy-extras
copy-extras:

	@echo "copy-extras"

# ------------------------
# LEVEL : Display servers
	make -C $(DEP_L2)/
# 3D demos.
	-cp $(GAMES)/bin/DEMO00.BIN   $(BASE)/DE/

# ------------------------
# LEVEL : (os/) Client-side GUI applications
	make -C $(DEP_L3)/
	-cp $(APPLICATIONS)/bin/TERMINAL.BIN   $(BASE)/DE
	-cp $(APPLICATIONS)/bin/TERM00.BIN     $(BASE)/DE
#-cp $(APPLICATIONS)/bin/GWS.BIN       $(BASE)/DE
    # Experimental applications
    # These need the '#' prefix.
	-cp $(APPLICATIONS)/bin/PUBTERM.BIN  $(BASE)/DE/

	@echo "~ copy-extras"

# --------------------------------------
#::2
# Step 2: $(DEP_L3)/gramvd  - Creating the directory to mount the VHD.
$(DEP_L3)/gramvd:
	@echo "========================="
	@echo "Build: Creating the directory to mount the VHD ..."
	sudo mkdir $(DEP_L3)/gramvd

# --------------------------------------
#::3
# ~ Step 3: vhd-mount - Mounting the VHD.
vhd-mount:
	@echo "=========================="
	@echo "Build: Mounting the VHD ..."
	-sudo umount $(DEP_L3)/gramvd
	sudo mount -t vfat -o loop,offset=32256 GRAMHV.VHD  $(DEP_L3)/gramvd/

# --------------------------------------
#::4
# ~ Step 4 vhd-copy-files - Copying files into the mounted VHD.
# Copying the $(BASE)/ folder into the mounted VHD.
vhd-copy-files:
	@echo "========================="
	@echo "Build: Copying files into the mounted VHD ..."
	# Copy $(BASE)/
	# sends everything from disk/ to root.
	sudo cp -r $(BASE)/*  $(DEP_L3)/gramvd

# --------------------------------------
#:::5
# ~ Step 5 vhd-unmount  - Unmounting the VHD.
vhd-unmount:
	@echo "======================"
	@echo "Build: Unmounting the VHD ..."
	sudo umount $(DEP_L3)/gramvd

# --------------------------------------
# Run on qemu using kvm.
PHONY := run
run: do_run
do_run:
	sh ./run

# --------------------------------------
# Run on qemu with no kvm.
PHONY := runnokvm
runnokvm: do_runnokvm
do_runnokvm:
	sh ./runnokvm


# --------------------------------------
# Basic clean.
clean:
	-rm *.o
	-rm *.BIN
	-rm $(DEP_L0)/kernel/*.o
	-rm $(DEP_L0)/kernel/*.BIN
	@echo "~clean"

# --------------------------------------
# Clean up all the mess.
clean-all: clean

	-rm *.o
	-rm *.BIN
	-rm *.VHD
	-rm *.ISO

	-rm $(DEP_L0)/boot/*.VHD 

# ==================
# (1) $(DEP_L0)/boot/
# Clear boot images
#	-rm -rf $(DEP_L0)/boot/arm/bin/*.BIN
	-rm -rf $(DEP_L0)/boot/x86/bin/*.BIN

# ==================
# (2) $(DEP_L0)/kernel/
# Clear kernel image
	-rm $(DEP_L0)/kernel/*.o
	-rm $(DEP_L0)/kernel/*.BIN
	-rm -rf $(DEP_L0)/kernel/KERNEL.BIN

# ==================
# (3) $(DEP_L0)/modules/
# Clear the ring0 module images
	-rm -rf $(DEP_L0)/modules/*.o
	-rm -rf $(DEP_L0)/modules/*.BIN
	-rm -rf $(DEP_L0)/modules/bin/*.BIN

# ==================
# $(DEP_L1)/
# Clear INIT.BIN
	-rm $(DEP_L1)/bin/*.BIN
	-rm $(DEP_L1)/init/*.o
	-rm $(DEP_L1)/init/*.BIN 

# ==================
# Clear the disk cache
	-rm -rf $(BASE)/*.BIN 
	-rm -rf $(BASE)/*.BMP
	-rm -rf $(BASE)/EFI/BOOT/*.EFI 
	-rm -rf $(BASE)/GRAMADO/*.BIN 
	-rm -rf $(BASE)/DE/*.BIN 
	-rm -rf $(BASE)/DE/*.BMP

	@echo "~clean-all"

# --------------------------------------
# Usage instructions.
usage:
	@echo "Building everything:"
	@echo "make all"
	@echo "Clear the mess to restart:"
	@echo "make clean-all"
	@echo "Testing on qemu:"
	@echo "./run"
	@echo "./runnokvm"

# --------------------------------------
# Danger zone!
# This is gonna copy th image into the real HD.
# My host is running on sdb and i copy the image into sda.
# It is because the sda is in primary master IDE.
# Gramado has been tested on sda
# and the Fred's Linux host machine is on sdb.
danger-install-sda:
	sudo dd if=./GRAMHV.VHD of=/dev/sda
danger-install-sdb:
	sudo dd if=./GRAMHV.VHD of=/dev/sdb

qemu-instance:
	-cp ./GRAMHV.VHD ./QEMU.VHD 
#xxx-instance:
#	-cp ./GRAMHV.VHD ./XXX.VHD 


# End

