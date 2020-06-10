// multithread lookup script
// Author: Grant Novota
#include "multi-lookup.h"
//#include <sys/types.h>     //for get id
//condition variables
pthread_cond_t queue_is_full;		// signal/wait on queue full.
pthread_cond_t queue_is_empty;		// signal/wait on queue empty. 
//mutex locks
pthread_mutex_t queue_lock;		// protects queue
pthread_mutex_t requester_lock;	// protects counter in readFiles section
pthread_mutex_t resolver_lock;	// protects output file in dns section

queue request_q;				// instance of the queue.
int files_completed = 0;		// to track the number of files added to the queue
int num_input_files;			// number of input files passed as arguments to executable
char* output_file;			    // output file for dns lookup
int max_threads;				
int max_requester_threads;      // user inputed max requester threads
int max_resolver_threads;		// user inputed max resolver threads
char* requester_log_path;
char* resolver_log_path;

pid_t requester_ids [10];
int requester_counts [10];
pid_t resolver_ids [10];
int resolver_counts [10];

void* requester_pool(char* input_files) {
	char** input_file_names = (char**) input_files; 
	pthread_t requester_threads[num_input_files];
	// create requester threads until number of input files done 
	for (int i=0; i < num_input_files; i++) {
		char* file_name = input_file_names[i];
		pthread_create(&requester_threads[i], NULL, (void*) read_files, (void*) file_name);
		// wait until requester threads finished
		pthread_join(requester_threads[i], NULL);
	}
	for (int i=0; i < num_input_files; i++) {
		// signal requester threads that the queue is empty to begin work
		pthread_cond_signal(&queue_is_empty);
	}
	return NULL;
}

void* resolver_pool(FILE* output_file_path) {
	pthread_t resolver_threads[max_resolver_threads];
	// create resolver threads to perform dns lookup until max
	for (int i=0; i < max_resolver_threads; i++) {
		pthread_create(&resolver_threads[i], NULL, dns, output_file_path);
		// wait until resolver threads have finished. 
		pthread_join(resolver_threads[i], NULL);
	}
	return NULL;
}

void requester_log(pid_t tid) {
	for (int i=0; i < 10; i++) {
		// printf("%d,%d\n", requester_ids[i], requester_counts[i]);
		if(requester_ids[i] == tid) {
			requester_counts[i]++;
			return;
		}
	}
	for (int i=0; i < 10; i++) {
		// printf("%d,%d\n", requester_ids[i], requester_counts[i]);
		if(requester_ids[i] == 0) {
			requester_ids[i] = tid;
			requester_counts[i] = 1;
			return;
		}
	}
	return;
}

void resolver_log(pid_t tid) {
	for (int i=0; i < 10; i++) {
		// printf("%d,%d\n", requester_ids[i], requester_counts[i]);
		if(resolver_ids[i] == tid) {
			resolver_counts[i]++;
			return;
		}
	}
	for (int i=0; i < 10; i++) {
		// printf("%d,%d\n", requester_ids[i], requester_counts[i]);
		if(resolver_ids[i] == 0) {
			resolver_ids[i] = tid;
			resolver_counts[i] = 1;
			return;
		}
	}
	return;
}

void* read_files(char* filename) {
	char file_path[SBUFF_SIZE] = "input/";
	strcat(file_path, filename);
	
	// get thread id
	pid_t tid = syscall(SYS_gettid);
	// pthread_t ptid = pthread_self();
    // uint64_t threadId = 0;
    // memcpy(&threadId, &ptid, std::min(sizeof(threadId), sizeof(ptid)));
	// perror("calling requstorlog.\n");
	requester_log(tid);

	// open input file. 
	FILE* input_file_path = fopen(file_path, "r");
	// bogus input file path
	if(!input_file_path){
		// print error to stderror
		perror(file_path);
		perror("Error: unable to open input file.\n");
		// lock counter variable to increment, if not done results in infinite loop in dns function
		pthread_mutex_lock(&requester_lock);
		files_completed++;
		pthread_mutex_unlock(&requester_lock);
		return NULL;
	}
	char hostname[SBUFF_SIZE];
	// scan in file and get hostnames
	while(fscanf(input_file_path, INPUTFS, hostname) > 0) {
		//check to see if queue is full
		//protect queue access with mutex lock while doing this
		pthread_mutex_lock(&queue_lock);
		while(queue_full(&request_q)) { // waits
			// automatically unlocking queue_lock and reaquiring it once signaled
			pthread_cond_wait(&queue_is_full, &queue_lock);
		}
		// push hostname onto queue
		enqueue(&request_q, strdup(hostname));
		// signal on condition queue_is_empty to let consumer/resolver threads know that something is in the queue
		pthread_cond_signal(&queue_is_empty);
		// unlock queue access
		pthread_mutex_unlock(&queue_lock);
	}
	// lock access to counter to increment number of files added to queue
	pthread_mutex_lock(&requester_lock);
	// increment counter in critical section
	files_completed++;
	// unlock access to counter
	pthread_mutex_unlock(&requester_lock);
	
	fclose(input_file_path);
	return NULL;
}

