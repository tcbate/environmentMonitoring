//bme280.c
//Reads data from BME280 and formats it for display

#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include "bme280.h"
#define BME280_ADDRESS 0x76 //BME280 address. It can also be 0x77
//macro definitions for device registers
#define ID 0xD0 //contains 0x60 for device id (BME280) 
#define RESET 0xe0 //resets if 0xB6 is written to register
#define CTRL_HUM 0xf2 //for oversampling of humidity p.28 of the datasheet
#define CONFIG 0xf5 //sets options p.30. Writes may be ignored if not in sleep mode
#define STATUS 0xF3 //contains device status
#define CTRL_MEAS 0xf4 //sets pressure and temperature settings. p.29 of datasheet
		       //et this after ctrl_hum
#define PRESS 0xf7 //pressure registers starting from 0xf7-0xf9 p.31
#define TEMP 0xfa // temperature register (from 0xfa - 0xfc) p.31
#define HUM 0xfd // humidity register (from 0xfd - 0xfe)

#define CALIB 0x88 //from 0x88-0xa1. p.27 and p.24
#define CALIB2 0xe1 //from 0xe1 - 0xf0 p.24 and 27

static int32_t t_fine; //global variable used by the calibration
static int i2c_device = -1;
/** Struct which stores the calibration data used for calculating
 *  the final, readable, environmental values
 */
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

static bme280Calibration calibrationData = {0}; //initialize the struct at 0
						//

/**This function downloads the calibration data from the sensor
 * and places it in the struct bme280Calibration calibrationData
 */

static int getCalibration(int fd){
	char reg[1]; //declaration pour le registre
	reg[0] = CALIB; 
	char cal1[25]; //buffer pour lire les résultats
	if(write(fd,reg,1) < 0){ //demande de l'information
	    perror("Error requesting calibration 1");
	    return -1; 
	}
	if(read(fd,cal1,25) < 0){ //retourner l'information
	    perror("Error getting calibration 1");
	    return -1;
	}
	reg[0] = CALIB2; //work around to get 
	char cal2[7];
	if(write(fd,reg,1) < 0){
	    perror("Failed to request calibration 2");
	    return -1;
	}
	if(read(fd,cal2,7) < 0){
	    perror("Error getting calibration 2");
		return -1;
	}

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
	reg[0] = 0xA1; //Doing it this way as it didn't seem to work reading it the normal way
	if(write(fd, reg, 1) < 0){
	    perror("Error requesting calibration 3");
	    return -1;
	}
	if(read(fd, &(calibrationData.dig_h1), 1) < 0){
	    perror("Error reading calibration 3");
	    return -1;
	}
//	calibrationData.dig_h1 = cal1[24];

	calibrationData.dig_h2 = (cal2[1] << 8) | cal2[0];
	calibrationData.dig_h3 = cal2[2];
	calibrationData.dig_h4 = (cal2[3] << 4) | (cal2[4] & 0xf);
	calibrationData.dig_h5 = ((cal2[4] & 0xf0) >>4) | (cal2[5] << 4);
	calibrationData.dig_h6 = (int8_t)cal2[6];

	return 0;
}
/**A function which converts the raw humidity data into a human
 * readable form.
 * This code is found on page 25-26 of the datasheet
 * @param adc_h : the raw humidity data
 * @return A human readable humidity data (in float for now)
 */ 	
static int32_t parseHumidity(uint32_t adc_h){
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
	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
				(dig_h1)) >> 4));
	v_x1_u32r = (v_x1_u32r  < 0 ? 0 : v_x1_u32r);
	v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

	return (v_x1_u32r>>12);
}
/** Function which takes no input, and returns nothing. This function
 *  is to set up and initialize the BME-280. It is currently initialized
 *  in forced mode, with no oversampling. The forced mode will need to
 *  be called each time a reading will be taken by writing to the
 *  appropriate register with the settings. 
 *
 *  @return The file descriptor for the BME280
 */
int bme280_initialize(){
	char reg[2];
	char msg[2]= "\n\n";

	i2c_device = open("/dev/i2c-1", O_RDWR);
	if(i2c_device < 0){
		perror("Failed to open i2c");
		return -1;
	}
	int status = ioctl(i2c_device, I2C_SLAVE,BME280_ADDRESS);
	if(status < 0){
		perror("I2C address set failed");
		return -1;
	}

	if(getCalibration(i2c_device) < 0){
		perror("Failed to get calibration");
		return -1;
	}
	//set mode
	uint8_t mode = 1; //Forced mode 1 or 2. Sleep: 0, normal mode 3
	uint8_t osrs_t = 1; //other setting available p.27 of bosch documentation
	uint8_t osrs_p = 1; //see page 27
	uint8_t osrs_h = 1; //goes into 0xf2
	
	//set humidity
	reg[0] = CTRL_HUM;
	reg[1] = osrs_h; //write 01 to register. P.28 shows 0x01 = oversampling x 1
	if(write(i2c_device,reg,2) < 0){
	    perror("Failed to configure BME280");
	    return -1;
	}
	//set the rest of the parameters 
	//
	reg[0] = CTRL_MEAS; //control register
	reg[1] = (osrs_t << 5) | (osrs_p << 2) | mode;
	if(write(i2c_device, reg, 2) < 0){
	    perror("Failed to take measurement for BME280");
	    return -1;
	}
	usleep(10000); // wait 10ms for the measurement to complete
	
	return 0;
	
}
/**This function converts the temperature from the raw sensor data into
 * a more human readible form. This is stored in the processed data struct.
 * The basis for this is on p.25
 *
 * @param adc_T the raw temperature data
 */
