GCC = icc
NVCC = nvcc
FLAGS = -O3
LIBS = -lrt

all: serial lab4p1

serial:	
	$(GCC) $(FLAGS) $(LIBS) biswas_biplob_serial.c -o $@

lab4p1:	
	$(NVCC) $(FLAGS) $(LIBS) biswas_biplob_lab4p1.cu -o $@

clean:
	rm -f serial lab4p1
