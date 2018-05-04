#include <stdio.h>
#include <mpi.h>
#include <pthread.h>
#include <iostream>
#include <algorithm> 
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <queue>
#define nArbiter 2
#define MYTAG 100

using namespace std;

/*
	tablica message
	0 - sender_id
	1 - tag
	2 - timestamp
	3 - clockWhenStart -> time when sender starts send request

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
};

struct queueType
{	
	int senderRank;
	int senderClock;

};

int size, myrank;
int arbiter = nArbiter;
int lamportClock = 0;
int nAgree = 0;
bool want ;


queue <queueType> myQueue;


int clockWhenStart=0;

void *receive_loop(void * arg);
void clockUpdate(int valueFromMsg);
void clockUpdate();


pthread_mutex_t	clock_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t nAgree_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t want_mutex= PTHREAD_MUTEX_INITIALIZER;

int check_N_agree(){
		pthread_mutex_lock(&nAgree_mutex);
		int myN= nAgree;
		pthread_mutex_unlock(&nAgree_mutex);
		return myN;
}

bool check_Queue(){
	pthread_mutex_lock(&queue_mutex);
	bool my= myQueue.empty();
	pthread_mutex_unlock(&queue_mutex);
	return my;
}

bool check_Want(){
	pthread_mutex_lock(&want_mutex);
	bool myWant = want;
	pthread_mutex_unlock(&want_mutex);
	if (myWant == true) return true;
	else return false;
}

int check_Lamport_Clock(){
	pthread_mutex_lock(&clock_mutex);
	int myLam = lamportClock;
	pthread_mutex_unlock(&clock_mutex);
	return myLam;
}

int main(int argc, char **argv)
{
	// Enable thread in MPI 
	int provided = 0;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);


	//Create thread
	pthread_t receive_thread;
	pthread_create(&receive_thread, NULL, receive_loop, 0);
	
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	int message[4];
	srand(myrank);
	int delay;

	while(1)
	{
		delay = rand() % 5;
		sleep(delay);
		//printf("%d:%d\t\tDelay =  %d\n", lamportClock, myrank, delay);

		pthread_mutex_lock(&want_mutex);
		want = true;
		pthread_mutex_unlock(&want_mutex);
		
		for (int i = 0; i < size; i++)
		{
			if (i != myrank)
			{
				clockUpdate();
				
				message[0] = myrank;
				message[1] = TAG_ARB_QUE;
				message[2] = check_Lamport_Clock();
				if (i == 0) clockWhenStart = message[2];
				message[3] = clockWhenStart;
				MPI_Send(&message, 4, MPI_INT, i, MYTAG, MPI_COMM_WORLD);
			
			//	printf("%d:%d\t\tWyslalem do %d\n", lamportClock, myrank, i);
				//cout << lamportClock << ":" << myrank << "\t\tWyslalem do " << i << "\n" << flush;
			}	
		}
		

	
		while (check_N_agree() < size - arbiter) 
		{
			//printf("%d:%d\t\tZa malo zgod\n", lamportClock, myrank);
			//cout << lamportClock << ":" << myrank << "\t\t Za malo zgod\n";
			sleep(1);
		}

		printf("%d:%d\t\tCHLANIE! Zgody = %d\n", check_Lamport_Clock(), myrank, nAgree);
		sleep(10);
		printf("%d:%d\t\tKONIEC CHLANIA!\n", check_Lamport_Clock(), myrank);

		pthread_mutex_lock(&want_mutex);	
		want = false;
		pthread_mutex_unlock(&want_mutex);

		pthread_mutex_lock(&nAgree_mutex);	
		nAgree = 0;
		pthread_mutex_unlock(&nAgree_mutex);
		
		

		while(!check_Queue())
		{
			clockUpdate();
			pthread_mutex_lock(&queue_mutex);
			message[0] = myrank;
			message[1] = TAG_ARB_ANS_OK;
			message[2] = check_Lamport_Clock();
			message[3] = myQueue.front().senderClock;

			MPI_Send(&message, 4, MPI_INT, myQueue.front().senderRank, MYTAG, MPI_COMM_WORLD);
			//printf("%d:%d\t\tWyslalem zgode z kolejki do %d\n", lamportClock, myrank, myQueue.front().senderRank);
			
			
			myQueue.pop();
			pthread_mutex_unlock(&queue_mutex);
		
		};

		sleep(5);
	};
		
	MPI_Finalize();
}

void *receive_loop(void * arg) {
	//printf("%d:%d\t\treceive loop\n", lamportClock, myrank);
	//cout << lamportClock << ":" <<myrank << "\t\treceive loop" << "\n" << flush;
	MPI_Status status;
	int message[4];
	while (1)
	{
		MPI_Recv(&message, 4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		clockUpdate(message[2]);
		int clockWhenStartRecv;
		
		
		switch (message[1])
		{
			
			case TAG_ARB_QUE:
				//printf("%d:%d\t\tOdebralem\n", lamportClock, myrank);
				//cout << lamportClock << ":" << myrank << "\t\tOdebralem\n" << flush;
				
				//czas kiedy nadawca arb_question zaczal pytac 
				clockWhenStartRecv=message[3];

				if (check_Want())
				{	
					clockUpdate();
					message[0] = myrank;
					message[1] = TAG_ARB_ANS_OK;
					message[2] = check_Lamport_Clock();
					message[3] = clockWhenStartRecv;
					MPI_Send(&message, 4, MPI_INT, status.MPI_SOURCE, MYTAG, MPI_COMM_WORLD);
					//printf("%d:%d\t\tWyslalem z czasem, do odbiorcy %d \n", lamportClock, myrank,status.MPI_SOURCE);
				}
				else
				{
					//printf("HERE\t\t%d\n", status.MPI_SOURCE);
					//if(message[2] < lamportClock) status.MPI_SOURCE -> nadawca 
					if(clockWhenStartRecv<clockWhenStart){
						clockUpdate();
						message[0] = myrank;
						message[1] = TAG_ARB_ANS_OK;
						message[2] = check_Lamport_Clock();
						message[3] = clockWhenStartRecv;
						MPI_Send(&message, 4, MPI_INT, status.MPI_SOURCE, MYTAG, MPI_COMM_WORLD);
//						printf("%d:%d\t\tWyslalem z czasem\n", lamportClock, myrank);
						//printf("%d:%d\t\tWyslalem z czasem, do odbiorcy %d \n", lamportClock, myrank,status.MPI_SOURCE);
					}

					//jesli remis to priorytet ma ten z mniejszym rankiem
					else if (clockWhenStartRecv==clockWhenStart){
						if( status.MPI_SOURCE <  myrank){
						clockUpdate();
						message[0] = myrank;
						message[1] = TAG_ARB_ANS_OK;
						message[2] = check_Lamport_Clock();
						message[3] = clockWhenStartRecv;
						MPI_Send(&message, 4, MPI_INT, status.MPI_SOURCE, MYTAG, MPI_COMM_WORLD);
						//printf("%d:%d\t\tWyslalem z czasem\n", lamportClock, myrank);
						//printf("%d:%d\t\tWyslalem z czasem, do odbiorcy %d \n", lamportClock, myrank,status.MPI_SOURCE);
						}
					}
					else
					{
						//Wstrzymaj
						
						pthread_mutex_lock(&queue_mutex);
						queueType waitProcess = {status.MPI_SOURCE,clockWhenStartRecv};
 						myQueue.push(waitProcess);
						pthread_mutex_unlock(&queue_mutex);
						//printf("%d:%d\t\tOdlozylem na kolejke, odlozony proces %d , jego czas %d\n", lamportClock, myrank,status.MPI_SOURCE,clockWhenStartRecv);
					}
				}
				//cout << lamportClock << ":" << myrank << "\t\tWyslalem z czasem\n" << flush;
				break;

			case TAG_ARB_ANS_OK:
				//czas mojego zadania na ktore dostaje zgode
				clockWhenStartRecv=message[3];
				
				if ( clockWhenStartRecv==clockWhenStart){
				clockUpdate();
				pthread_mutex_lock(&nAgree_mutex);
				nAgree += 1;
				pthread_mutex_unlock(&nAgree_mutex);
				//printf("%d:%d\t\tOdebralem zgode(%d/%d) , nadawca %d\n", lamportClock, myrank, nAgree, size - 1,status.MPI_SOURCE);
			}
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

