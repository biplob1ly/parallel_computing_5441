#include "amr.h"
#include <stdlib.h>

int runConvergenceLoop() 
{
    int iteration,j;
    double *newTemp = (double *) malloc(boxCount * sizeof(double));
    for (iteration = 0; !hasConverged(); iteration++) {
        for (j = 0; j < boxCount; j++) {
            newTemp[j] = getNewTemp(j);
        }

        for (j = 0; j < boxCount; j++) {
            boxes[j].temp = newTemp[j];
        }
    }

    free(newTemp);
    return iteration;
}