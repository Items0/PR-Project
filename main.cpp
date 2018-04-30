#include <stdio.h>
#include <mpi.h>
#include <pthread.h>
#include <iostream>
#include <algorithm> 
#include <unistd.h>
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

enum tags
{
    TAG_ARB_QUE = 10, 
    TAG_ARB_ANS_OK = 20,
    TAG_ARB_ANS_NO = 30
};

int size, myrank;
int arbiter = nArbiter;
int lamportClock = 0;
int nAgree = 0;

void *receive_loop(void * arg);
void clockUpdate(int valueFromMsg);
void clockUpdate();

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
	//MPI_Status status;
	while(1)
	{
		for (int i = 0; i < size; i++)
		{
			if (i != myrank)
			{
				clockUpdate();
				message[0] = myrank;
				message[1] = TAG_ARB_QUE;
				message[2] = lamportClock;
				MPI_Send(&message, 3, MPI_INT, i, MYTAG, MPI_COMM_WORLD);
				printf("%d:%d\t\tWyslalem do %d \n", lamportClock, myrank, i);
				//cout << lamportClock << ":" << myrank << "\t\tWyslalem do " << i << "\n" << flush;
			}	
		}
			
		while (nAgree < size - 1) 
		{
			printf("%d:%d\t\tZa malo zgod\n", lamportClock, myrank);
			//cout << lamportClock << ":" << myrank << "\t\t Za malo zgod\n";
			sleep(1);
		}
		sleep(5);
		nAgree = 0;
	};
		
	MPI_Finalize();
}

void *receive_loop(void * arg) {
	printf("%d:%d\t\treceive loop\n", lamportClock, myrank);
	//cout << lamportClock << ":" <<myrank << "\t\treceive loop" << "\n" << flush;
	MPI_Status status;
	int message[3];
	while (1)
	{
		MPI_Recv(&message, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		clockUpdate(message[2]);
		switch (message[1])
		{
			case TAG_ARB_QUE:
				printf("%d:%d\t\tOdebralem\n", lamportClock, myrank);
				//cout << lamportClock << ":" << myrank << "\t\tOdebralem\n" << flush;
				clockUpdate(message[2]);
				message[0] = myrank;
				message[1] = TAG_ARB_ANS_OK;
				message[2] = lamportClock;
				MPI_Send(&message, 3, MPI_INT, status.MPI_SOURCE, MYTAG, MPI_COMM_WORLD);
				printf("%d:%d\t\tWyslalem z czasem\n", lamportClock, myrank);
				//cout << lamportClock << ":" << myrank << "\t\tWyslalem z czasem\n" << flush;
				break;

			case TAG_ARB_ANS_OK:
				clockUpdate();
				nAgree += 1;
				printf("%d:%d\t\tOdebralem zgode(%d/%d)\n", lamportClock, myrank, nAgree, size - 1);
				//cout << lamportClock << ":" << myrank << "\t\tOdebralem zgode ("<< nAgree << "/" << size - 1 << ")\n" << flush;
				break;
		}
	}
	//cout << lamportClock << ":" <<myrank << "\treceive loop" << "\n";
}

void clockUpdate(int valueFromMsg) {
	pthread_mutex_lock(&clock_mutex);
	lamportClock = max(lamportClock, valueFromMsg) + 1;
	pthread_mutex_unlock(&clock_mutex);
}

void clockUpdate() {
	pthread_mutex_lock(&clock_mutex);
	lamportClock += 1;
	pthread_mutex_unlock(&clock_mutex);
}

