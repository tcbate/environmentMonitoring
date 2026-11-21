# environmentMonitoring

This project is the long and tedious version of the weekend Arduino project
of reading from a BME280 sensor and showing the humidity and temperature 
detected from the same on a MAX7219 driven eight digit, seven segment display.

As opposed to the weekend Arduino or Raspberry Pi project, this project runs 
using userspace drivers for the BME280 and MAX7219 display driver, based on a 
partial implementation of the datasheet in C, using Linux System Calls to the 
I2C and SPI controllers for sensor reading and for display.

### Hardware
This project uses : 
- BME 280 sensor which communicates over I2C
- A seven-segment, eight digit MAX7219 driven display controlled over SPI
- Raspberry Pi

### Building the project 

This project includes a Makefile. To build the project, navigate to the root
directory and execute `make`. This will compile the source. Once this file is
built, one just needs to execute `./environmentMonitor`

There are also two hardware test programs included. 

One to test the thermometer and the other to test the display. 

To build the thermometer test: 
`make thermometer`

and for the display test:
`make display`

These can then be launched using 
`./displayTest` and `./thermometerTest`
