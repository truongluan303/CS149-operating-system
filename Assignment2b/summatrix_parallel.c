/******************************************************************************
 * 
 * @file summatrix_parallel.c
 * 
 * @author Hoang (Luan) Truong, Shubham Goswami
 * 
 * @date 2022-03-07
 * 
 *****************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>

#define INT_SIZE sizeof(int)


/**
 * @brief boolean type
 */
typedef enum 
{
    TRUE, 
    FALSE,
} 
BOOLEAN;


/*===========================================================================*/
/* Globals                                                                   */
/*===========================================================================*/

int     argc;               // no. of input arguments
char**  argv;               // input arguments vector
int     n;                  // number of columns in matrix to process
int*    shared_mem;         // the shared memory


/*===========================================================================*/
/* Function Prototypes                                                       */
/*===========================================================================*/

void init_globs(int arg_c, char** arg_v);

BOOLEAN validate_input(int argv, char** argc);

const char* get_file_extension(const char* filepath);

void print_warning(int value, unsigned int row_num, const char* filename);

void report_error(const char* message);

int process_file(int depth);

int calculate_matrix_sum(const char* filepath, unsigned int n);


/*===========================================================================*/
/* main                                                                      */
/*===========================================================================*/

int main(int arg_c, char** arg_v)
{
    // check if the command line input is valid
    if (validate_input(arg_c, arg_v) == FALSE) return 1;

    // initialize global variables
    init_globs(arg_c, arg_v);

    int num_of_files = argc - 2;

    if (process_file(num_of_files) == -1) return -1;

    printf("\n\nThe matrix sum is: %d\n", *shared_mem);

    return 0;
}


/*===========================================================================*/
/* init_globs                   Initialize the global variables              */
/*===========================================================================*/

void init_globs(int arg_c, char** arg_v)
{
    n           = (int) strtol(arg_v[arg_c - 1], (char**)NULL, 10);
    argc        = arg_c;
    argv        = arg_v;
    shared_mem  = (int*)mmap(
                        NULL,
                        (int) n * INT_SIZE,
                        PROT_READ|PROT_WRITE,
                        MAP_ANON|MAP_SHARED,
                        -1, 
                        0);
}


/*===========================================================================*/
/* validate_input               Check if the input arguments are valid       */
/*===========================================================================*/

BOOLEAN validate_input(int argc, char** argv)
{
    // if not enough arguments, then show error and return
    if (argc < 3)
    {
        report_error("Error: Not enough input arguments\n");
        return FALSE;
    }

    // check whether the file's extension is txt
    for (unsigned short i = 1; i < argc - 1; i++)
    {
        char *filepath = *(argv + i);
        if (strcmp(get_file_extension(filepath), "txt") != 0)
        {
            report_error("Error: An argument input is not a text file\n");
            return FALSE;
        }
    }

    // check whether the given N parameter is a valid non-negative integer
    char* n = *(argv + argc - 1);
    for (unsigned short i = 0; n[i] != '\0'; i++)
    {
        if (isdigit(n[i]) == 0)
        {
            report_error("Error: N parameter is not valid\n");
            return FALSE;
        }
    }

    return TRUE;
}


/*===========================================================================*/
/* get_file_extension           Return the extension of a file               */
/*===========================================================================*/

const char* get_file_extension(const char* filepath)
{
    const char *dot = strrchr(filepath, '.');
    if (!dot || dot == filepath) return "";
    return dot + 1;
}


/*===========================================================================*/
/* print_warning                Print out a warning due to a negative value  */
/*===========================================================================*/

void print_warning(int value, unsigned int row_num, const char* filename)
{
    printf("\033[1;33m");                   // change text color to yellow
    printf("Warning: value");
    printf("\033[1;31m");                   // change text color to red
    printf(" %d ", value);
    printf("\033[1;33m");                   // change text color to yellow
    printf("found on row %d in '%s'\n", 
           row_num,
           filename);
    printf("\033[0m");                      // reset text color
}


/*===========================================================================*/
/* report_error                 Print out an error message                   */
/*===========================================================================*/

void report_error(const char* message)
{
    printf("\033[1;31m");                   // change text color to red
    printf("%s", message);
    printf("\033[0m");                      // reset text color
}


/*===========================================================================*/
/* calculate_matrix_sum         Calculate the matrix sum in a given file     */
/*===========================================================================*/

int calculate_matrix_sum(const char* filepath, unsigned int n)
{
    FILE* file;                         // the input file
    file = fopen(filepath, "r");        // try open the given file

    if (file == NULL)
    {
        report_error("Range: cannot open file");
        return -1;
    }

    unsigned int    result  = 0;        // the sum to be calculated
    unsigned int    count   = 0;        // count the no. of numbers on a line
    unsigned int    row     = 1;        // current row/line
    int             num;                // contains the current number read
    char            ch      = 0;        // contains the char read
    BOOLEAN         ignore  = FALSE;    // ignore the number read if true

    while (ch != EOF)
    {
        // if `ch` is a number
        if (isdigit(ch) || ch == '-')
        {                                   
            ungetc(ch, file);           // push ch back & scan the whole number
            fscanf(file, "%d", &num);

            if (ignore == FALSE)        // skip if ignore is set
            {
                if (num < 0) print_warning(num, row, filepath);
                else result += num;
            }
            // if there are more nums on the line than input N,
            // ignore the remaining nums on that line
            if (++count >= n) ignore = TRUE;
        }
        ch = getc(file);
        if (ch == '\n')                 // if newline detected
        {
            row++;                      // increase the row number
            count = 0;                  // reset the numbers counted
            ignore = FALSE;             // reset ignore flag
        }
    }
    fclose(file);                       // close the file
    return result;                      // return the result calculated
}


/*===========================================================================*/
/* process_file                 Process the files                            */
/*===========================================================================*/

int process_file(int depth)
{
    if (depth == 0) return 0;

    int sum = 0;
    int pid = fork();
    
    // if map failed
    if (shared_mem == MAP_FAILED)
    {
        report_error("Map Failed");
        return -1;
    }

    // if fork failed
    if (pid < 0)
    {
        report_error("Forked Failed");
        return -1;
    }

    // if child process
    if (pid == 0)
    {
        if (process_file(depth - 1) == -1) return -1;

        printf("\nProcessing '%s'...\n", argv[depth]);
        sum = calculate_matrix_sum(argv[depth], n);
        *shared_mem += sum;

        exit(0);
    }

    // if parent process
    else
    {
        wait(NULL);
    }

    return *shared_mem;
}