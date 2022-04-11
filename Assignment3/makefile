output: proc_manager.o
	gcc -Wall -Werror proc_manager.o -o proc_manager

proc_manager.o: proc_manager.c
	gcc -Wall -Werror -c proc_manager.c

run:
	make
	./proc_manager cmdfile.txt

memcheck:
	make
	valgrind ./proc_manager cmdfile.txt

clean:
	rm *.o proc_manager *.err *.out