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
int boxCount;
Box *boxes;
double maxTemp, minTemp;
double timeHostWoMemTransfer;

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


int runConvergenceLoopHost() 
{
    int iteration,j;
    double *newTemp = (double *) malloc(boxCount * sizeof(double));
    for (iteration = 0; !hasConverged(); iteration++) {
        clock_t perStart = clock();
        for (j = 0; j < boxCount; j++) {
            newTemp[j] = getNewTemp(j);
        }
        double perDiff = clock() - perStart;
        timeHostWoMemTransfer += perDiff / CLOCKS_PER_SEC;

        for (j = 0; j < boxCount; j++) {
            boxes[j].temp = newTemp[j];
        }
    }

    free(newTemp);
    return iteration;
}


int main(int argc, char **argv) 
{
    sscanf(argv[1], "%lf", &AFFECT_RATE);
    sscanf(argv[2], "%lf", &EPSILON);
    if(!getInput()) 
    {
        printf("Error in input!");
    }

    for (int i = 0; i < boxCount; i++)
    {
        getNeighborConfig(&boxes[i]);
    }

    int flopsPerIteration = 0;
    for(int i=0; i<boxCount; i++)
    {
        flopsPerIteration += boxes[i].totalNeighborsCount*2+5;
    }

    int iterationsHost = runConvergenceLoopHost();
    double nGigaFlop = iterationsHost * (flopsPerIteration / 1000000000.0);
    printf("Number of flop per iteration: %d\n", flopsPerIteration);
    printf("Total number of Giga flop in Device: %lf\n", nGigaFlop);

    printf("\n**************************************************************************\n");
    printf("dissipation converged in %d iterations,\n", iterationsHost);
    printf("    with max DSV = %.7lf and min DSV = %.7lf\n", maxTemp, minTemp);
    printf("    affect rate  = %lf; epsilon = %lf\n\n", AFFECT_RATE, EPSILON);
    double GigaflopsPerSecWoTransfer = nGigaFlop / timeHostWoMemTransfer;
    printf("Time taken on host (ms) = %lf\n", timeHostWoMemTransfer * 1000);
    printf("Giga FLOPS per sec on Host = %lf\n", GigaflopsPerSecWoTransfer);
    printf("\n**************************************************************************\n");

    freeMemory();
    return 0;
}

