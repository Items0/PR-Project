#include <stdio.h>
#include <mpi.h>
#include <pthread.h>
#include <iostream>
#define nArbiter 2
#define MYTAG 100

using namespace std;

/*
	tablica message
	0 - sender_id
	1 - tag
	2 - timestamp

	TAGI:
	10 - zapytanie o arbita
	20 - odpowiedz do arbitra
	21 - odpowiedz OK
	22 - nie OK?
*/

int size, myrank;
int arbiter = nArbiter;
int lamportClock = 0;

void *receive_loop(void * arg);


pthread_mutex_t	receive_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t	send_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t	clock_mutex = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char **argv)
{
	// Enable thread in MPI 
	int provided = 0;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	//MPI_Init(&argc, &argv);

	//Create thread
	pthread_t receive_thread;
	pthread_create(&receive_thread, NULL, receive_loop, 0);

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	int message[3];
	MPI_Status status;
	if(myrank == 0) 
	{
		
		for (int i = 0; i < size; i++)
		{
			if (i != myrank)
			{
				lamportClock += 1;
				message[0] = myrank;
				message[1] = 10;
				message[2] = lamportClock;
				MPI_Send(&message, 3, MPI_INT, i, MYTAG, MPI_COMM_WORLD);
				cout << lamportClock << ":" << myrank << "\tWyslalem do " << i << "\n";
			}	
		}
		for(int i = 0; i < size - 1; i++)
		{
			MPI_Recv(&message, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			//printf("---0: Odebralem---(Nadawca = %d, TAG = %d, Clock nadawcy = %d)\n",message[0], message[1], message[2]);
			if(lamportClock < message[2])
			{
				lamportClock = message[2] + 1;
			}
			else
			{
				lamportClock += 1;
			}
			cout << lamportClock << ":" << myrank << "\tOdebralem --- (Nadawca = " << message[0] << ", TAG = " << message[1] << ", Clock nadawcy = " << message[2] << "\n";
		}
	}
	else 
	{
		MPI_Recv(&message, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		cout << lamportClock << ":" << myrank << "\tOdebralem\n";
		if(lamportClock < message[2])
		{
			lamportClock = message[2] + 1;
		}
		else
		{
			lamportClock += 1;
		}
		cout << lamportClock << ":" << myrank << "\tOdebralem z czasem\n";
		lamportClock += 1;
		message[0] = myrank;
		message[1] = 20;
		message[2] = lamportClock;
		MPI_Send(&message, 3, MPI_INT, 0, MYTAG, MPI_COMM_WORLD);
		cout << lamportClock << ":" << myrank << "\tWyslalem z czasem\n";
	}
	MPI_Finalize();
}

void *receive_loop(void * arg) {
	MPI_Status status;
	cout << lamportClock << ":" <<myrank << "\treceive loop" << "\n";
}


