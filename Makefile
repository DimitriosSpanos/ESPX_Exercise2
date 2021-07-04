CC=gcc
CFLAGS=-O3
ARM_GCC=arm-linux-gnueabihf-gcc


default: all

espx:
	$(CC) $(CFLAGS) -o espx espx.c -lpthread

rpi:
	$(ARM_GCC) espx.c -o rpi_espx -lpthread

finish:
	scp rpi_espx root@192.168.0.1:~	

.PHONY: clean

all: espx

test:
	@printf "\n** Testing espx **\n\n"
	./espx

clean:
	rm -f espx rpi_espx
