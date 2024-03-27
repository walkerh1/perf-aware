all:

read_overhead: read_overhead_main.o common.h
	gcc -Wall -g -O3 -o read_overhead read_overhead_main.o

read_overhead_main.o: read_overhead_main.c read_overhead_test.c repetition_tester.c buffer.c clock.c common.h
	gcc -Wall -g -O3 -c read_overhead_main.c

clean:
	rm -f read_overhead
	rm -f *.o
