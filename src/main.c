//main.c
//For the moment, the main file for this thing I am writing

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include "bme280.h"
#include "max7219.h"

void cleanup(){
	bme280_cleanup();
	max7219_cleanup();
}

void handle_sigint(int signum){
	(void)signum; //throw away the value
	write_string("        "); //clear the display
	cleanup();
	exit(0);
    }	

int main(){
	//Handle SIG INT
	struct sigaction sa;
	sa.sa_handler = handle_sigint;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGINT, &sa, NULL) == -1) {
        	perror("sigaction");
        	exit(EXIT_FAILURE);
    	}

	//initialize the sensor and display
	max7219_initialize();
	write_string("        ");

	if(bme280_initialize() < 0){
	    perror("Initialization Error BME280");
	    write_string("E00     ");
	    return -1;
	}
	while(1){
		if(takeMeasurement() < 0){
		    perror("Measurement error");
		    write_string("E01     ");
		}
		float temperature = getTemperatureC();
		float humidity = getHumidity();
		//printf("Current temperature is : %.0f C\n",temperature);

		char buffer[11];
		snprintf(buffer, 11, "%.1f  %.1f",temperature,humidity);
		write_string(buffer);
		sleep(10);
	}

	return 0;
}

