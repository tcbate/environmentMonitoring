//main.c
//Main function for the program

#include <stdio.h>
#include <stdlib.h>
#include "max7219.h"

int main(){
	max7219_initialize();
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
		char buffer[17];
		read(STDIN_FILENO, buffer, 16);
		write_string(buffer);	
	}
}
