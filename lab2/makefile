GCC = icc
FLAGS = -O3
LIBS = -lrt -pthread

all: serial disposable persistent

serial:
	$(GCC) $(FLAGS) $(LIBS) biswas_biplob_serial.c amr.c -o $@

disposable:
	$(GCC) $(FLAGS) $(LIBS) biswas_biplob_disposable.c amr.c -o $@

persistent:
	$(GCC) $(FLAGS) $(LIBS) biswas_biplob_persistent.c amr.c -o $@

clean:
	rm -f serial disposable persistent
