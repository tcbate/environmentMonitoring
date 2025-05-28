//thermometer.c
//Reads data from BME280 and formats it for display

#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>

/**
 * Initialization of i2c device.
 * returns the file descriptor initialized to the proper address
 * @return i2c_device
 */
int initialize(){
	char reg[2];
	char msg[2]= "\n\n";

	int i2c_device = open("/dev/i2c-1", O_RDWR);
	if(i2c_device < 0){
		perror("Failed to open i2c");
		return i2c_device;
	}
	int status = ioctl(i2c_device, I2C_SLAVE,0x76);
	if(status < 0){
		perror("I2C address set failed");
		return -1;
	}

	// for now: 
	reg[0] = 0xd0;
	write(i2c_device, reg, 1);
	read(i2c_device, msg, 1);
	printf("Device ID : %x \n",msg[0]);

	//set mode
	uint8_t mode = 1; //Forced mode 1 or 2. Sleep: 0, normal mode 3
	uint8_t osrs_t = 1; //other setting available p.27 of bosch documentation
	uint8_t osrs_p = 1; //see page 27
	uint8_t osrs_h = 1; //goes into 0xf2
	
	//set humidity
	reg[0] = 0xf2;
	reg[1] = 0x01;
	write(i2c_device,reg,2);
	//set the rest of the parameters 
	//
	reg[0] = 0xf4; //control register
	reg[1] = (osrs_t << 5) | (osrs_p << 2) | mode;
	write(i2c_device, reg, 2);
	usleep(10000); // for now, wait for the measurement to happen

	return i2c_device;
	
}
	
int main(){
	int i2c_device = initialize();
	if(i2c_device < 0){
		perror("i2c initialization failed");
	}
	char reg = 0xf7;
	write(i2c_device, &reg, 1);
       	char data[8];
	read(i2c_device, data, 8);

	uint32_t pressureRaw= (data[0]<<12) | (data[1]<<4) | ((data[2] & 0xf0)>>4);

	printf("Raw Pressure : %x\n",pressureRaw);

	uint32_t tempRaw = (data[3]<<12) | (data[4]<<4) | ((data[5] & 0xf0)>>4);

	printf("Raw Temperature : %x \n", tempRaw);

	uint32_t humidityRaw = (data[6] << 8) | (data[7]);

	printf("Raw Humidity : %x \n",humidityRaw);

}
	


	