void* dns(FILE* output_file_path) {
	pid_t tid = syscall(SYS_gettid);
	resolver_log(tid);

	while(1){
		// lock queue mutex to access
		pthread_mutex_lock(&queue_lock);
		// check to see if queue is empty
		while(queue_empty(&request_q)){
			// ensure that we lock files completed counter variable to access
			pthread_mutex_lock(&requester_lock);
			int finished = 0;
			// determine if we are at the end of the queue 
			if(files_completed == num_input_files) finished = 1;
			// unlock counter variable
			pthread_mutex_unlock(&requester_lock);
			// finished, unlock queue 
			if (finished){
				// unlock queue mutex
				pthread_mutex_unlock(&queue_lock);
				return NULL;
			}
			// if the queue is empty but we still have files to consume that means that consumer threads must wait
			pthread_cond_wait(&queue_is_empty, &queue_lock);
		}
		// dequeue one hostname
		char* hostname = (char*) dequeue(&request_q);
		// let producer processes know that there is space in the queue
		pthread_cond_signal(&queue_is_full);
		// allocate IP address
		char first_ip[MAX_IP_LENGTH];
		// if we are unable to perform dns lookup on hostname
		// Error bogus hostname
		if (dnslookup(hostname, first_ip, sizeof(first_ip))==UTIL_FAILURE){
			fprintf(stderr, "Error: DNS lookup failure on hostname: %s\n", hostname);
			strncpy(first_ip, "", sizeof(first_ip));
		}
		// lock the output file using resolver mutex lock
		pthread_mutex_lock(&resolver_lock);
		// add in the hostname with its IP addres
		fprintf(output_file_path, "%s,%s\n", hostname, first_ip);
		// unlock resolver mutex to allow access to output file 
		pthread_mutex_unlock(&resolver_lock);
		// free space for next hostname
		free(hostname);
		// unlock queue access
		pthread_mutex_unlock(&queue_lock);
	}
	return NULL;
}

int main(int argc, char* argv[]){
	// to record runtime
	clock_t begin = clock();
	
	files_completed = 0;
	max_requester_threads = strtol(argv[1], NULL, 10);
	printf("Creating %d requester threads\n", max_requester_threads);
	max_resolver_threads = strtol(argv[2], NULL, 10);
	printf("Creating %d resolver threads\n", max_resolver_threads);
	
	requester_log_path = argv[3];
	resolver_log_path = argv[4];
	num_input_files = argc-6;
	// output file is the last argument
	output_file = argv[argc-1];
	char* input_files[num_input_files];

	// initialize queue
	initialize_queue(&request_q, MAXQUEUESIZE);
	// init condition variables to signal/wait on queue full or empty
	pthread_cond_init(&queue_is_full, NULL);
	pthread_cond_init(&queue_is_full, NULL);
	// init mutex locks
	pthread_mutex_init(&queue_lock, NULL);
	pthread_mutex_init(&requester_lock, NULL);
	pthread_mutex_init(&resolver_lock, NULL);	

	// validate args
	if (argc < MIN_ARGUMENTS) {
		fprintf(stderr, "Not enough arguments: %d", (argc-1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return(EXIT_FAILURE);
	}

	// create array of input files
	// starts as the 6th argument
	for(int i=0; i < num_input_files; i++) {
		input_files[i] = argv[i+5];
	}
	
	//open output file.
	FILE* output_file_path = fopen(output_file, "w");
	// bogus output file. 
	if(!output_file_path) {
	    perror("Error: unable to open output file");
	    exit(EXIT_FAILURE);
	}

	// declare requester ID thread, used to initialize requester thread pool
	pthread_t requester_ID;
	// requester = producer, startup for requester pool.
	int producer = pthread_create(&requester_ID, NULL, (void*) requester_pool, input_files);
	if (producer != 0) {
		errno = producer;
		perror("Error: pthread_create requester");
		exit(EXIT_FAILURE);
	}

	// declare resolver ID thread, used to initialize resolver thread pool
	pthread_t resolver_ID;
	// resolver = consumer, startup for resolver pool.
	// pthread_create(pthread_t *threadName, const pthread_attr * attr, void*(*start_routine), void* arg)
	int consumer = pthread_create(&resolver_ID, NULL, (void*) resolver_pool, output_file_path);
	if (consumer != 0) {
		errno = consumer;
		perror("Error: pthread_create resolver");
		exit(EXIT_FAILURE);
	}
	
	// wait for threads to finish using pthread_join
	// pthread_join suspends execution of calling thread ie(resolver_ID/requester_ID) until execution is complete
	// until threads in resolver pool have completed their tasks
	pthread_join(requester_ID, NULL);
	pthread_join(resolver_ID, NULL);

	// close output file. 
	fclose(output_file_path);
	// free queue memory
	queue_release(&request_q);
	// destroy mutex locks
	pthread_mutex_destroy(&resolver_lock);
	pthread_mutex_destroy(&queue_lock);
	pthread_mutex_destroy(&requester_lock);
	// destroy condition variables. 
	pthread_cond_destroy(&queue_is_empty);
	pthread_cond_destroy(&queue_is_full);

	FILE* requester_log_file = fopen(requester_log_path, "w");
	for (int i=0; i < 10; i++) {
		if (requester_ids[i] == 0) {
			break;
		}
		fprintf(requester_log_file, "%d,%d\n", requester_ids[i], requester_counts[i]);
	}
	fclose(requester_log_file);

	FILE* resolver_log_file = fopen(resolver_log_path, "w");
	for (int i=0; i < 10; i++) {
		if (resolver_ids[i] == 0) {
			break;
		}
		fprintf(resolver_log_file, "%d,%d\n", resolver_ids[i], resolver_counts[i]);
	}
	fclose(resolver_log_file);

	// record runtime
	clock_t end = clock();
	double elapsed = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("elapsed: %f\n", elapsed);

	 
	pthread_exit(NULL);
	
	
	return EXIT_SUCCESS;
}