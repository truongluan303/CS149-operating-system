output: mem_tracer.o
	gcc mem_tracer.o -o mem_tracer

mem_tracer.o: mem_tracer.c
	gcc -Wall -Werror -c mem_tracer.c

run:
	make
	./mem_tracer cmdfile.txt

clean:
	rm *.o mem_tracer