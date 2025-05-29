//main.c
//For the moment, the main file for this thing I am writing

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <bme280.h>

int main(){
	int i2c_device = initialize();
	takeMeasurement(i2c_device);
	float temperature = getTemperatureC(); 

	return 0;
}

