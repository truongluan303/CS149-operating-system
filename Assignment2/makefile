output: summatrix_parallel.o
	gcc -Wall -Werror summatrix_parallel.o -o summatrix_parallel

summatrix_parallel.o: summatrix_parallel.c
	gcc -Wall -Werror -c summatrix_parallel.c

run:
	make
	./summatrix_parallel matrix.txt morematrix.txt 4

memcheck:
	make
	valgrind ./summatrix_parallel matrix.txt morematrix.txt 4

clean:
	rm *.o summatrix_parallel