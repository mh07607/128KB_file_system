build:
	gcc filesystem.c -Wall -o filesystem.exe

launch:
	gcc filesystem.c -Wall -o filesystem.exe
	filesystem.exe
	
test:
	gcc a.c -Wall -o a.exe
	a.exe