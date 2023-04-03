#include "amr.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define abs(x) (x < 0 ? -(x) : x)

double AFFECT_RATE;
double EPSILON;
int boxCount;
Box *boxes;
double maxTemp, minTemp;
double timeDevice;

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


__global__ void calcNewTemp(Box *dboxes, double *dtemp, double affectRate, int boxCnt) 
{   
    int boxId = blockIdx.x*blockDim.x + threadIdx.x;
    if (boxId < boxCnt) 
    {
        double waat = dboxes[boxId].temp * dboxes[boxId].uncommonEdgeLength;
        for (int i = 0; i < dboxes[boxId].totalNeighborsCount; i++) 
        {
            waat += dboxes[boxId].allNeighbors[i].commonEdgeLength * dboxes[dboxes[boxId].allNeighbors[i].id].temp;
        }
        waat /= dboxes[boxId].perimeter;
        dtemp[boxId] = dboxes[boxId].temp + (waat - dboxes[boxId].temp) * affectRate;
    }
}


__global__ void updateTemp(Box *dboxes, double *dtemp, int boxCnt) 
{   
    int boxId = blockIdx.x*blockDim.x + threadIdx.x;
    if(boxId < boxCnt) 
    {
        dboxes[boxId].temp = dtemp[boxId];
    }
}


int runConvergenceLoopDevice() 
{
    int iteration,j;
    
    //Memory allocation
    Box *deviceBoxes;
    Neighbor *dboxNeighbors[boxCount];
    double *newTemp, *newDeviceTemp;
    int temp_memsize = boxCount * sizeof(double);
    int boxes_memsize = boxCount * sizeof(Box);
    newTemp = (double *) malloc(temp_memsize);
    cudaMalloc((void **) &newDeviceTemp, temp_memsize);
    cudaMalloc((void **) &deviceBoxes, boxes_memsize);
    for (int i=0; i<boxCount; i++)
    {
        cudaMalloc((void **) &dboxNeighbors[i], boxes[i].totalNeighborsCount * sizeof(Neighbor));
    }
    int thread_per_block = 128;
    int nBlocks = (boxCount + thread_per_block -1)/thread_per_block;
    dim3 dimGrid(nBlocks);
    dim3 dimBlock(thread_per_block);
    cudaMemcpy(deviceBoxes, boxes, boxes_memsize, cudaMemcpyHostToDevice);
    for (int i=0; i<boxCount; i++)
    {
        cudaMemcpy(dboxNeighbors[i], boxes[i].allNeighbors, boxes[i].totalNeighborsCount * sizeof(Neighbor), cudaMemcpyHostToDevice);
        cudaMemcpy(&(deviceBoxes[i].allNeighbors), &dboxNeighbors[i], sizeof(Neighbor*), cudaMemcpyHostToDevice);
    }

    for (iteration = 0; !hasConverged(); iteration++) 
    {
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);
        cudaEventRecord(start);
        calcNewTemp<<<dimGrid, dimBlock>>>(deviceBoxes, newDeviceTemp, AFFECT_RATE, boxCount);
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
        float diff = 0;
        cudaEventElapsedTime(&diff, start, stop);
        timeDevice += diff/1000;

        updateTemp<<<dimGrid, dimBlock>>>(deviceBoxes, newDeviceTemp, boxCount);
        cudaMemcpy(newTemp, newDeviceTemp, temp_memsize, cudaMemcpyDeviceToHost);
        for (j = 0; j < boxCount; j++) 
        {
             boxes[j].temp = newTemp[j];
        }
    }

    free(newTemp);
    cudaFree(newDeviceTemp);
    cudaFree(deviceBoxes);
    cudaFree(dboxNeighbors[boxCount]);
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

    int iterationsDevice = runConvergenceLoopDevice();
    double nGigaFlop = iterationsDevice * (flopsPerIteration / 1000000000.0);
    printf("Number of flop per iteration: %d\n", flopsPerIteration);
    printf("Total number of Giga flop in Device: %lf\n", nGigaFlop);

    printf("\n**************************************************************************\n");
    printf("dissipation converged in %d iterations,\n", iterationsDevice);
    printf("    with max DSV = %.7lf and min DSV = %.7lf\n", maxTemp, minTemp);
    printf("    affect rate  = %lf; epsilon = %lf\n\n", AFFECT_RATE, EPSILON);
    double GigaFlopsPerSec = nGigaFlop / timeDevice;
    printf("Time taken on device (ms) = %lf\n", timeDevice * 1000);
    printf("Giga FLOPS per sec on device = %lf\n", GigaFlopsPerSec);
    printf("\n**************************************************************************\n");

    freeMemory();
    cudaDeviceReset();
    return 0;
}

