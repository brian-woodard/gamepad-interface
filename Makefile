all:
	g++ -c Gamepad.cpp
	g++ main.cpp -o main Gamepad.o

clean:
	rm -f *.o
	rm -f main
