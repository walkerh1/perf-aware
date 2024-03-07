all:

haversine: haversine.o
	gcc -Wall -g -O3 -o haversine haversine.o -lm

haversine.o: haversine.c haversine.h haversine_formula.c haversine_clock.c haversine_profiler.c
	gcc -Wall -g -O3 -c haversine.c

generate_haversines: generate_haversines.o
	gcc -Wall -o generate_haversines generate_haversines.o -lm

generate_haversines.o: generate_haversines.c haversine_formula.c haversine.h
	gcc -Wall -c generate_haversines.c

clean:
	rm -f generate_haversines
	rm -f haversine
	rm -f *.o
