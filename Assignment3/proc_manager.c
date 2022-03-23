/**
 * @file proc_manager.c
 * @author Hoang Truong (Luan), Shubham Goswami
 * @date 2022-03-22
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>


/*---------------------------------------------------------------------------*/
/* Function Prototypes                                                       */
/*---------------------------------------------------------------------------*/
/**
 * @brief check if the input arguments are valid
 * @param argc the arguments count
 * @param argv the arguments vector
 * @return true if the input arguments are valid
 */
bool validate_input(int argc, char** argv);
/**
 * @brief get the extension/file from a file's name
 * @param path the path to the file
 * @return the extension of the given file
 */
const char* get_file_extension(const char* path);
/**
 * @brief print a negative value warning on the console
 * @param value     the negative value
 * @param row_num   the row number where the value is on
 */
void print_warning(int value, unsigned int row_num);
/**
 * @brief print an error message on the console and
 *        optionally return with status code 1
 * @param message       the error message to be printed
 * @param exit_program  whether to exit the program with code 1
 */
void report_error(const char* message, bool exit_program);


/*---------------------------------------------------------------------------*/
/* Main Function                                                             */
/*---------------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    // check input validity
    if (!validate_input(argc, argv)) return 1;

    //Variables related to file
    FILE *cmd_file;
    char input[256] = "";
    char *cmds[16] = { NULL };
    char *line;
    int num = 0;

    pid_t pid = 0;
    pid_t cpid;
    int num_of_children = 0;
    int wait_stat = 0;
    int err_flag;

    //Variables related to the output files
    char file_out[15] = "";
    char file_err[15] = "";
    int fdout;
    int fderr;

    //Variables related to error file
    char *err_content;
    char err_string[10] = "error:\n";

    //open the text file for reading
    cmd_file = fopen(argv[1], "r");
    if (cmd_file != NULL)
    {
        printf("%s", "\nProgram Started...\n");
        
        //Read each line of the file
        while (true)
        {
            line = fgets(input, sizeof(input), cmd_file);
            if (line == NULL) break;

            num++;

            int i = 0;
            char *cmd;

            // split the command line using delimiters
            cmds[0] = strtok(input, " \t\n");
            if (cmds[0] != NULL)
            {
                while ((cmd = strtok(NULL, " \t\n")) != NULL)
                {
                    i++;
                    cmds[i] = cmd;
                }
            }

            pid = fork();
            if (pid == -1)
            {
                //Print related error when pid is -1
                err_content = strerror(errno);
                write(2, err_string, strlen(err_string));
                write(2, err_content, strlen(err_content));
                write(2, "\n", 1);
            }
            else
            {
                if (pid == 0)
                {
                    // store the output in the buffer
                    sprintf(file_out, "%d.out", getpid());
                    // open an output file
                    fdout = open(file_out, O_RDWR | O_CREAT | O_APPEND);
                    // change the permissions of the file
                    fchmod(fdout, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                           S_IROTH | S_IWOTH);
                    if (fdout != -1)
                    {
                        dup2(fdout, 1);     //create a copy of file
                    }

                    //store the output in the buffer
                    sprintf(file_err, "%d.err", getpid());
                    //open an output file for error and change the permissions of the file
                    fderr = open(file_err, O_RDWR | O_CREAT | O_APPEND);
                    fchmod(fderr, S_IRUSR | S_IWUSR | S_IRGRP |
                           S_IWGRP | S_IROTH | S_IWOTH);
                    if (fderr != -1)
                    {
                        dup2(fderr, 2);   // create a copy of the file
                    }
                    //replace the process image
                    err_flag = execvp(cmds[0], cmds);
                    //Output the details of error and exit code
                    if (err_flag)
                    {
                        char err_str[64] = "";
                        int exitCode = errno;
                        snprintf(err_str, sizeof(err_str), 
                                 "failed to execute command: %s err: %d\n", 
                                 cmds[0], exitCode);
                        perror(err_str);
                        exit(exitCode);
                    }
                    //close the output files
                    close(fdout);
                    close(fderr);
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
        write(2, "\n", 1);
    }
    if (cmd_file != NULL) fclose(cmd_file);

    if (pid != 0)
    {
        // append a corresponding statement in the output file pid.out 
        // whether the child finished/exited
        while (num_of_children > 0)
        {
            cpid = wait(&wait_stat);

            char str_buff[100] = "";
            int count;
            char exit_code;
            int signal;

            //open an output file pid.out and change the permissions of the file
            sprintf(file_out, "%d.out", cpid);
            fdout = open(file_out, O_RDWR | O_CREAT | O_APPEND);
            fchmod(fdout, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

            //Write the pid of the child in the output file
            count = snprintf(str_buff, sizeof(str_buff), 
                             "Finished child %d pid of parent %d\n", 
                             cpid, getpid());
            write(fdout, str_buff, count);

            //open the pid.err and give the correct permissions
            sprintf(file_err, "%d.err", cpid);
            fderr = open(file_err, O_RDWR | 
                           O_CREAT | O_APPEND);
            fchmod(fderr, S_IRUSR | S_IWUSR | S_IRGRP | 
                   S_IWGRP | S_IROTH | S_IWOTH);

            //Output the exit code
            if (WIFEXITED(wait_stat))
            {
                exit_code = WEXITSTATUS(wait_stat);
                count = snprintf(str_buff, sizeof(str_buff), 
                                 "Exited with exit code = %d\n", exit_code);
            }

            //Output the kill signal
            if (WIFSIGNALED(wait_stat))
            {
                signal = WTERMSIG(wait_stat);
                count = snprintf(str_buff, sizeof(str_buff), 
                                 "Killed with signal %d\n", signal);
            }

            write(fderr, str_buff, count);  // write the error to the file
            num_of_children--;              // decrease number of children
        }
    }
    printf("%s", "\nProgram Finished!\n\n");
    return 0;
}


/*---------------------------------------------------------------------------*/
/* Function Definitions                                                      */
/*---------------------------------------------------------------------------*/


bool validate_input(int argc, char** argv)
{
    // if not enough arguments
    if (argc != 2)
    {
        report_error("Error: Invalid number of arguments\n", true);
        return false;
    }
    // check whether the file's extension is txt
    char *filepath = *(argv + 1);
    if (strcmp(get_file_extension(filepath), "txt") != 0)
    {
        report_error("Error: The argument input is not a text file\n", true);
        return false;
    }
    return true;
}


const char* get_file_extension(const char* path)
{
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path) return "";
    return dot + 1;
}


void print_warning(int value, unsigned int row_num)
{
    printf("\033[1;33m");                   // change text color to yellow
    printf("Warning: value");
    printf("\033[1;31m");                   // change text color to red
    printf(" %d ", value);
    printf("\033[1;33m");                   // change text color to yellow
    printf("found on row %d\n", row_num);
    printf("\033[0m");                      // reset text color
}


void report_error(const char* message, bool exit_program)
{
    printf("\033[1;31m");                   // change text color to red
    printf("%s", message);                  // print the error message
    printf("\033[0m");                      // reset text color
    if (exit_program) exit(1);
}
