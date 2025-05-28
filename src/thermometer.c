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
int32_t t_fine; //global variable used by the calibration

typedef struct{
	unsigned short dig_t1;
	signed short   dig_t2;
	signed short   dig_t3;
	unsigned short dig_p1;
	signed short dig_p2;
	signed short dig_p3;
	signed short dig_p4;
	signed short dig_p5;
	signed short dig_p6;
	signed short dig_p7;
	signed short dig_p8;
	signed short dig_p9;
	signed short dig_h2;
	signed short dig_h4;
	signed short dig_h5;
	unsigned char dig_h1;
	unsigned char dig_h3;
	unsigned char dig_h6;
} bme280Calibration;

static bme280Calibration calibrationData = {0};

void getCalibration(int fd){
	char reg[1]; //declaration
	reg[0] = 0x88;
	char cal1[25];
	write(fd,reg,1);
	read(fd,cal1,25);

	reg[0] = 0xe1;
	char cal2[7];
	write(fd,reg,1);
	read(fd,cal2,7);

	calibrationData.dig_t1 = (cal1[1] << 8) | cal1[0];
	calibrationData.dig_t2 = (cal1[3] << 8) | cal1[2];
	calibrationData.dig_t3 = (cal1[5] << 8) | cal1[4];
	
	calibrationData.dig_p1 = (cal1[7] << 8) | cal1[6];
	calibrationData.dig_p2 = (cal1[9] << 8) | cal1[8];
	calibrationData.dig_p3 = (cal1[11]<< 8) | cal1[10];
	calibrationData.dig_p4 = (cal1[13]<< 8) | cal1[12];
	calibrationData.dig_p5 = (cal1[15]<< 8) | cal1[14];
	calibrationData.dig_p6 = (cal1[17]<< 8) | cal1[16];
	calibrationData.dig_p7 = (cal1[19]<< 8) | cal1[18];
	calibrationData.dig_p8 = (cal1[21]<< 8) | cal1[20];
	calibrationData.dig_p9 = (cal1[23]<< 8) | cal1[22];
	// Read dig_H1 (single byte at 0xA1)
	reg[0] = 0xA1;
	write(fd, reg, 1);
	read(fd, &(calibrationData.dig_h1), 1);
//	calibrationData.dig_h1 = cal1[24];

	calibrationData.dig_h2 = (cal2[1] << 8) | cal2[0];
	calibrationData.dig_h3 = cal2[2];
	calibrationData.dig_h4 = (cal2[3] << 4) | (cal2[4] & 0xf);
	calibrationData.dig_h5 = ((cal2[4] & 0xf0) >>4) | (cal2[5] << 4);
	calibrationData.dig_h6 = (int8_t)cal2[6];
}
	
float getHumidity(uint32_t adc_h){
	int32_t v_x1_u32r;
	int32_t dig_h4 = (int32_t)calibrationData.dig_h4;
	int32_t dig_h5 = (int32_t)calibrationData.dig_h5;
	int32_t dig_h6 = (int32_t)calibrationData.dig_h6;
	int32_t dig_h3 = (int32_t)calibrationData.dig_h3;
	int32_t dig_h2 = (int32_t)calibrationData.dig_h2;
	int32_t dig_h1 = (int32_t)calibrationData.dig_h1;

	v_x1_u32r = (t_fine - ((int32_t)76800));
	v_x1_u32r = (((((adc_h << 14) - (((int32_t)dig_h4) << 20) - (((int32_t)dig_h5) * v_x1_u32r)) + ((int32_t)16384)) >> 15)
    * (((((((v_x1_u32r * ((int32_t)dig_h6)) >> 10) * (((v_x1_u32r * ((int32_t)dig_h3)) >> 11) + ((int32_t)32768))) >> 10)
    + ((int32_t)2097152)) * ((int32_t)dig_h2) + 8192) >> 14));
	/*
	v_x1_u32r = (((((adc_h << 14) - ((dig_h4) << 20) - ((dig_h5) * v_x1_u32r))
					+ ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * dig_h6)) >> 10) *
								(((v_x1_u32r * (dig_h3)) >> 11) +
								 ((uint32_t) 32786))) >> 10) + ((uint32_t)2097152)) * ((dig_h2) + 8192) >> 14));
								 */
	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
				(dig_h1)) >> 4));
	v_x1_u32r = (v_x1_u32r  < 0 ? 0 : v_x1_u32r);
	v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

	return (float)(v_x1_u32r>>12)/1024.0f;



}
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
	
	getCalibration(i2c_device);
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

