// multithread header
// Author: Grant Novota
#ifndef MULTITHREAD_H
#define MULTITHREAD_H

// libraries
#include <pthread.h>	// threading
#include <stdlib.h>		
#include <stdio.h>
#include <errno.h>		// for errno
#include <unistd.h>		
#include <sys/types.h>     //for get id
#include <sys/syscall.h>
#include "util.h"		// for DNS lookup
#include "queue.h"		// queue for hostnames

#include <time.h>       // for getting app runtime

#define MIN_ARGUMENTS 6
#define USAGE "<# requester> <# resolver> <requester log> <resolver log> <input_file_paths ...> <output_file_path>"
#define SBUFF_SIZE 1025
#define INPUTFS "%1024s"
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define MAX_THREADS = 10
#define MAX_INPUT_FILES = 5
#define MIN_RESOLVER_THREADS 2

// push hostnames into queue
void* read_files(char* filename);

// producer thread pool
void* requester_pool(char* inputFiles);

// requester log
void requester_log(pid_t tid);

// resolver log
void resolver_log(pid_t tid);
// DNS lookup.
void* dns();

// consumer thread pool
void* resolver_pool();

int main(int argc, char* argv[]);

#endif