all:

read_overhead: read_overhead_main.o common.h
	gcc -Wall -g -O3 -o read_overhead read_overhead_main.o common.h

read_overhead_main.o: read_overhead_main.c read_overhead_test.c buffer.c clock.c common.h
	gcc -Wall -g -O3 -c read_overhead_main.c read_overhead_test.c buffer.o clock.o common.h

buffer.o: buffer.c common.h
	gcc -Wall -g -O3 -c buffer.c common.h

clock.o: clock.c common.h
	gcc -Wall -g O3 -c clock.c common.h

clean:
	rm -f read_overhead
	rm -f *.o
