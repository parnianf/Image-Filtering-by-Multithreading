CC=g++
STD=-std=c++11
CF=$(STD)


ImageFilters.out: main.o
	$(CC) $(CF) -o ImageFilters.out main.o

$(BUILD_DIR)/main.o: main.cpp
	$(CC) $(CF) -c main.cpp -o main.o


.PHONY: clean
clean:
	rm -rf *.o *.out