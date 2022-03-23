#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdbool.h>

/*---------------------------------------------------------------------------*/
/* Function Prototypes                                                       */
/*---------------------------------------------------------------------------*/
/**
 * check if the input arguments are valid
 * @param argc the arguments count
 * @param argv the arguments vector
 * @return true if the input arguments are valid
 */
bool validate_input(int argc, char** argv);
/**
 * get the extension/file from a file's name
 * @param path the path to the file
 * @return the extension of the given file
 */
const char* get_file_extension(const char* path);
/**
 * @brief print an error message on the console
 * @param message the error message to be printed
 */
void print_error(const char* message);




/*---------------------------------------------------------------------------*/
/* Main Function                                                             */
/*---------------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    //Variables related to file
    FILE *cmd_file;
    char input[256] = "";
    char *arguments[16] = {NULL};
    char *line;
    int num = 0;

    pid_t pid;
    pid_t cpid;
    int num_of_children = 0;
    int wait_stat = 0;
    int err_flag;

    //Variables related to the output files
    char filenameOut[15] = "";
    char filenameError[15] = "";
    int fdOut;
    int fdError;

    //Variables related to error file
    char *err_content;
    char err_string[10] = "error:\n";
    char newLine = '\n';

    //open the text file for reading
    cmd_file = fopen(argv[1], "r");
    if (cmd_file != NULL)
    {

        //Read each line of the file
        while (1)
        {
            line = fgets(input, sizeof(input), cmd_file);
            printf("%s", line);
            if (line == NULL)
            {
                break;
            }
            num++;

            int i = 0;
            char *argument;

            //Split the command line using delimiters
            arguments[0] = strtok(input, " \t\n");
            if (arguments[0] != NULL)
            {
                while ((argument = strtok(NULL, " \t\n")) != NULL)
                {
                    i++;
                    arguments[i] = argument;
                }
            }

            pid = fork();
            if (pid == -1)
            {
                //Print related error when pid is -1
                err_content = strerror(errno);
                write(2, err_string, strlen(err_string));
                write(2, err_content, strlen(err_content));
                write(2, &newLine, 1);
            }
            else
            {
                if (pid == 0)
                {
                    //store the output in the buffer
                    sprintf(filenameOut, "%d.out", getpid());
                    //open an output file
                    fdOut = open(filenameOut, O_RDWR | O_CREAT | O_APPEND);
                    //Change the permissions of the file
                    fchmod(fdOut, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                    if (fdOut != -1)
                    {
                        dup2(fdOut, 1); //create a copy of file
                    }

                    printf("Starting command %d: child %d pid of parent %d\n", num, getpid(), getppid());

                    //store the output in the buffer
                    sprintf(filenameError, "%d.err", getpid());
                    //open an output file for error and change the permissions of the file
                    fdError = open(filenameError, O_RDWR | O_CREAT | O_APPEND);
                    fchmod(fdError, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                    if (fdError != -1)
                    {
                        //create a copy of the file
                        dup2(fdError, 2);
                    }

                    //replace the process image
                    err_flag = execvp(arguments[0], arguments);
                    //Output the details of error and exit code
                    if (err_flag)
                    {
                        char err_str[64] = "";
                        int exitCode = errno;
                        snprintf(err_str, sizeof(err_str), "failed to execute command: %s err: %d\n", arguments[0], exitCode);
                        perror(err_str);
                        exit(exitCode);
                    }
                    //close the output files
                    close(fdOut);
                    close(fdError);
                    break;
                }
                else
                {

                    num_of_children++;
                }
            }
        }
    }
    else
    {
        //Output the errors in the .err file
        err_content = strerror(errno);
        write(2, err_string, strlen(err_string));
        write(2, err_content, strlen(err_content));
        write(2, &newLine, 1);
    }

    if (cmd_file != NULL)
    {
        fclose(cmd_file);
    }

    if (pid != 0)
    {
        //Append a corresponding statement in the output file pid.out whether the child finished/exited
        while (num_of_children > 0)
        {
            cpid = wait(&wait_stat);

            char stringBuffer[100] = "";
            int count;
            char exitCode;
            int signal;

            //open an output file pid.out and change the permissions of the file
            sprintf(filenameOut, "%d.out", cpid);
            fdOut = open(filenameOut, O_RDWR | O_CREAT | O_APPEND);
            fchmod(fdOut, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

            //Write the pid of the child in the output file
            count = snprintf(stringBuffer, sizeof(stringBuffer), "Finished child %d pid of parent %d\n", cpid, getpid());
            write(fdOut, stringBuffer, count);

            //open the pid.err and give the correct permissions
            sprintf(filenameError, "%d.err", cpid);
            fdError = open(filenameError, O_RDWR | O_CREAT | O_APPEND);
            fchmod(fdError, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

            //Output the exit code
            if (WIFEXITED(wait_stat))
            {
                exitCode = WEXITSTATUS(wait_stat);
                count = snprintf(stringBuffer, sizeof(stringBuffer), "Exited with exitcode = %d\n", exitCode);
            }

            //Output the kill signal
            if (WIFSIGNALED(wait_stat))
            {
                signal = WTERMSIG(wait_stat);
                count = snprintf(stringBuffer, sizeof(stringBuffer), "Killed with signal %d\n", signal);
            }

            //write the error to the file
            write(fdError, stringBuffer, count);

            num_of_children--;
        }
    }
    return 0;
}


/*---------------------------------------------------------------------------*/
/* Function Definitions                                                      */
/*---------------------------------------------------------------------------*/

bool validate_input(int argc, char** argv)
{
    if (argc < 2)
    {
        return false;
    }
    for (unsigned int i = 0; i < argc; i++)
    {
        char* filepath = argv[i];
        const char* file_ext = get_file_extension(filepath);
        if (strcmp(file_ext, "txt") != 0)
        {

            return false;
        }
    }
    return true;
}


const char* get_file_extension(const char* path)
{
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path) return "";
    return dot + 1;
}


void print_error(const char* message)
{
    printf("\033[1;31m");       // change text color to red
    printf("%s", message);      // print the error message
    printf("\033[0m");          // reset text color
}