float getTemperature(int32_t adc_T){
	//returns temperature in degrees C. Output is with 0.01 precision 5125 = 51.25c
	int32_t dig_t1 = (int32_t)calibrationData.dig_t1;
	int32_t dig_t2 = (int32_t)calibrationData.dig_t2;
	int32_t dig_t3 = (int32_t)calibrationData.dig_t3;
	int32_t var1, var2, T;

	var1 = ((((adc_T>>3) - (dig_t1<<1))) * (dig_t2)) >> 11;
	var2 = (((((adc_T>>4) - (dig_t1)) * ((adc_T>>4) - (dig_t1))) >> 12) * (dig_t3)) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	return T/100.0f;
}

float getPressure(int32_t adc_p) {
	int64_t var1, var2, p;
	int64_t dig_p6 = (int64_t)calibrationData.dig_p6;
	int64_t dig_p5 = (int64_t)calibrationData.dig_p5;
	int64_t dig_p4 = (int64_t)calibrationData.dig_p4;
	int64_t dig_p3 = (int64_t)calibrationData.dig_p3;
	int64_t dig_p2 = (int64_t)calibrationData.dig_p2;
	int64_t dig_p1 = (int64_t)calibrationData.dig_p1;
	int64_t dig_p9 = (int64_t)calibrationData.dig_p9;
	int64_t dig_p8 = (int64_t)calibrationData.dig_p8;
	int64_t dig_p7 = (int64_t)calibrationData.dig_p7;

	

	var1 = ((int64_t)t_fine) - 128000;
	var2 = var1 * var1 * dig_p6;
	var2 = var2 + ((var1*dig_p5)<<17);
	var2 = var2 + ((dig_p4) <<35);
	var1 = ((var1 * var1 * dig_p3) >> 8) + ((var1 * dig_p2) <<12);
	var1 = (((((int64_t)1)<<47) + var1)) * (dig_p1)>>33; 

	if (var1 == 0){
		return 0;
	}
	p = 1048576-adc_p;
	p = (((p<<31)-var2)*3125)/var1;
	var1 = ((dig_p9) * (p>>13) * (p>>13)) >> 25;
	var2 = ((dig_p8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + ((dig_p7)<<4);
	return (float)(p/256.0f);
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
	//debugging
	printf("dig_h1: %u dig_h2: %d dig_h3: %u dig_h4: %d dig_h5: %d dig_h6: %d\n", 
    calibrationData.dig_h1, calibrationData.dig_h2, calibrationData.dig_h3, 
    calibrationData.dig_h4, calibrationData.dig_h5, calibrationData.dig_h6);
	//end debugging
	uint32_t pressureRaw= (data[0]<<12) | (data[1]<<4) | ((data[2] & 0xf0)>>4);

	printf("Raw Pressure : 0x%x\n",pressureRaw);

	uint32_t tempRaw = (data[3]<<12) | (data[4]<<4) | ((data[5] & 0xf0)>>4);

	printf("Raw Temperature : 0x%x \n", tempRaw);
	

	uint32_t humidityRaw = (data[6] << 8) | (data[7]);

	printf("Raw Humidity : 0x%x \n",humidityRaw);
	
	float temperature = getTemperature(tempRaw);
	printf("Actual temperature : %.2f\n",temperature);

	float pressure = getPressure(pressureRaw);
	printf("Acutal Pressure : %.2f\n",pressure);

	float humidity = getHumidity(humidityRaw);
	printf("Actual Humidity : %.2f\n",humidity);

	printf("T_fine : %d \n",t_fine);

}
	


	
