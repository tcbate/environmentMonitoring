//main.c
//Main function for the program

#include <stdio.h>
#include <stdlib.h>
#include "display.h"

int main(){
	int spi = open("/dev/spidev0.0", O_RDWR);
	if(spi < 0){
		printf("Failed to open SPI device");
		exit(1);
	}
	initialize(spi);
	int result = 0;
	/*result = spi_write(spi,0x0f,0x01);
	if(result < 1){
		perror("Spi write failed:");
	}*/
	//write digits. Pour mettre un décimal, il faut
	//ajouter un f avant le chiffre. Par exemple
	//spi_write(spi,0x4,0xf1) au lieu de ... (spi,0x4,0x1)
	/*

	char data[9] = "53262671";
	write_string(spi, data);
*/
	while(1){
		char buffer[9];
		read(STDIN_FILENO, buffer, 9);
		write_string(spi,buffer);	
	}
}
