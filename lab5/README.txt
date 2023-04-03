Compilation Command: make

[biplob@o0119 cse5441_lab5]$ make
mpicc -O3 -fopenmp biswas_biplob_lab5_mpi.c -o lab5_mpi



RunCommand: mpirun -np 5 -ppn 1 ./lab5_mpi <AFFECT_RATE> <EPSILON> <FILE_NAME>

[biplob@o0119 cse5441_lab5]$ mpirun -np 5 -ppn 1 ./lab5_mpi 0.02 0.02 testgrid_400_12206
 
Sending to process: 1 -> BoxCount: 3051, Starting BoxId: 0, Ending BoxId: 3050
Sending to process: 2 -> BoxCount: 3051, Starting BoxId: 3051, Ending BoxId: 6101
Sending to process: 3 -> BoxCount: 3051, Starting BoxId: 6102, Ending BoxId: 9152
Sending to process: 4 -> BoxCount: 3053, Starting BoxId: 9153, Ending BoxId: 12205

**************************************************************************
dissipation converged in 794818 iterations,
    with max DSV = 0.0846372 and min DSV = 0.0829445
    affect rate  = 0.020000; epsilon = 0.020000

Time taken for MPI operation: 100798.503160 ms

**************************************************************************


