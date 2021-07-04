# ESPX - Exercise 2

# Binary Files

**timestamps.bin** contains the differences of times as "double".
**covidTrace.bin** contains the MAC address values as "uint64".

# User Space

To test locally, you should have the **arm-linux-gnueabihf-gcc** cross-compiler.
Note that you should have the RPi connected.

```
1. run "make rpi"
2. run "make finish"
3. enter the RPi's password
4. run in the RPi using "./rpi_espx"
```

For Windows, if you want to transfer the .bin files from the RPi to your
computer run:

```
1. go to the directory in which you want to tranfer the .bin files
2. run "scp root@192.168.0.1:timestamps.bin timestamps.bin"
3. run "scp root@192.168.0.1:covidTrace.bin covidTrace.bin"
```

# Kernel Space 

To test locally, you should have the **gcc** compiler.

```
1. run "make all"
2. run "make test"
```



Repo for the second exercise of course 077 - Real Time Embedded Systems, Aristotle University of Thessaloniki, Dpt. of Electrical & Computer Engineering.
