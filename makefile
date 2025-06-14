displayFiles = src/max7219.c
displayTest = src/main_display.c
thermometerFiles = src/bme280.c
main = src/main.c
output = environmentMonitor

integrated: $(thermometerFiles) $(displayFiles) $(main) 
	gcc -g $(thermometerFiles) $(displayFiles) $(main) -I include -o $(output)

thermometer: $(thermometerFiles) thermometerTest.c
	gcc $(thermometerFile) thermometerTest.c -o thermometerTest -I include

display: $(displayFiles) $(displayTest)
	gcc -g $(displayFiles) $(displayTest) -o displayTest -I include

debug: $(displayFiles) $(main)
	gcc -g $(displayFiles) $(displayTest) -o $(output) -I include




