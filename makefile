build:
	gcc filesystem.c -Wall -o filesystem.exe

launch:
	gcc filesystem.c -Wall -o filesystem.exe
	filesystem.exe sampleinput.txt
	
clean:
	rm -f filesystem.exe

compile:
	gcc filesystem.c -Wall -o filesystem.exe