all:

haversine: haversine.o
	gcc -Wall -o haversine haversine.o

haversine.o: haversine.c haversine.h haversine_formula.c haversine_clock.c
	gcc -Wall -c haversine.c haversine_formula.c haversine_clock.c

generate_haversines: generate_haversines.o
	gcc -Wall -o generate_haversines generate_haversines.o

generate_haversines.o: generate_haversines.c haversine_formula.c haversine.h
	gcc -Wall -c generate_haversines.c haversine_formula.c

clean:
	rm -f generate_haversines
	rm -f haversine
	rm -f *.o