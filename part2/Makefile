all:

haversine: haversine.o
	clang -Wall -arch x86_64 -g -O3 -o haversine haversine.o

haversine.o: haversine.c haversine.h haversine_formula.c haversine_clock.c haversine_profiler.c
	clang -Wall -arch x86_64 -g -O3 -c haversine.c

generate_haversines: generate_haversines.o
	clang -Wall -o generate_haversines generate_haversines.o

generate_haversines.o: generate_haversines.c haversine_formula.c haversine.h
	clang -Wall -c generate_haversines.c

clean:
	rm -f generate_haversines
	rm -f haversine
	rm -f *.o