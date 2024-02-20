sim8086: sim8086.o sim8086_print.o
	gcc -Wall -o sim8086 sim8086.o sim8086_print.o

sim8086.o: sim8086.c sim8086.h
	gcc -Wall -c sim8086.c

sim8086_print.o: sim8086_print.c sim8086.h
	gcc -Wall -c sim8086_print.c

clean:
	rm -f sim8086
	rm *.o