//main.c
//For the moment, the main file for this thing I am writing

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "bme280.h"
#include "max7219.h"

typedef struct{
    bool run;
    pthread_mutex_t stateMutex;
} ProgramState;

typedef struct{
    ProgramState *state;
    MeasurementStruct *measurements;
} SensorArgs;

typedef struct{
    ProgramState *state;
    MeasurementStruct *measurements;
} DisplayArgs;

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

void *measurementThreadFunction(void *args){
    SensorArgs *sensorArgs = args;
    MeasurementStruct *measurement = sensorArgs->measurements;
    ProgramState *programState = sensorArgs->state;

    pthread_mutex_lock(&programState->stateMutex);
	while(programState->run){
        pthread_mutex_unlock(&programState->stateMutex);

        pthread_mutex_lock(&measurement->measurementMutex);
        measurement->errorCode = takeMeasurement(measurement);
        pthread_mutex_unlock(&measurement->measurementMutex);
        pthread_cond_broadcast(&measurement->measurementCondition);
		
		sleep(10);

        pthread_mutex_lock(&programState->stateMutex);
	}
    pthread_mutex_unlock(&programState->stateMutex);

	return NULL;
}

void *displayThreadFunction(void *params){
    DisplayArgs *args = params;
    ProgramState *state = args->state;
    MeasurementStruct *measurement = args->measurements;

    uint8_t lastReading = 0;

    pthread_mutex_lock(&state->stateMutex);
    while(state->run){
        pthread_mutex_unlock(&state->stateMutex);
        
        //measurement critical area
        pthread_mutex_lock(&measurement->measurementMutex);
        //check if there is a new reading. This may drop readings in rare
        //cases.
        while(measurement->measurementNumber == lastReading){
            pthread_cond_wait(&measurement->measurementCondition,
                &measurement->measurementMutex);
        }
        float temperature = getTemperatureC(measurement);
	    float humidity = getHumidity(measurement);
        lastReading = measurement->measurementNumber;
        pthread_mutex_unlock(&measurement->measurementMutex);
        //end measurement critical area
        
	    char buffer[11];
	    snprintf(buffer, 11, "%.1f  %.1f",temperature,humidity);
	    write_string(buffer);

        pthread_mutex_lock(&state->stateMutex);
    }
    pthread_mutex_unlock(&state->stateMutex);

    return NULL;
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

    //initialize the shared elements for the threads
    ProgramState *state = malloc(sizeof(ProgramState));
    if(!state){
        perror("State Struct allocation failed");
        exit(1);
    }
    state->run = true;
    int error = pthread_mutex_init(&state->stateMutex, NULL);
    if(error != 0){
        fprintf(stderr,"%s \n",strerror(error));
        exit(2);
    }
    

    SensorArgs *sensorArgs = calloc(1,sizeof(SensorArgs));
    if(!sensorArgs){
        perror("sensorArgs allocation failed");
        exit(1);
    }

    MeasurementStruct *measurement = calloc(1,sizeof(MeasurementStruct));
    if(!measurement){
        perror("Measurement Struct allocation failed");
        exit(1);
    }
    measurement->errorCode = 0;
    error = pthread_mutex_init(&measurement->measurementMutex, NULL);
    if(error != 0){
        fprintf(stderr,"%s \n",strerror(error));
        exit(2);
    }
    error = pthread_cond_init(&measurement->measurementCondition, NULL);
    if(error != 0){
        fprintf(stderr,"%s \n",strerror(error));
        exit(2);
    }

    sensorArgs->state = state;
    sensorArgs->measurements = measurement;

    DisplayArgs *displayArgs = calloc(1, sizeof(DisplayArgs));
    displayArgs->state = state;
    displayArgs->measurements = measurement;

    //here is the part I make the pthrads
    pthread_t sensorThread;
    error = pthread_create(&sensorThread, 
            NULL, 
            &measurementThreadFunction, 
            sensorArgs);
    if(error != 0){
        fprintf(stderr,"%s \n",strerror(error));
        exit(3);
    }
        

    pthread_t spiDisplayThread;
    error = pthread_create(&spiDisplayThread, 
            NULL, 
            &displayThreadFunction, 
            displayArgs);
    if(error != 0){
        fprintf(stderr,"%s \n",strerror(error));
        exit(3);
    }
        
    //here is the part where the program terminates
    pthread_join(sensorThread, NULL);
    pthread_join(spiDisplayThread, NULL);

    return 0;
    
}
