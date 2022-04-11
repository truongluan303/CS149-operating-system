output: summatrix.o
	gcc -Wall -Werror summatrix.o -o summatrix

summatrix.o: summatrix.c
	gcc -Wall -Werror -c summatrix.c

run:
	make
	./summatrix matrix.txt 4

memcheck:
	make
	valgrind ./summatrix matrix.txt

clean:
	rm *.o summatrix