# shutdown
# 2019 - Created by Fred Nora.

VERSION = 0
PATCHLEVEL = 0
SUBLEVEL = 0
EXTRAVERSION =
#NAME = shutdown

CFLAGS = -m64 \
	-std=gnu89 \
	-nodefaultlibs \
	-nostdinc \
	-nostdlib \
	-static \
	-fgnu89-inline \
	-ffreestanding \
	-fno-builtin \
	-fno-pie \
	-fleading-underscore \
	-fno-stack-protector \
	-s \
	-Werror=strict-prototypes    


LIBC     = ../../alpha/libs/rtl/include/
LIBCOBJ  = ../../alpha/libs/rtl/obj

LIBIO    = ../../alpha/libs/libio01/include/
LIBIOOBJ = ../../alpha/libs/libio01/obj

#Objects
myObjects = crt0.o main.o qemu.o rtl.o \
ctype.o stdio.o stdlib.o string.o \
unistd.o termios.o ioctl.o fcntl.o libio.o    


.PHONY: all all-c jackpot-link finalize clean

all: main.o all-c jackpot-link finalize clean
	@echo "Ok?"
main.o:
	gcc  -c  main.c  $(CFLAGS) -I $(LIBC) -I $(LIBIO) -o main.o
all-c:

	gcc  -c  qemu.c  $(CFLAGS) -I $(LIBC) -I $(LIBIO) -o qemu.o

# Imports

	cp $(LIBCOBJ)/crt0.o   .
	cp $(LIBCOBJ)/rtl.o  .

	cp $(LIBIOOBJ)/libio.o  .

	cp $(LIBCOBJ)/ctype.o   .
	cp $(LIBCOBJ)/stdio.o .
	cp $(LIBCOBJ)/stdlib.o .
	cp $(LIBCOBJ)/string.o .
	cp $(LIBCOBJ)/unistd.o  .
	cp $(LIBCOBJ)/termios.o  .
	cp $(LIBCOBJ)/ioctl.o  .
	cp $(LIBCOBJ)/fcntl.o  .

jackpot-link:
	ld -m elf_x86_64 -T ../link.ld -o SHUTDOWN.BIN $(myObjects)    
finalize:
	cp SHUTDOWN.BIN  ../bin
clean:
	-rm *.o
	-rm SHUTDOWN.BIN 
	
