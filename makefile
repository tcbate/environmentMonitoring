displayFiles = src/display.c
displayTest = src/main_display.c
thermometerFiles = src/thermometer.c
output = main

thermometer: $(thermometerFiles) thermometerTest.c
	gcc $(thermometerFile) thermometerTest.c -o thermometerTest -I include

display: $(displayFiles) $(displayTest)
	gcc -g $(displayFiles) $(displayTest) -o displayTest -I include

debug: $(displayFiles) $(main)
	gcc -g $(displayFiles) $(displayTest) -o $(output) -I include




