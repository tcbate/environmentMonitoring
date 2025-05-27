//display.h
//Header file which contains the write files
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

int spi_write(int fd, __u8 addr, __u8 data);

void initialize(int fd);

void write_digit(int fd, int digit, int number);

void write_string(int fd, char *data);
