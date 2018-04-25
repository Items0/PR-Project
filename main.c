#include <stdio.h>
#include <mpi.h>
#define nArbiter 2
#define MYTAG 100

/*struct Message {
	int sender_id;
	int tag;
	int timestamp;

}*/
/*
	tablica message
	0 - sender_id
	1 - tag
	2 - timestamp
	3 - decision 
		-1 - nie
		0 - nic
		1 - tak

	TAGI:
	10 - zapytanie o arbita
	20 - odpowiedz do arbitra
*/
int main(int argc, char **argv)
{
	int size, rank;
	int arbiter = nArbiter;
	int lamportClock = 0;

	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	int ARtab[size];
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int message[4];

	if(rank == 0) 
	{
		int i;
		for (i = 0; i < size; i++)
		{
			if (i != rank)
			{
				lamportClock += 1;
				message[0] = rank;
				message[1] = 10;
				message[2] = lamportClock;
				message[3] = 0;
				MPI_Send(&message, 4, MPI_INT, i, MYTAG, MPI_COMM_WORLD);
				//printf("---0: Wyslalem--- %d\n", i);
				printf("%d: Wyslalem z czasem %d, do %d\n", rank, lamportClock, i);
			}	
		}
		for(i = 0; i < size - 1; i++)
		{
			MPI_Recv(&message, 4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_SOURCE, MPI_COMM_WORLD, &status);
			printf("---0: Odebralem---(Nadawca = %d, TAG = %d, Clock nadawcy = %d, decyzja = %d)\n",message[0], message[1], message[2], message[3]);
			if(lamportClock < message[2])
			{
				lamportClock = message[2] + 1;
			}
			else
			{
				lamportClock += 1;
			}
			printf("Lamport clock po odebraniu = %d)\n", lamportClock);
		}
	}
	else 
	{
		MPI_Recv(&message, 4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_SOURCE, MPI_COMM_WORLD, &status);
		printf("%d: Odebralem\n", rank);
		if(lamportClock < message[2])
		{
			lamportClock = message[2] + 1;
		}
		else
		{
			lamportClock += 1;
		}
		printf("%d: Odebralem z czasem %d\n", rank, lamportClock);
		lamportClock += 1;
		message[0] = rank;
		message[1] = 20;
		message[2] = lamportClock;
		message[3] = 1;
		MPI_Send(&message, 4, MPI_INT, 0, MYTAG, MPI_COMM_WORLD);
		printf("%d: Wyslalem z czasem%d\n", rank, lamportClock);
	}

	MPI_Finalize();
}
