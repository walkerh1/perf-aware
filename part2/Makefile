generate_haversines: generate_haversines.o
	gcc -Wall -o generate_haversines generate_haversines.o

generate_haversines.o: generate_haversines.c haversine.h
	gcc -Wall -c generate_haversines.c

clean:
	rm generate_haversines
	rm *.o