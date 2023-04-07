CC=g++
STD=-std=c++11
CF=$(STD) 
LINK_FLAGS = -pthread


ImageFilters.out: main.o
	$(CC) $(CF) $(LINK_FLAGS) -o ImageFilters.out main.o

main.o: main.cpp
	$(CC) $(CF) $(LINK_FLAGS) -c main.cpp -o main.o


.PHONY: clean
clean:
	rm -rf *.o *.out