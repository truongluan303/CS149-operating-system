output: mem_tracer.o
	gcc -Wall -Werror mem_tracer.o -o mem_tracer

mem_tracer.o: mem_tracer.c
	gcc -Wall -Werror -c mem_tracer.c

run:
	make
	./mem_tracer cmdfile.txt

memcheck:
	make
	valgrind --leak-check=full --track-origins=yes ./mem_tracer cmdfile.txt

clean:
	rm *.o mem_tracer