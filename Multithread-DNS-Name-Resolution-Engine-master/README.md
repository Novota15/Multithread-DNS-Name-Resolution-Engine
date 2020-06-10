# Multithread-DNS-Name-Resolution-Engine

### Grant Novota

grno9650@colorado.edu

A multi-thread application that resolves domain names to IP addresses, similar to the operation performed each time a new website is accessed in a web browser.

## Contents

```
Multithread-DNS-Name-Resolution-Engine/                 
├── multi-lookup.c                # main program
├── Makefile                      # defines objects and flags
├── queue.c                       # contains functions for utilizing a circular queue data structure
├── util.c                        # contains dns lookup function
├── multi-lookup.h                # variable and function declarations for multi-lookup
├── queue.h                       # variable and function declarations for queue
├── util.h                        # variable and function declarations for util
├── serviced.txt                  # each requester records the number of files it serviced
├── resolverlog.txt               # each resolver records the number of files it serviced
├── results.txt                   # list of names mapped to their ip addresses
├── results-ref.txt               # sample output of the IPs for the hostnames from the names1.txt file
├── performance.py                # plotting performance metrics
├── performance.txt               # testing results
└── input/
    ├── names1.txt
    ├── names2.txt
    └── ...
```

## Setup

To build the executable run:

`$ gcc -pthread -o multi-lookup multi-lookup.c queue.c util.c`

## Usage

`$ ./multi-lookup <# requester> <# resolver> <requester log> <resolver log> <input_file_names ...> <output_file_path>`