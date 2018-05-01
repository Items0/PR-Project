all: main

main: main.cpp
	mpic++ -Wall -pthread -std=c++11 main.cpp -o main

@echo "Compilation complete"