/**
 * @file summatrix_parallel.c
 * 
 * @author Hoang (Luan) Truong, Shubham Goswami
 * 
 * @date 2022-03-07
 */

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>


//===========================================================================//
//============================ Functions Prototypes =========================//
//===========================================================================//

/**
 * @brief Check if the input entered is valid
 * @param argc number of arguments entered
 * @param argv the arguments entered
 */
void validate_input(unsigned int argc, char** argv);

/**
 * @brief Get the extension of a file
 * @param filepath path to the file
 * @return the given file's extension
 */
const char* get_file_extension(const char* filepath);

/**
 * @brief Print a negative value warning on the console
 * @param value     the negative value
 * @param row_num   the row number where the value is on
 */
void print_warning(int value, unsigned int row_num);

/**
 * @brief Print an error message on the console and
 *        optionally return with status code 1
 * @param message       the error message to be printed
 * @param exit_program  whether to exit the program with code 1
 */
void report_error(const char* message, bool exit_program);

/**
 * @brief Calculate the sum of all non-negative numbers in the
 *        matrix in a given text file. If the number of columns
 *        in the matrix exceeds the given number n, then the
 *        calculation will stop at column (n)th
 * @param filepath  the path to the file containing the matrix
 * @param n         the number of columns to calculate up to
 * @return the sum calculated
 */
int calculate_matrix_sum(const char* filepath, unsigned int n);

/**
 * process the files in parallel and calculate the matrices sums
 * @param i     current counter
 * @param fd    file descriptors
 * @param argc  number of arguments passed in
 * @param args  the input arguments
 * @param n     input number N
 */
void process_matrices_parallel(unsigned int i, int fd[][2], unsigned int argc,
                               char** args, unsigned int n);


//===========================================================================//
//                                Main Function                              //
//===========================================================================//

int main(int argc, char** args)
{
    // check if the command line input is valid
    validate_input(argc, args);

    unsigned int num_of_files = argc - 2;
    unsigned int result = 0;
    unsigned int n = (int)strtol(args[argc - 1], (char**)NULL, 10);

    // create pipes for the input files
    // each pipe will connect 2 input files
    int fd[argc][2];
    for (unsigned int i = 0; i < num_of_files; i++)
    {
        if (pipe(fd[i]) < 0) report_error("Pipe Failed", true);
    }

    // process the matrices in parallel
    process_matrices_parallel(num_of_files, fd, argc, args, n);

    // add up all the matrices' sums calculated
    for (unsigned int i = 0; i < num_of_files; i++)
    {
        int sum_calculated;
        read(fd[i][0], &sum_calculated, sizeof(sum_calculated));
        // exit with status code 1 if there exists a failed matrix sum
        if (sum_calculated == -1) return 1;
        // otherwise, add the sum calculated to the result
        result += sum_calculated;
    }

    printf("Total sum: %d", result);

    return 0;
}


//===========================================================================//
//============================ Functions Definitions ========================//
//===========================================================================//


void validate_input(unsigned int argc, char** argv)
{
    char* error_message;

    // if not enough arguments
    if (argc < 3)
    {
        report_error("Error: Not enough input arguments\n", true);
    }
    // if number of arguments is valid
    else
    {
        // check whether the file's extension is txt
        for (unsigned short i = 1; i < argc - 1; i++)
        {
            char *filepath = *(argv + i);
            if (strcmp(get_file_extension(filepath), "txt") != 0)
            {
                report_error("Error: An argument input is not a text file\n", true);
            }
        }
        // check whether the given N parameter is a valid non-negative integer
        char* n = *(argv + argc - 1);
        for (unsigned short i = 0; n[i] != '\0'; i++)
        {
            if (isdigit(n[i]) == 0)
            {
                report_error("Error: N parameter is not valid\n", true);
            }
        }
    }
}


const char* get_file_extension(const char* filepath)
{
    const char *dot = strrchr(filepath, '.');
    if (!dot || dot == filepath) return "";
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
    printf("\033[1;31m");           // change text color to red
    printf("%s", message);          // print the error message
    printf("\033[0m");              // reset text color
    if (exit_program) exit(1);
}


int calculate_matrix_sum(const char* filepath, unsigned int n)
{
    FILE* file;     // the input file
    file = fopen(filepath, "r");    // try open the given file

    // if the file cannot be opened, print error message and exit with code 1
    if (file == NULL)
    {
        report_error("Range: cannot open file", false);
        return -1;
    }

    unsigned int result = 0;                // the sum to be calculated
    unsigned int count = 0;                 // keep track of the amount of numbers on a line
    unsigned int row = 1;                   // current row/line
    int num;                                // contains the current number read
    char ch = 0;                            // contains the char read
    bool ignore = false;                    // ignore the number read if true

    while (ch != EOF)
    {
        // if `ch` is a number
        if (isdigit(ch) || ch == '-')
        {                                   
            ungetc(ch, file);               // push ch back and scan the whole number
            fscanf(file, "%d", &num);

            if (!ignore)                               // skip if ignore is set
            {
                if (num < 0) print_warning(num, row);
                else result += num;
            }
            // if there are more nums on the line than input N,
            // ignore the remaining nums on that line
            if (++count >= n) ignore = true;
        }
        ch = getc(file);
        if (ch == '\n')                 // if newline detected
        {
            row++;                      // increase the row number
            count = 0;                  // reset the numbers counted
            ignore = false;             // reset ignore flag
        }
    }
    fclose(file);                // close the file
    return result;                      // return the result calculated
}


void process_matrices_parallel(unsigned int i, int fd[][2], unsigned int argc,
                               char** args, unsigned int n)
{
    if (i == 0) return;

    int pid = fork();

    // if fork failed
    if (pid < 0) report_error("Fork Failed", true);
    // child process
    if (pid == 0)
    {
        //make recursion to process all the input files in parallel
        process_matrices_parallel(i - 1, fd, argc, args, n);
        unsigned int sum = calculate_matrix_sum(args[i], n);

        //Use the pipe to transfer the array
        write(fd[i - 1][1], &sum, sizeof(sum));
        exit(0);
    }
    // parent process
    else
    {
        close(fd[n - 1][1]);
        wait(NULL);
    }
}