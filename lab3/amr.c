#include "amr.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define abs(x) (x < 0 ? -(x) : x)
#define NS_PER_US 1000

double AFFECT_RATE;
double EPSILON;
int THREAD_COUNT;
int boxCount;
Box *boxes;
double maxTemp, minTemp;

double getNewTemp(int boxId) 
{
    double waat = boxes[boxId].temp * boxes[boxId].uncommonEdgeLength;

    for (int i = 0; i < boxes[boxId].totalNeighborsCount; i++) 
    {
        waat += boxes[boxId].allNeighbors[i].commonEdgeLength * boxes[boxes[boxId].allNeighbors[i].id].temp;
    }

    waat /= boxes[boxId].perimeter;

    return boxes[boxId].temp + (waat - boxes[boxId].temp) * AFFECT_RATE;
}

int hasConverged() 
{
    maxTemp = minTemp = boxes[0].temp;

    for (int i = 1; i < boxCount; i++) 
    {
        if (boxes[i].temp > maxTemp) 
        {
            maxTemp = boxes[i].temp;
        }

        if (boxes[i].temp < minTemp) 
        {
            minTemp = boxes[i].temp;
        }
    }

    return maxTemp - minTemp > maxTemp * EPSILON ? 0 : 1;
}

void freeMemory() 
{
    int i, j;

    for (i = 0; i < boxCount; i++) 
    {
        for (j = 0; j < 4; j++) 
        {
            free(boxes[i].neighborsInDir[j]);
        }

        free(boxes[i].allNeighbors);
    }

    free(boxes);
}

void getNeighborConfig(Box *box) 
{
    box->totalNeighborsCount = 0;
    for (int i = 0; i < 4; i++)
    {
        box->totalNeighborsCount += box->neighborCountInDir[i];
    }
    box->allNeighbors = (Neighbor *) malloc(box->totalNeighborsCount * sizeof(Neighbor));
    int k = 0;
    box->uncommonEdgeLength = box->perimeter;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < box->neighborCountInDir[i]; j++)
        {
            Box neighbor = boxes[box->neighborsInDir[i][j]];
            int commonEdge = 0;
            if (i < 2) {
                commonEdge = abs(max(box->upperLeftX, neighbor.upperLeftX) - min(box->upperLeftX + box->width, neighbor.upperLeftX + neighbor.width));
            } else {
                commonEdge = abs(max(box->upperLeftY, neighbor.upperLeftY) - min(box->upperLeftY + box->height, neighbor.upperLeftY + neighbor.height));
            }
            box->allNeighbors[k].id = neighbor.id;
            box->allNeighbors[k].commonEdgeLength = commonEdge;
            box->uncommonEdgeLength -= commonEdge;
            k++;
        }
        
    }
    
}

int getBoxConfig(Box *box) 
{
    if(!scanf("%d", &box->id)) 
    {
        return 0;
    }
    if(!scanf("%d %d %d %d", &box->upperLeftY, &box->upperLeftX, &box->height, &box->width)) 
    {
        return 0;
    }
    box->perimeter = 2*(box->height + box->width);

    for (int i = 0; i < 4; i++)
    {
        if(!scanf("%d", &box->neighborCountInDir[i])) 
        {
            return 0;
        }
        box->neighborsInDir[i] = (int *) malloc(box->neighborCountInDir[i] * sizeof(int));
        for (int j = 0; j < box->neighborCountInDir[i]; j++)
        {
            if(!scanf("%d", &box->neighborsInDir[i][j])) 
            {
                return 0;
            }
        }
    }
    if(!scanf("%lf", &box->temp)) 
    {
        return 0;
    }
    return 1;
}

int getInput() 
{
    if(!scanf("%d %*d %*d", &boxCount)) 
    {
        return 0;
    }

    boxes = (Box *) malloc(boxCount * sizeof(Box));
    for (int i = 0; i < boxCount; i++)
    {
        getBoxConfig(&boxes[i]);
    }

    int garbage = scanf("%*d");
    return 1;
}

int main(int argc, char **argv) 
{
    sscanf(argv[1], "%lf", &AFFECT_RATE);
    sscanf(argv[2], "%lf", &EPSILON);
    sscanf(argv[3], "%d", &THREAD_COUNT);
    if(!getInput()) 
    {
        printf("Error in input!");
    }

    for (int i = 0; i < boxCount; i++)
    {
        getNeighborConfig(&boxes[i]);
    }
    
    struct timespec chronoStart, chronoEnd;
    clock_t clockStart, clockEnd;
    time_t timeStart, timeEnd;

    clockStart = clock();
    time(&timeStart);
    clock_gettime(CLOCK_REALTIME, &chronoStart);

    int iterations = runConvergenceLoop();

    clockEnd = clock();
    time(&timeEnd);
    clock_gettime(CLOCK_REALTIME, &chronoEnd);

    int clockDiff = clockEnd - clockStart;
    int timeDiff = timeEnd - timeStart;
    double chronoDiff = (double)(((chronoEnd.tv_sec - chronoStart.tv_sec) * CLOCKS_PER_SEC)
                          + ((chronoEnd.tv_nsec - chronoStart.tv_nsec) / NS_PER_US));

    printf("\n**************************************************************************\n");
    printf("dissipation converged in %d iterations,\n", iterations);
    printf("    with max DSV = %.7lf and min DSV = %.7lf\n", maxTemp, minTemp);
    printf("    affect rate  = %lf; epsilon = %lf\n\n", AFFECT_RATE, EPSILON);
    printf("elapsed convergence loop time  (clock): %d\n", clockDiff);
    printf("elapsed convergence loop time   (time): %d\n", timeDiff);
    printf("elapsed convergence loop time (chrono): %lf\n", chronoDiff);
    printf("\n**************************************************************************\n");

    freeMemory();

    return 0;
}

