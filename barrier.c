#include <stdio.h>
#include <stdlib.h>
#include "pthread.h"


#define NoThreadsCreated 4

// barrier data structure
typedef struct {
    int maxCount; // maximum number of runners
    struct sub_barrier {
        pthread_mutex_t wait_lk;   // mutex for waiters at barrier
        pthread_cond_t wait_cv;   // waiting condition for waiters at barrier
        int runners;   // number of running threads
    } sub_barrier[NoThreadsCreated];
    struct sub_barrier *sbp;    // current sub_barrier
} barrier_t;

// barrier initialization function
int barrier_init(barrier_t *barrier, int count, void *arg) {
    // if less then 1 thread is being created then what the hell are we doing :)
    if (count < 1){
        return (-1);
    }
    barrier->maxCount = count;
    barrier->sbp = &barrier->sub_barrier[0];
    for (int i = 0; i < NoThreadsCreated; ++i) {
        struct sub_barrier *sbp = &(barrier->sub_barrier[i]);
        sbp->runners = count;
        // if initialising mutex faced error return -1
        if (pthread_mutex_init(&sbp->wait_lk, arg)){
            return (-1);
        }
        // if initialising thread's conditions faced error return -1
        if (pthread_cond_init(&sbp->wait_cv, arg)){
            return (-1);
        }
    }
    // if the initialization is completed correctly return 0
    return (0);
}

// barrier waiting function
int barrier_wait(barrier_t *bp) {
    struct sub_barrier *sbp = bp->sbp;
    // Locking threads with mutex
    pthread_mutex_lock(&sbp->wait_lk);
    // last thread to reach barrier
    if (sbp->runners == 1) {
        // check to see if the runners is 1 the number of threads are
        // not 1 if it was one then why do we bother multi threading :)
        if (bp->maxCount != 1) {
            // reset runner count (by doing this the while loop in line 62 will not be correct any more and other threads
            // will skip the while loop
            sbp->runners = bp->maxCount;
            // switch sub-barriers
            if(bp->sbp == &bp->sbp[0]){
                bp->sbp = &bp->sbp[1];
            }
            else{
                bp->sbp = &bp->sbp[0];
            }
            // wake up the waiters and tell them that we faced the last thread so you can
            pthread_cond_broadcast(&sbp->wait_cv);
        }
    } else {
        // as it's not the last thread , just decrease the runners by one
        sbp->runners--;
        // with the help of pthread condition we are waiting for the next thread and we do not let other threads
        // to pass and unlock the mutex, unless we find out that we reached the last thread
        while (sbp->runners != bp->maxCount){
            pthread_cond_wait(&sbp->wait_cv, &sbp->wait_lk);
        }
    }
    pthread_mutex_unlock(&sbp->wait_lk);
    return (0);
}

int barrier_destroy(barrier_t *bp) {
    int n;
    printf("starting to destroying barrier");
    for (int i = 0; i < NoThreadsCreated; ++i) {
        n = pthread_cond_destroy(&bp->sbp[i].wait_cv);
        if (n)
            return (n);

        n = pthread_mutex_destroy(&bp->sbp[i].wait_lk);
        if (n)
            return (n);
    }
    printf("finished to destroying barrier");
    return (0);
}

void *routine(barrier_t *ba) {
    printf("waiting at the barrier\n");
    for (int i = 0; i < NoThreadsCreated; ++i) {
        barrier_wait(ba);
        // do parallel computations
    }
    printf("We passed the barrier\n");
    return NULL;
}

int main() {
    int i;
    barrier_t bar;
    pthread_t *tid;

    // initialising barrier (note that we want also have main thread
    barrier_init(&bar, NoThreadsCreated+1, NULL);
    tid = (pthread_t *) calloc(NoThreadsCreated, sizeof(pthread_t));
    // creating threads
    for (i = 0; i < NoThreadsCreated; ++i) {
        if (pthread_create(&tid[i], NULL,
                           (void *(*)(void *)) routine,
                           &bar) != 0) {
            perror("thread_creation");
            exit(1);
        }
    }
    // doing parallel computations // critical section
    for (i = 0; i < NoThreadsCreated; i++) {
        barrier_wait(&bar);
        // do parallel computations
    }
    // joining threads
    for (i = 0; i < NoThreadsCreated; i++) {
        if(pthread_join(tid[i], NULL) != 0){
            perror("thread_join");
            exit(1);
        }
    }
//    if(barrier_destroy(&bar) != 0){
//        perror("thread_destroy");
//        exit(1);
//    }
    return 0;
}
