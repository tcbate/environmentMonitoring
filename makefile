displayFiles = src/display.c
main = src/main.c
output = main

debug: $(displayFiles) $(main)
	gcc -g $(displayFiles) $(main) -o $(output) -I include



