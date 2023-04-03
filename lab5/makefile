COMPILER = mpicc
FLAGS = -O3
LIBS = -fopenmp

all: lab5_mpi

lab5_mpi:
	$(COMPILER) $(FLAGS) $(LIBS) biswas_biplob_lab5_mpi.c -o $@

clean:
	rm -f lab5_mpi
