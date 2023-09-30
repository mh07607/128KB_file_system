build:
	compile

launch:
	gcc filesystem.c -Wall -o filesystem.exe
	./filesystem.exe sampleinput.txt
	
clean:
	rm filesystem.exe	

compile:
	gcc filesystem.c -Wall -o filesystem.exe