all: main

main: main.cpp
	mpic++ -Wall -pthread -std=c++11 main.cpp -o main
	@echo "Compilation complete"

debug: main.cpp
	mpirun -np $(np) ./main 1 > debug.txt

sort: debug.txt
	sort -n debug.txt -o sorted-debug.txt
