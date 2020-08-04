/* 
 * Netta Paulson
 * CMSC 312
 * Assignment 3- Print Server
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/ipc.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/times.h>

#define SIZE 30
#define SNAMEBM "/buferMutex5935"
#define SNAMEGE "emptyGate3525"
#define SNAMEME "/emptyMutex3252"
#define SNAMEGF "/fullGate8945"
#define SNAMEMF "/fullMutex230"

int *buffer_index, *val_empty, *val_full, *total_jobs;
sem_t *empty_gate; 
sem_t *empty_mutex;
sem_t *full_gate; 
sem_t *full_mutex;
sem_t *buffer_mutex;
struct print_request *queue;
int jobs_consumed;

struct timeval end;

struct print_request {
	int id;
	int size;
} print_request;

void my_sem_wait_empty(){
	sem_wait(empty_gate);
	sem_wait(empty_mutex);
	*val_empty -= 1;
	if(*val_empty > 0){
		sem_post(empty_gate);
	}
	sem_post(empty_mutex);
}
void my_sem_wait_full(){
	sem_wait(full_gate);
	sem_wait(full_mutex);
	*val_full -= 1;
	if(*val_full > 0){
		sem_post(full_gate);
	}
	sem_post(full_mutex);
}

void my_sem_post_empty(){
	sem_wait(empty_mutex);
	*val_empty += 1;
	if(*val_empty == 1){
		sem_post(empty_gate);
	}
	sem_post(empty_mutex);
}

void my_sem_post_full(){
	sem_wait(full_mutex);
	*val_full += 1;
	if(*val_full == 1){
		sem_post(full_gate);
	}
	sem_post(full_mutex);
}

void *consumer(void *thread_n) {
	int thread_num = *(int *)thread_n;
	while(1){
		if(jobs_consumed == *total_jobs) {
			break;
		}
		my_sem_wait_empty();
		sem_wait(buffer_mutex);
		if (*buffer_index > 0) {
			(*buffer_index)--;
			int size = queue[*buffer_index].size;
			int id = queue[*buffer_index].id;
			jobs_consumed += 1;
			printf("Consumer %d dequeue %d, %d from buffer\n", thread_num-1,id, size);
		}
		sem_post(buffer_mutex);
		my_sem_post_full();
		sleep(rand() % (2)) + 1;
		if(jobs_consumed == *total_jobs) {
			gettimeofday(&end, NULL);
			break; 
		}

	}
	pthread_exit(0);
}

void producer_fork(int process){
	int x;
	int jobs = (rand() % (30)) + 1;
	*total_jobs += jobs;
	for(x = 0; x < jobs; x++){
		wait(rand() % (500 + 1 - 50)) + 50;
		my_sem_wait_full();	
		sem_wait(buffer_mutex);	
		int size = (rand() % (1000 + 1 - 100)) + 100;
		if(*buffer_index < 30){
			queue[(*buffer_index)].id = x;
			queue[(*buffer_index)].size = size;
			(*buffer_index)++;
			printf("Producer %d added %d to buffer\n", process, size);
		}
		sem_post(buffer_mutex);
		my_sem_post_empty();
		sleep(1);
	}
}

void ctrlCHandler(int sig_num){ 
	signal(SIGINT, ctrlCHandler); 
	fflush(stdout); 
}


int main(int argc, char* argv[]) { 
	signal(SIGINT, ctrlCHandler);
	int shmid1, shmid2, shmid3, shmid4, shmid5;
	key_t key1, key2, key3, key4, key5;

	key1 = 7754;
	key2 = 7845;
	key3 = 4823;
	key4 = 4573;
	key5 = 9345;

	if((shmid1 = shmget(key1, sizeof(struct print_request*), IPC_CREAT | 0666)) < 0 ) {
		perror("shmget print_request");
		exit(1);
	}

	if((queue = shmat(shmid1, NULL, 0)) == (struct print_request*) -1) {
		perror("shmat print_request");
		exit(1);
	}

	if((shmid2 = shmget(key2, sizeof(int*), IPC_CREAT | 0666)) < 0 ) {
		perror("shmget int");
		exit(1);
	}

	if((buffer_index = shmat(shmid2, NULL, 0)) == (int*) -1) {
		perror("shmat int");
		exit(1);
	}

	*buffer_index = 0;

	if((shmid3 = shmget(key3, sizeof(int*), IPC_CREAT | 0666)) < 0 ) {
		perror("shmget int");
		exit(1);
	}

	if((val_empty = shmat(shmid3, NULL, 0)) == (int*) -1) {
		perror("shmat int");
		exit(1);
	}

	*val_empty = 0;

	if((shmid4 = shmget(key4, sizeof(int*), IPC_CREAT | 0666)) < 0 ) {
		perror("shmget int");
		exit(1);
	}

	if((val_full = shmat(shmid4, NULL, 0)) == (int*) -1) {
		perror("shmat int");
		exit(1);
	}

	*val_full = SIZE;

	if((shmid5 = shmget(key5, sizeof(int*), IPC_CREAT | 0666)) < 0 ) {
		perror("shmget int");
		exit(1);
	}

	if((total_jobs = shmat(shmid5, NULL, 0)) == (int*) -1) {
		perror("shmat int");
		exit(1);
	}

	*total_jobs = 0;
	jobs_consumed = 0;

	sem_unlink(SNAMEGE);
	sem_unlink(SNAMEME);
	sem_unlink(SNAMEGF);
	sem_unlink(SNAMEMF);
	sem_unlink(SNAMEBM);

	empty_gate = sem_open(SNAMEGE, O_CREAT, 0644, 0);
	empty_mutex = sem_open(SNAMEME, O_CREAT, 0644, 1);
	full_gate = sem_open(SNAMEGF, O_CREAT, 0644, 1);
	full_mutex = sem_open(SNAMEMF, O_CREAT, 0644, 1);
	buffer_mutex = sem_open(SNAMEBM, O_CREAT, 0644, 1);


	int num_prod = atoi(argv[1]);
	int num_cons = atoi(argv[2]);
	int i;
	int pid[num_prod];
	struct timeval start;
	gettimeofday(&start,NULL);
	for(i = 0; i < num_prod; i++){
		pid[i] = fork();
		if(pid[i] == 0){
			empty_gate = sem_open(SNAMEGE, 0);
			empty_mutex = sem_open(SNAMEME, 0);
			full_gate = sem_open(SNAMEGF, 0); 
			full_mutex = sem_open(SNAMEMF, 0);
			buffer_mutex = sem_open(SNAMEBM, 0);
			
			srand(time(NULL)+getpid());
			producer_fork(i);
			exit(1);
		}
	}
	pthread_t thread[num_cons];
	int c;
	for(c = 0; c < num_cons; c++) {
		pthread_create(&thread[c], NULL, consumer, &c);
	}
	int w;
	for(w = 0; w < num_prod; w++){
		wait(NULL);
	}

	shmctl(shmid1, IPC_RMID, NULL);	
	shmctl(shmid2, IPC_RMID, NULL);
	shmctl(shmid3, IPC_RMID, NULL);
	shmctl(shmid4, IPC_RMID, NULL);
	shmctl(shmid5, IPC_RMID, NULL);
	shmdt(queue);
	shmdt(buffer_index);
	shmdt(val_empty);
	shmdt(val_full);
	shmdt(total_jobs);
	sem_close(empty_gate);
	sem_close(empty_mutex);
	sem_close(full_gate);
	sem_close(full_mutex);
	sem_close(buffer_mutex);

	long seconds = (end.tv_sec - start.tv_sec);
	long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
	printf ( "Execution time:   %d seconds %d micros \n", seconds, micros);
	return 0;
}
