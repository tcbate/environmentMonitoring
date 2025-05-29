//display.h
//Header file which contains the write files
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

/**@brief Initialization of the MAX7219
 * Initializes the MAX2719
 * @param fd The file descriptor for the device
 *static 
 */
void max7219_initialize(int fd);

/**@brief Function which writes a string to the screen
 * Takes a string as input and writes it to the 
 * device referenced by the file descriptor 
 * @param fd The file descriptor for the device
 * @param data The string being written to
 */
void write_string(int fd, char *data);
