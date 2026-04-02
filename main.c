#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <semaphore.h>

void logStart(char* tID);//function to log that a new thread is started
void logFinish(char* tID);//function to log that a thread has finished its time

void startClock();//function to start program clock
long getCurrentTime();//function to check current time since clock was started
time_t programClock;//the global timer/clock for the program

typedef struct thread //represents a single thread, you can add more members if required
{
    char tid[4];//id of the thread as read from file, set in readFile() for you
    unsigned int startTime;//start time of thread as read from file, set in readFile() for you
    int state;//you can use it as per your desire
    pthread_t handle;//you can use it as per your desire
    int retVal;//you can use it as per your desire

    // Added for Assignment 3 synchronization
    sem_t gate;      // per-thread semaphore to allow entry to critical section
    int waiting;     // 1 if waiting to be scheduled for critical section
} Thread;

// Global synchronization state
static sem_t g_mutex;            // protects shared state
static Thread *g_threads = NULL;
static int g_threadCount = 0;

static unsigned int g_earliestStart = 0;
static unsigned int g_lastStart = 0;

static int g_inCS = 0;           // 1 if a thread is currently in critical section
static int g_firstChosen = 0;    // 1 after first critical-section entry has been chosen
static int g_lastParity = -1;    // 0 even, 1 odd, -1 none yet

static int g_remainingEven = 0;  // not-yet-finished threads with even y
static int g_remainingOdd  = 0;  // not-yet-finished threads with odd y

static int tid_parity(const Thread *t)
{
    // Thread IDs are txy; y is last digit (0-9)
    char c = t->tid[2];
    if (c < '0' || c > '9') {
        return 0;
    }
    return ((c - '0') % 2);
}

static Thread* pick_next_waiting(int desiredParity, int parityMatters, int startTimeFilterOnEarliest)
{
    Thread *best = NULL;

    for (int i = 0; i < g_threadCount; i++) {
        Thread *cur = &g_threads[i];

        if (!cur->waiting) {
            continue;
        }

        if (startTimeFilterOnEarliest && cur->startTime != g_earliestStart) {
            continue;
        }

        if (parityMatters && tid_parity(cur) != desiredParity) {
            continue;
        }

        if (best == NULL) {
            best = cur;
        } else {
            // Prefer earlier creation time (smaller startTime); tie-break by ID
            if (cur->startTime < best->startTime) {
                best = cur;
            } else if (cur->startTime == best->startTime && strcmp(cur->tid, best->tid) < 0) {
                best = cur;
            }
        }
    }

    return best;
}

static void schedule_next_locked(void)
{
    // g_mutex must already be held
    if (g_inCS) {
        return;
    }

    if (!g_firstChosen) {
        // First thread: earliest creation time
        Thread *first = pick_next_waiting(0, 0, 1);
        if (first != NULL) {
            first->waiting = 0;
            g_inCS = 1;
            g_firstChosen = 1;
            sem_post(&first->gate);
        }
        return;
    }

    // After first: alternate parity based on last digit y
    int neededParity = 1 - g_lastParity;

    // Starvation control: only after last thread has started, and needed parity no longer exists
    int now = (int)getCurrentTime();
    int neededRemaining = (neededParity == 0) ? g_remainingEven : g_remainingOdd;
    int relaxParity = (now >= (int)g_lastStart && neededRemaining == 0);

    Thread *next = NULL;

    if (!relaxParity) {
        next = pick_next_waiting(neededParity, 1, 0);
        if (next == NULL) {
            return;
        }
    } else {
        next = pick_next_waiting(0, 0, 0);
        if (next == NULL) {
            return;
        }
    }

    next->waiting = 0;
    g_inCS = 1;
    sem_post(&next->gate);
}

void* threadRun(void* t)//implement this function in a suitable way
{
    Thread *self = (Thread*)t;

    logStart(self->tid);

    //your entry section synchronization logic will appear here
    sem_wait(&g_mutex);
    self->waiting = 1;
    schedule_next_locked();
    sem_post(&g_mutex);

    sem_wait(&self->gate);

    //critical section starts here, it has only the following printf statement
    printf("[%ld] Thread %s is in its critical section\n", getCurrentTime(), self->tid);
    //critical section ends here

    //your exit section synchronization logic will appear here
    sem_wait(&g_mutex);

    g_lastParity = tid_parity(self);

    if (g_lastParity == 0) {
        if (g_remainingEven > 0) {
            g_remainingEven--;
        }
    } else {
        if (g_remainingOdd > 0) {
            g_remainingOdd--;
        }
    }

    g_inCS = 0;
    schedule_next_locked();
    sem_post(&g_mutex);


    logFinish(self->tid);
    return NULL;
}

