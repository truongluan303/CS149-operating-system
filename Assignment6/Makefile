output: summatrix_threaded.o
	gcc -pthread -Wall -Werror summatrix_threaded.o -o summatrix_threaded

summatrix_threaded.o: summatrix_threaded.c
	gcc -pthread -Wall -Werror -c summatrix_threaded.c

run:
	make
	./summatrix_threaded matrix1.txt matrix2.txt matrix3.txt 4

memcheck:
	make
	valgrind ./summatrix_threaded matrix1.txt matrix2.txt matrix3.txt 4

clean:
	rm *.o summatrix_threaded