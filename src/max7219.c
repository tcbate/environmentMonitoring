//display.c
//Thomas Bate
//File to test and use the MAX2719 8-digit display
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

static int spi_device = 0;
/**@brief Handles the SPI commands to write to the registers
 * This funciton will construct the SPI message and then 
 * send it to the SPI device
 */
static int spi_write(int spi_device, __u8 addr, __u8 data){
	//charger le tampon des données
	__u16 send = ((__u16)data<< 8) | addr;

	//remplissage du struct
	struct spi_ioc_transfer mesg = {0};
	mesg.tx_buf = (unsigned long)&send;
	mesg.rx_buf = 0; //Pas de reception de données ici
	mesg.len = 2; //longueur de deux bytes
	mesg.speed_hz = 100000;
	mesg.delay_usecs = 0;
	mesg.bits_per_word = 8;
	mesg.cs_change = 0;
	mesg.pad = 0;

	//envoie de la message
	int result = ioctl(spi_device,SPI_IOC_MESSAGE(1),&mesg);
	return result;
}

void max7219_initialize(){
	spi_device = open("/dev/spidev0.0", O_RDWR);
	if(spi_device < 0){
		printf("Failed to open SPI device");
	}
	//decode mode p7 of datasheet
	spi_write(spi_device,0x09,0xff);
	//activate all digits
	spi_write(spi_device,0x0b,0x07);
	//display test off
	spi_write(spi_device,0x0f,0x0);
	//exit shutdown mode
	spi_write(spi_device,0x0c,0x01);

}

static void write_digit(int digit, int number){
	__u8 addr = (__u8)digit + 1;
	__u8 data = (__u8)number;

	spi_write(spi_device,addr,data);
}

void write_string(char *data){
	int len = 8;
	int j = 7;
	int i = 0;
	int digitCount = 8;
	while(i < len){
		if(data[i] == ' '){
			write_digit((__u8)len-i-1, 0xf);
		} else if (data[i] == 'p' || data[i] == 'P') {
			write_digit((__u8)len-i-1,0xe);
		} else if (data[i] == 'l' || data[i] == 'L'){
			write_digit((__u8)len-i-1,0xd);
		} else if (data[i] == 'h' || data[i] == 'H'){
			write_digit((__u8)len-i-1,0xc);
		} else if (data[i] == 'e' || data[i] == 'E'){
			write_digit((__u8)len-i-1, 0xb);
		} else if (data[i] == '-'){
			write_digit((__u8)len-i-1,0xa);
		} else if (data[i] == 'x' || data[i] == 'X'){
			//ne fait rien
		}
		else if (data[i] >= '0' && data[i] <='9'){
			__u8 chiffre = (__u8)(data[i] - '0');
			write_digit((__u8)len-i-1, chiffre);
		}
		i++;
	}
		

}

void max7219_cleanup(){
	close(spi_device);
}
