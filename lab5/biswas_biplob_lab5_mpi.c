#include "amr.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>

#define max_boxs 15000
#define max_neighbor 80000
#define send_data_tag 2001
#define return_data_tag1 2002
#define return_data_tag2 2003
#define stop_tag 2005

#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define abs(x) (x < 0 ? -(x) : x)
#define NS_PER_US 1000
#define THREAD_COUNT 28

double AFFECT_RATE;
double EPSILON;
int boxCount;
Box *boxes;
double temp[max_boxs], temp2[max_boxs], updatedTemp[max_boxs];
int nCount[max_boxs], nCount2[max_boxs];
int uncmnLen[max_boxs], uncmnLen2[max_boxs];
int nStartIdx[max_boxs], nStartIdx2[max_boxs];
int perimtr[max_boxs], perimtr2[max_boxs];
int nCmnLen[max_neighbor], nCmnLen2[max_neighbor];
int nIds[max_neighbor], nIds2[max_neighbor];
int params[4], params2[4];
double maxTemp, minTemp;

int hasConverged(double *temps) 
{
    maxTemp = minTemp = temps[0];

    for (int i = 1; i < boxCount; i++) 
    {
        if (temps[i] > maxTemp) 
        {
            maxTemp = temps[i];
        }

        if (temps[i] < minTemp) 
        {
            minTemp = temps[i];
        }
    }

    return maxTemp - minTemp > maxTemp * EPSILON ? 0 : 1;
}

void freeBoxes() 
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

double getNewTemp(int *cmnLen, int *nId, double *bxTemp, int boxId, int nStrtIdx, int nCnt, int uncmnLn, int perimeter, double affect_rate) 
{
    double waat = bxTemp[boxId] * uncmnLn;

    for (int i = 0; i < nCnt; i++) 
    {
        waat += cmnLen[nStrtIdx + i] * bxTemp[nId[nStrtIdx + i]];
    }

    waat /= perimeter;
    return bxTemp[boxId] + (waat - bxTemp[boxId]) * affect_rate;
}


