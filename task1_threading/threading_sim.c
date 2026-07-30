// ST5004CEM - Task 1: Process Management and Threading
// threading_sim.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_WORKER_THREADS 4
#define NUM_SCHED_TASKS    5
#define TIME_QUANTUM       2

// ---------- Part A: Shared Balance + Mutex ----------

static long shared_balance = 1000;
static pthread_mutex_t balance_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int  thread_id;
    long amount;
    int  iterations;
} worker_arg_t;

void *worker_thread(void *arg) {
    worker_arg_t *w = (worker_arg_t *)arg;

    for (int i = 0; i < w->iterations; i++) {
        pthread_mutex_lock(&balance_lock);

        long old_balance = shared_balance;
        usleep(500);
        shared_balance = old_balance + w->amount;

        printf("[Thread %d] balance %ld -> %ld (delta %+ld)\n",
               w->thread_id, old_balance, shared_balance, w->amount);

        pthread_mutex_unlock(&balance_lock);
        usleep(100);
    }
    return NULL;
}

// ---------- Part B: Round-Robin Scheduler ----------

typedef struct {
    int id;
    int burst_remaining;
    int arrival;
    int completion;
} rr_task_t;

void round_robin_scheduler(rr_task_t tasks[], int n, int quantum) {
    int *remaining = malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) remaining[i] = tasks[i].burst_remaining;

    int time = 0, done = 0;
    printf("\n--- Round Robin Scheduler (quantum = %d) ---\n", quantum);

    while (done < n) {
        int progressed = 0;
        for (int i = 0; i < n; i++) {
            if (remaining[i] <= 0) continue;
            progressed = 1;
            int slice = remaining[i] < quantum ? remaining[i] : quantum;
            printf("t=%2d : Task %d runs for %d tick(s) (remaining before: %d)\n",
                   time, tasks[i].id, slice, remaining[i]);
            time += slice;
            remaining[i] -= slice;
            if (remaining[i] == 0) {
                tasks[i].completion = time;
                done++;
                printf("      Task %d completed at t=%d\n", tasks[i].id, time);
            }
        }
        if (!progressed) break;
    }

    printf("\nTask  Burst  Completion  Turnaround\n");
    for (int i = 0; i < n; i++) {
        int turnaround = tasks[i].completion - tasks[i].arrival;
        printf(" %3d   %4d    %6d      %6d\n",
               tasks[i].id, tasks[i].burst_remaining, tasks[i].completion, turnaround);
    }
    free(remaining);
}

// ---------- Part C: Deadlock Prevention via Lock Ordering ----------

static pthread_mutex_t lock_A = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lock_B = PTHREAD_MUTEX_INITIALIZER;

void *safe_thread_1(void *arg) {
    (void)arg;
    pthread_mutex_lock(&lock_A);
    printf("[Safe-1] locked A\n");
    usleep(1000);
    pthread_mutex_lock(&lock_B);
    printf("[Safe-1] locked B -- critical section -- releasing both\n");
    pthread_mutex_unlock(&lock_B);
    pthread_mutex_unlock(&lock_A);
    return NULL;
}

void *safe_thread_2(void *arg) {
    (void)arg;
    pthread_mutex_lock(&lock_A);
    printf("[Safe-2] locked A\n");
    usleep(1000);
    pthread_mutex_lock(&lock_B);
    printf("[Safe-2] locked B -- critical section -- releasing both\n");
    pthread_mutex_unlock(&lock_B);
    pthread_mutex_unlock(&lock_A);
    return NULL;
}

// ---------- Main ----------

int main(void) {
    printf("=========================================================\n");
    printf(" PART A: Shared balance protected by mutex (%d threads)\n", NUM_WORKER_THREADS);
    printf("=========================================================\n");

    pthread_t workers[NUM_WORKER_THREADS];
    worker_arg_t args[NUM_WORKER_THREADS] = {
        {1,  200, 3},
        {2, -150, 3},
        {3,  100, 3},
        {4,  -50, 3},
    };

    for (int i = 0; i < NUM_WORKER_THREADS; i++)
        pthread_create(&workers[i], NULL, worker_thread, &args[i]);
    for (int i = 0; i < NUM_WORKER_THREADS; i++)
        pthread_join(workers[i], NULL);

    long expected = 1000;
    for (int i = 0; i < NUM_WORKER_THREADS; i++)
        expected += (long)args[i].amount * args[i].iterations;
    printf("Final balance: %ld (expected: %ld)\n", shared_balance, expected);

    printf("\n=========================================================\n");
    printf(" PART B: Round-robin scheduling simulation\n");
    printf("=========================================================\n");
    rr_task_t tasks[NUM_SCHED_TASKS] = {
        {1, 7, 0, 0},
        {2, 4, 0, 0},
        {3, 9, 0, 0},
        {4, 3, 0, 0},
        {5, 6, 0, 0},
    };
    round_robin_scheduler(tasks, NUM_SCHED_TASKS, TIME_QUANTUM);

    printf("\n=========================================================\n");
    printf(" PART C: Deadlock prevention via consistent lock ordering\n");
    printf("=========================================================\n");
    pthread_t t1, t2;
    pthread_create(&t1, NULL, safe_thread_1, NULL);
    pthread_create(&t2, NULL, safe_thread_2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("\nNo deadlock occurred.\n");

    return 0;
}