int readFile(char* fileName, Thread** threads)
{
    if (fileName == NULL || threads == NULL) {
        return -1;
    }

    FILE *fp = fopen(fileName, "r");
    if (fp == NULL) {
        return -1;
    }

    int capacity = 8;
    int count = 0;
    Thread *arr = (Thread*)malloc(sizeof(Thread) * capacity);
    if (arr == NULL) {
        fclose(fp);
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Strip Windows CRLF
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[len - 1] = '\0';
            len--;
        }

        if (len == 0) {
            continue;
        }

        char *semi = strchr(line, ';');
        if (semi == NULL) {
            continue;
        }

        *semi = '\0';
        char *idPart = line;
        char *timePart = semi + 1;

        if (strlen(idPart) != 3) {
            continue;
        }

        unsigned long st = strtoul(timePart, NULL, 10);

        if (count == capacity) {
            capacity *= 2;
            Thread *tmp = (Thread*)realloc(arr, sizeof(Thread) * capacity);
            if (tmp == NULL) {
                free(arr);
                fclose(fp);
                return -1;
            }
            arr = tmp;
        }

        memset(&arr[count], 0, sizeof(Thread));
        strncpy(arr[count].tid, idPart, 3);
        arr[count].tid[3] = '\0';
        arr[count].startTime = (unsigned int)st;
        arr[count].state = 0;
        arr[count].retVal = 0;
        arr[count].waiting = 0;
        count++;
    }

    fclose(fp);

    *threads = arr;
    return count;
}

int main(int argc, char *argv[])
{
    Thread* threads = NULL;
    int threadCount = -1;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return -1;
    }

    threadCount = readFile(argv[1], &threads);
    if (threadCount <= 0 || threads == NULL) {
        fprintf(stderr, "Error: could not read input file.\n");
        return -1;
    }

    g_threads = threads;
    g_threadCount = threadCount;

    g_earliestStart = threads[0].startTime;
    g_lastStart = threads[0].startTime;
    g_remainingEven = 0;
    g_remainingOdd = 0;

    for (int i = 0; i < threadCount; i++) {
        if (threads[i].startTime < g_earliestStart) {
            g_earliestStart = threads[i].startTime;
        }
        if (threads[i].startTime > g_lastStart) {
            g_lastStart = threads[i].startTime;
        }
        if (tid_parity(&threads[i]) == 0) {
            g_remainingEven++;
        } else {
            g_remainingOdd++;
        }
    }

    if (sem_init(&g_mutex, 0, 1) != 0) {
        fprintf(stderr, "Error: sem_init failed.\n");
        free(threads);
        return -1;
    }

    for (int i = 0; i < threadCount; i++) {
        if (sem_init(&threads[i].gate, 0, 0) != 0) {
            fprintf(stderr, "Error: sem_init failed.\n");
            for (int j = 0; j < i; j++) {
                sem_destroy(&threads[j].gate);
            }
            sem_destroy(&g_mutex);
            free(threads);
            return -1;
        }
    }

    startClock();

    // Create threads exactly at their startTime values (Lab 8 emphasis)
    int *order = (int*)malloc(sizeof(int) * threadCount);
    if (order == NULL) {
        fprintf(stderr, "Error: malloc failed.\n");
        for (int i = 0; i < threadCount; i++) {
            sem_destroy(&threads[i].gate);
        }
        sem_destroy(&g_mutex);
        free(threads);
        return -1;
    }

    for (int i = 0; i < threadCount; i++) {
        order[i] = i;
    }

    // Sort indices by (startTime, tid) using a simple selection sort (portable, no qsort_r)
    for (int i = 0; i < threadCount - 1; i++) {
        int best = i;

        for (int j = i + 1; j < threadCount; j++) {
            int ib = order[best];
            int ij = order[j];

            if (threads[ij].startTime < threads[ib].startTime) {
                best = j;
            } else if (threads[ij].startTime == threads[ib].startTime &&
                       strcmp(threads[ij].tid, threads[ib].tid) < 0) {
                best = j;
            }
        }

        int tmp = order[i];
        order[i] = order[best];
        order[best] = tmp;
    }

    int created = 0;
    int pos = 0;

    while (pos < threadCount) {
        unsigned int targetTime = threads[order[pos]].startTime;

        while ((unsigned int)getCurrentTime() < targetTime) {
            usleep(1000);
        }

        // Create all threads that start at this target time
        while (pos < threadCount && threads[order[pos]].startTime == targetTime) {
            int idx = order[pos];
            if (pthread_create(&threads[idx].handle, NULL, threadRun, &threads[idx]) != 0) {
                fprintf(stderr, "Error: pthread_create failed.\n");
                for (int k = 0; k < created; k++) {
                    pthread_join(threads[order[k]].handle, NULL);
                }
                free(order);
                for (int k = 0; k < threadCount; k++) {
                    sem_destroy(&threads[k].gate);
                }
                sem_destroy(&g_mutex);
                free(threads);
                return -1;
            }
            created++;
            pos++;
        }
    }

    for (int i = 0; i < threadCount; i++) {
        pthread_join(threads[order[i]].handle, NULL);
    }

    free(order);

    for (int i = 0; i < threadCount; i++) {
        sem_destroy(&threads[i].gate);
    }
    sem_destroy(&g_mutex);

    free(threads);
    return threadCount;
}

void logStart(char* tID)
{
    printf("[%ld] New Thread with ID %s is started.\n", getCurrentTime(), tID);
}

void logFinish(char* tID)
{
    printf("[%ld] Thread with ID %s is finished.\n", getCurrentTime(), tID);
}

void startClock() //do not change this method
{
    programClock = time(NULL);
}

long getCurrentTime() //invoke this method whenever you want to check how much time units passed
//since you invoked startClock()
{
    time_t now;
    now = time(NULL);
    return now-programClock;
}