int main(int argc, char **argv) 
{   
    int tmpCount_to_send, tmpCount_to_receive, start_boxId_to_send, start_boxId_to_receive, end_box, boxCount_to_send,
    boxCount_to_receive, start_idx_for_nid, end_idx, nidCount_to_send, nidCount_to_receive;
    double affect_rate_to_send, affect_rate_to_receive;
    MPI_Status status;
    int cur_proc_id, root_process, ierr, num_procs, proc_id, box_per_process, sender, iteration;
    clock_t clockStart;

    ierr = MPI_Init(&argc, &argv);
    
    root_process = 0;
    ierr = MPI_Comm_rank(MPI_COMM_WORLD, &cur_proc_id);
    ierr = MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if(cur_proc_id == root_process) 
    {    
        sscanf(argv[1], "%lf", &AFFECT_RATE);
        sscanf(argv[2], "%lf", &EPSILON);
        freopen(argv[3], "r", stdin);
        if(!getInput()) 
        {
            printf("Error in input!");
        }

        for (int i = 0; i < boxCount; i++)
        {
            getNeighborConfig(&boxes[i]);
            temp[i] = boxes[i].temp;
            uncmnLen[i] = boxes[i].uncommonEdgeLength;
            nCount[i] = boxes[i].totalNeighborsCount;
            perimtr[i] = boxes[i].perimeter;
        }

        int k = 0;
        for (int i = 0; i < boxCount; i++)
        {
            nStartIdx[i] = k;
            for (int j = 0; j < boxes[i].totalNeighborsCount ; j++)
            {
                nCmnLen[k] = boxes[i].allNeighbors[j].commonEdgeLength;
                nIds[k] = boxes[i].allNeighbors[j].id;
                k++;
            }  
        }

        box_per_process = boxCount / (num_procs-1);

        /* distribute a portion of the boxes to each child process */
        for(proc_id = 1; proc_id < num_procs; proc_id++) 
        {         
            start_boxId_to_send = (proc_id - 1)*box_per_process;
            end_box = proc_id*box_per_process - 1;

            if((boxCount - end_box) < box_per_process)
                end_box = boxCount - 1;
            
            boxCount_to_send = end_box - start_boxId_to_send + 1;
            printf("Sending to process: %d -> BoxCount: %d, Starting BoxId: %d, Ending BoxId: %d\n", proc_id, boxCount_to_send, start_boxId_to_send, end_box);
            start_idx_for_nid = nStartIdx[start_boxId_to_send];
            end_idx = nStartIdx[end_box] + nCount[end_box] - 1;
            nidCount_to_send = end_idx - start_idx_for_nid + 1;
            affect_rate_to_send = AFFECT_RATE;

            tmpCount_to_send = boxCount;
            params[0] = tmpCount_to_send;
            params[1] = start_boxId_to_send;
            params[2] = boxCount_to_send;
            params[3] = nidCount_to_send;

            ierr = MPI_Send( &params[0], 4 , MPI_INT, proc_id, send_data_tag, MPI_COMM_WORLD);
                    
            ierr = MPI_Send( &uncmnLen[start_boxId_to_send], boxCount_to_send, MPI_INT, proc_id, send_data_tag, MPI_COMM_WORLD);

            ierr = MPI_Send( &nCount[start_boxId_to_send], boxCount_to_send, MPI_INT, proc_id, send_data_tag, MPI_COMM_WORLD);

            ierr = MPI_Send( &perimtr[start_boxId_to_send], boxCount_to_send, MPI_INT, proc_id, send_data_tag, MPI_COMM_WORLD);

            ierr = MPI_Send( &nIds[start_idx_for_nid], nidCount_to_send, MPI_INT, proc_id, send_data_tag, MPI_COMM_WORLD);

            ierr = MPI_Send( &nCmnLen[start_idx_for_nid], nidCount_to_send, MPI_INT, proc_id, send_data_tag, MPI_COMM_WORLD);

            ierr = MPI_Send( &affect_rate_to_send, 1, MPI_DOUBLE, proc_id, send_data_tag, MPI_COMM_WORLD);
        }

        int bxCount, boxIdx;
        clockStart = MPI_Wtime();
        for (iteration = 0; !hasConverged(temp); iteration++)
        {
            for(proc_id = 1; proc_id < num_procs; proc_id++) 
            {
                ierr = MPI_Send( &temp, tmpCount_to_send, MPI_DOUBLE, proc_id, send_data_tag, MPI_COMM_WORLD);
            }

            for (proc_id = 1, boxIdx = 0; proc_id < num_procs; proc_id++, boxIdx += bxCount)
            {
                ierr = MPI_Recv( &bxCount, 1, MPI_INT, proc_id, return_data_tag1, MPI_COMM_WORLD, &status);
                ierr = MPI_Recv( &updatedTemp[boxIdx], bxCount, MPI_DOUBLE, proc_id, return_data_tag2, MPI_COMM_WORLD, &status);
            }
            
            for (int id = 0; id < boxCount; id++)
            {
                temp[id] = updatedTemp[id];
            }
                       
        }        
        double diff = (MPI_Wtime() - clockStart) * 1000;
        for (proc_id = 1; proc_id < num_procs; proc_id++)
        {
            ierr = MPI_Send( &temp, tmpCount_to_send, MPI_DOUBLE, proc_id, stop_tag, MPI_COMM_WORLD);
        }
        printf("\n**************************************************************************\n");
        printf("dissipation converged in %d iterations,\n", iteration);
        printf("    with max DSV = %.7lf and min DSV = %.7lf\n", maxTemp, minTemp);
        printf("    affect rate  = %lf; epsilon = %lf\n\n", AFFECT_RATE, EPSILON);
        printf("Time taken for MPI operation: %lf ms\n", diff);
        printf("\n**************************************************************************\n");
        freeBoxes();
    }
    else 
    {
        ierr = MPI_Recv( &params2, 4 , MPI_INT, root_process, send_data_tag, MPI_COMM_WORLD, &status);
     
        tmpCount_to_receive = params2[0];
        start_boxId_to_receive = params2[1];
        boxCount_to_receive = params2[2];
        nidCount_to_receive = params2[3];      
        
        ierr = MPI_Recv( &uncmnLen2, boxCount_to_receive, MPI_INT, root_process, send_data_tag, MPI_COMM_WORLD, &status);

        ierr = MPI_Recv( &nCount2, boxCount_to_receive, MPI_INT, root_process, send_data_tag, MPI_COMM_WORLD, &status);

        ierr = MPI_Recv( &perimtr2, boxCount_to_receive, MPI_INT, root_process, send_data_tag, MPI_COMM_WORLD, &status);

        ierr = MPI_Recv( &nIds2, nidCount_to_receive, MPI_INT, root_process, send_data_tag, MPI_COMM_WORLD, &status);

        ierr = MPI_Recv( &nCmnLen2, nidCount_to_receive, MPI_INT, root_process, send_data_tag, MPI_COMM_WORLD, &status);

        ierr = MPI_Recv( &affect_rate_to_receive, 1, MPI_DOUBLE, root_process, send_data_tag, MPI_COMM_WORLD, &status);
        
        
        double *newTemp = (double *) malloc(boxCount_to_receive * sizeof(double)); 
        while (1)
        {
            ierr = MPI_Recv( &temp2, tmpCount_to_receive, MPI_DOUBLE, root_process, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if(status.MPI_TAG != stop_tag)
            {
                int strt_nidx = 0;
                #pragma omp parallel for num_threads(THREAD_COUNT)
                for (int i=0, j=start_boxId_to_receive; i < boxCount_to_receive; i++, j++)
                {
                    newTemp[i] = getNewTemp(nCmnLen2, nIds2, temp2, j, strt_nidx, nCount2[i], uncmnLen2[i], perimtr2[i], affect_rate_to_receive);
                    strt_nidx += nCount2[i];
                }
                ierr = MPI_Send( &boxCount_to_receive, 1, MPI_INT, root_process, return_data_tag1, MPI_COMM_WORLD);
                ierr = MPI_Send( newTemp, boxCount_to_receive, MPI_DOUBLE, root_process, return_data_tag2, MPI_COMM_WORLD);
            }
            else
            {
                break;
            }
            
        }
        free(newTemp);
    }
    ierr = MPI_Finalize();
}

