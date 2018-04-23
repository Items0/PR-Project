all: main

main: main.c
	mpicc main.c -o main
	@echo "Compilation complete"
