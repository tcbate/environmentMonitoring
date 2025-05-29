//bme280.h
#ifndef BME280_H
#define BME280_H

/**This function finds the device and 
 * from there, downloads the calibration
 * data, and eventually, returns the file
 * descriptor
 * @return int File descriptor for the I2C device
 */
int bme280_initialize();

/**Converts the processed temperature into celsius
 * as a float
 * @return temperature in C
 */
float getTemperatureC();

/**Gets the pressure in HPa and returns it as a float
 * @return The pressure in hPa
 */
float getPressureHPa();

/**gets the parsed humidity from the sensor and 
 * converts it to a percentage
 * @return percent humidity in a float
 */
float getHumidity();

/**requests a sensor, referenced by the file
 * descriptor take a measurement
 *
 * @param The file descriptor for the i2c device
 */
void takeMeasurement();

/**Closes all file descriptors for the bme280
 */
void bme280_cleanup();

#endif
