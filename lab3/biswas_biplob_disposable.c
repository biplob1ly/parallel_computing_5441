#include "amr.h"
#include <stdlib.h>
//#include <stdio.h>
#include <omp.h>

int runConvergenceLoop() 
{
    int iteration, thread_created;
    double *newTemp = (double *) malloc(boxCount * sizeof(double));
    
    for(iteration=0; !hasConverged(); iteration++) 
    {
        #pragma omp parallel num_threads(THREAD_COUNT)
        {
            int id = omp_get_thread_num();
            if(id == 0) {
                thread_created = omp_get_num_threads();
            }

            #pragma omp for
            for (int i = 0; i < boxCount; i++)
            {
                newTemp[i] = getNewTemp(i);
            }
        }

        for (int i = 0; i < boxCount; i++)
        {
            boxes[i].temp = newTemp[i];
        }
    }

    //printf("Number of threads created: %d", thread_created);
    free(newTemp);
    return iteration;
}
