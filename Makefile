all:
	gcc -o queue_r.o -c -fPIC -std=c99 -Wall -pedantic -D_GNU_SOURCE -O3 -I./ -L./ -pthread queue_r.c
	ar -rs libqueue_r.a queue_r.o
	gcc -o test -std=c99 -Wall -pedantic -D_GNU_SOURCE -O3 -I./ -L./ tests/test.c -lqueue_r -pthread
	gcc -o test2 -std=c99 -Wall -pedantic -D_GNU_SOURCE -O3 -I./ -L./ tests/test2.c -lqueue_r -pthread 
clean:
	rm *.a *.o test test2
