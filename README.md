# Multithreaded Synchronization System (C, POSIX Threads)

This project implements a multithreaded system in C using POSIX threads and semaphores to simulate thread scheduling and synchronization.

## Features

- Thread creation with varying start times
- Critical section control (only one thread executes at a time)
- Ordering constraints based on thread ID patterns
- Deadlock prevention
- Starvation handling logic

## Technologies

- C
- POSIX Threads (pthreads)
- Semaphores
- Linux

## How to Run

Compile:
make

Run:
./Assignment_3 sample_input.txt

## Example Output

The program outputs thread execution logs showing thread start, execution in critical sections, and completion.

## Concepts Demonstrated

- Multithreading and concurrency
- Synchronization using semaphores
- Deadlock avoidance
- Starvation handling
