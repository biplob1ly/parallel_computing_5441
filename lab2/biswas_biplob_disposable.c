#include "amr.h"
#include <stdlib.h>
#include <pthread.h>

void *parallelFunc(void *param) 
{
    ThreadParams tp = *(ThreadParams *) param;
    for (int i=0; i<tp.threadBoxCount; i++) {
        tp.tempToUpdate[i + tp.firstBoxId] = getNewTemp(i + tp.firstBoxId);
    }

    return NULL;
}


int runConvergenceLoop() 
{
    int iteration;
    double *newTemp = (double *) malloc(boxCount * sizeof(double));
    
    for(iteration=0; !hasConverged(); iteration++) 
    {
        pthread_t threads[THREAD_COUNT];
        ThreadParams threadParams[THREAD_COUNT];
        int initBoxId = 0;

        for (int tid = 0; tid < THREAD_COUNT; tid++)
        {
            threadParams[tid].firstBoxId = initBoxId;
            threadParams[tid].threadBoxCount = boxCount/THREAD_COUNT + (boxCount % THREAD_COUNT > tid ? 1 : 0);
            threadParams[tid].tempToUpdate = newTemp;
            initBoxId += threadParams[tid].threadBoxCount;
            pthread_create(&threads[tid], NULL, parallelFunc, (void *) &threadParams[tid]);
        }
        
        for (int tid = 0; tid < THREAD_COUNT; tid++)
        {
            pthread_join(threads[tid], NULL);
        }
        
        for (int i = 0; i < boxCount; i++)
        {
            boxes[i].temp = newTemp[i];
        }
    }

    free(newTemp);
    return iteration;
}