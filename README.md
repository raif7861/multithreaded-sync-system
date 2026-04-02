# Multithreaded Synchronization System (C, POSIX Threads)

This project implements a multithreaded system in C using POSIX threads and semaphores to simulate thread scheduling, synchronization, and controlled access to critical sections.

## Features

- Thread creation with varying start times  
- Mutual exclusion using semaphores (critical section control)  
- Controlled execution ordering across threads  
- Deadlock prevention mechanisms  
- Starvation handling logic  

## Technologies

- C  
- POSIX Threads (pthreads)  
- Semaphores  
- GCC (Linux)

## How to Run

### Requirements
- GCC compiler  
- Linux environment (recommended)

### Compile
make

### Run
./Assignment_3 sample_input.txt

### Optional (Save Output)
./Assignment_3 sample_input.txt > output.txt

## Environment Note

This project is designed for a Linux environment (e.g., university servers or Docker containers).

macOS users may encounter issues with POSIX semaphore functions (sem_init, sem_destroy) due to deprecation in the default macOS toolchain.

## Example Output

The program generates execution logs that demonstrate:
- Thread creation  
- Entry into critical sections  
- Controlled execution order  
- Thread completion  

## Concepts Demonstrated

- Multithreading and concurrency  
- Synchronization using semaphores  
- Deadlock avoidance  
- Starvation handling  
- Low-level systems programming  
