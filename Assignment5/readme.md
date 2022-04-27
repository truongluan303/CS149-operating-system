# Process Manager Program
A program to manage processes. This program will read from an input textfile that contains shell commands on multiple lines. The program then will execute each command on a new process and redirect the output generated to a file, named by the process ID.

### Requirements
Must have `gcc` installed on your device. Ultimately, this program is written in Linux/Unix OS. However, it is also expected to be runnable on other Operating Systems.

### Usage
Since the project contains a makefile, you can simply use the `make` command to compile it to an executable named `proc_manager`. You can then run this executable with a textfile containing shell commands to execute them: `proc_manager <textfile>`. Since I have included an example textfile named `cmdfile.txt`, the process to run that file is: `make` (or alternatively `gcc proc_manager.c -o proc_manager`), then `proc_manager cmdfile.txt`.

The makefile also includes a few more usages such as `make clean` to execute `rm *.out *.err *.o proc_manager`, or `make memcheck` to run memory leak check with valgrind via command `valgrind proc_manager cmdfile.txt`.

### Warning
The program will restart any process that takes more than 2 seconds to execute. Hence, if you textfile contains a command that requires a long processing time, the program will end up in an infinite loop unless you kill it.
For example, a command `sleep 5` will theoretically take 5 seconds to execute, which is more than the limit time of 2 seconds. So the program will keep restarting this command.