static int32_t parseTemperature(int32_t adc_T){
	//returns temperature in degrees C. Output is with 0.01 precision 5125 = 51.25c
	int32_t dig_t1 = (int32_t)calibrationData.dig_t1;
	int32_t dig_t2 = (int32_t)calibrationData.dig_t2;
	int32_t dig_t3 = (int32_t)calibrationData.dig_t3;
	int32_t var1, var2, T;

	var1 = ((((adc_T>>3) - (dig_t1<<1))) * (dig_t2)) >> 11;
	var2 = (((((adc_T>>4) - (dig_t1)) * ((adc_T>>4) - (dig_t1))) >> 12) * (dig_t3)) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	
	return T;
}

/**Accepts the raw pressure data from the sensor, and converts
 * it into human readable form. The processed data is stored
 * in the processed data struct.
 * The basis for this formula is on p.25
 * @param adc_p The raw pressure data
 */
static int32_t parsePressure(int32_t adc_p) {
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
		return;
	}
	p = 1048576-adc_p;
	p = (((p<<31)-var2)*3125)/var1;
	var1 = ((dig_p9) * (p>>13) * (p>>13)) >> 25;
	var2 = ((dig_p8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + ((dig_p7)<<4);

	return (uint32_t)p;
}

/**This function converts the parsed form of the temperature into
 * celsius
 * @return The temperature in Celsius as a float
 */
float getTemperatureC(MeasurementStruct *measurement){
	int32_t temperature = measurement->temperature;
	return (float)temperature/100.0f;
}

float getPressureHPa(MeasurementStruct *measurement){
	uint32_t pressure = measurement->pressure;
	return (float)pressure/25600.0f; //256 to convert from raw to pa. 100 to convert to hPa
}

float getHumidity(MeasurementStruct *measurement){
	uint32_t humidity = measurement->humidity;
	return (float)humidity/1024.0f;
}

int takeMeasurement(MeasurementStruct *measurement){
	//parameters needed to request a read
	uint8_t mode = 1; //Forced mode 1 or 2. Sleep: 0, normal mode 3
	uint8_t osrs_t = 1; //other setting available p.27 of bosch documentation
	uint8_t osrs_p = 1; //see page 27
	uint8_t osrs_h = 1; //goes into 0xf2

	//set up and take the measurement
	char reg[2];
	reg[0] = CTRL_MEAS; //control register
	reg[1] = (osrs_t << 5) | (osrs_p << 2) | mode;
	if(write(i2c_device, reg, 2) < 0){
	    perror("Failed to take measurement");
	    return -1;
	}
	usleep(10000); // for now, wait for the measurement to happen 
		       
	//read the data 
	reg[0] = PRESS; //burst read starting with the pressure register
	if(write(i2c_device, &reg, 1) < 0){
	    perror("Failed to request data read BME280");
	    return -1;
	}
       	char data[8];
	if(read(i2c_device, data, 8) < 0){
	    perror("Failed to read data from BME280");
	    return -1;
	}

	//fill the variables based on p.31	
	uint32_t pressureRaw = (data[0]<<12) | (data[1]<<4) | ((data[2] & 0xf0)>>4);

	uint32_t tempRaw = (data[3]<<12) | (data[4]<<4) | ((data[5] & 0xf0)>>4);

	uint32_t humidityRaw = (data[6] << 8) | (data[7]); 

	measurement->temperature = parseTemperature(tempRaw);
	measurement->pressure = parsePressure(pressureRaw);
	measurement->humidity = parseHumidity(humidityRaw);
    measurement->measurementNumber++;

	return 0;
}

void bme280_cleanup(){
	close(i2c_device);
}	
	
/*
int main(){
	int i2c_device = initialize();
	if(i2c_device < 0){
		perror("i2c initialization failed");
	}
	
	takeMeasurement(i2c_device);
	
	float temperature = getTemperatureC();
	printf("Actual temperature : %.2f\n",temperature);

	float pressure = getPressureHPa();
	printf("Acutal Pressure : %.2f\n",pressure);

	float humidity = getHumidity();
	printf("Actual Humidity : %.2f\n",humidity);

	printf("T_fine : %d \n",t_fine);

}
	*/


	
