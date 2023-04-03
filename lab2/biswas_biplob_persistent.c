#include "amr.h"
#include <stdlib.h>
#include <pthread.h>

pthread_barrier_t barr1, barr2;
int converged = 0;

void *parallelFunc(void *param) 
{
    ThreadParams tp = *(ThreadParams *) param;
    while (!converged)
    {
        pthread_barrier_wait(&barr1);
        for (int i=0; i<tp.threadBoxCount; i++) 
        {
            tp.tempToUpdate[i + tp.firstBoxId] = getNewTemp(i + tp.firstBoxId);
        }
        pthread_barrier_wait(&barr2);
    }
    return NULL;
}


int runConvergenceLoop() 
{
    int iteration, initBoxId = 0;
    double *newTemp = (double *) malloc(boxCount * sizeof(double));
    pthread_t threads[THREAD_COUNT];
    ThreadParams threadParams[THREAD_COUNT];

    pthread_barrier_init(&barr1, NULL, THREAD_COUNT + 1);
    pthread_barrier_init(&barr2, NULL, THREAD_COUNT + 1);

    for (int tid = 0; tid < THREAD_COUNT; tid++)
    {
        threadParams[tid].firstBoxId = initBoxId;
        threadParams[tid].threadBoxCount = boxCount/THREAD_COUNT + (boxCount % THREAD_COUNT > tid ? 1 : 0);
        threadParams[tid].tempToUpdate = newTemp;
        initBoxId += threadParams[tid].threadBoxCount;
        pthread_create(&threads[tid], NULL, parallelFunc, (void *) &threadParams[tid]);
    }
    
    for (iteration = 0; !hasConverged(); iteration++)
    {
        pthread_barrier_wait(&barr1);
        pthread_barrier_destroy(&barr1);
        pthread_barrier_init(&barr1, NULL, THREAD_COUNT + 1);
        pthread_barrier_wait(&barr2);
        pthread_barrier_destroy(&barr2);
        pthread_barrier_init(&barr2, NULL, THREAD_COUNT + 1);

        for (int i = 0; i < boxCount; i++)
        {
            boxes[i].temp = newTemp[i];
        }
    }
    
    converged = 1;
    pthread_barrier_wait(&barr1);
    pthread_barrier_destroy(&barr1);
    pthread_barrier_wait(&barr2);
    pthread_barrier_destroy(&barr2);

    for (int tid = 0; tid<THREAD_COUNT; tid++) 
    {
        pthread_join(threads[tid], NULL);
    }

    free(newTemp);
    return iteration;
}