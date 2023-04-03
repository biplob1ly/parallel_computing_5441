#ifndef AMR_H
#define AMR_H

typedef struct
{
    int id;
    int commonEdgeLength;
} Neighbor;

typedef struct
{
    int id;
    int upperLeftX, upperLeftY, height, width;
    int neighborCountInDir[4];
    int *neighborsInDir[4];
    double temp;
    int totalNeighborsCount;
    Neighbor *allNeighbors;
    int uncommonEdgeLength;
    int perimeter; 
} Box;

typedef struct
{
    int firstBoxId;
    int threadBoxCount;
    double *tempToUpdate;
} ThreadParams;

enum 
{
    TOP = 0, BOTTOM, LEFT, RIGHT
};

extern double AFFECT_RATE;
extern double EPSILON;
extern int boxCount;
extern Box *boxes;

int getInput();
int getBoxConfig();
void getNeighborConfig();
double getNewTemp();
int hasConverged();
void freeMemory();
int runConvergenceLoop();

#endif