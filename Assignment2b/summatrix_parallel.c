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
#include <sys/mman.h>
#include <fcntl.h>

/*===========================================================================*/
/* Function Prototypes                                                       */
/*===========================================================================*/

/**
 * @brief Check if the input entered is valid
 * @param argc number of arguments entered
 * @param argv the arguments entered
 */
bool validate_input(unsigned int argc, char** argv);

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
 */
void report_error(const char* message);

int process_file(int argc, char** argv, int depth);

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


/*===========================================================================*/
/* Main Function                                                             */
/*===========================================================================*/
int main(int argc, char** argv)
{
    // check if the command line input is valid
    if (!validate_input(argc, argv)) return 1;

    int num_of_files = argc - 2;

    int result = process_file(argc, argv, num_of_files);

    if (result == -1) return 1;

    printf("Total sum: %d", result);

    return 0;
}


/*===========================================================================*/
/* Function Definitions                                                      */
/*===========================================================================*/

bool validate_input(unsigned int argc, char** argv)
{
    char* error_message;

    // if not enough arguments, then show error and return
    if (argc < 3)
    {
        report_error("Error: Not enough input arguments\n");
        return false;
    }

    // check whether the file's extension is txt
    for (unsigned short i = 1; i < argc - 1; i++)
    {
        char *filepath = *(argv + i);
        if (strcmp(get_file_extension(filepath), "txt") != 0)
        {
            report_error("Error: An argument input is not a text file\n");
            return false;
        }
    }
    // check whether the given N parameter is a valid non-negative integer
    char* n = *(argv + argc - 1);
    for (unsigned short i = 0; n[i] != '\0'; i++)
    {
        if (isdigit(n[i]) == 0)
        {
            report_error("Error: N parameter is not valid\n");
            return false;
        }
    }
    return true;
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

void report_error(const char* message)
{
    printf("\033[1;31m");                   // change text color to red
    printf("%s", message);
    printf("\033[0m");                      // reset text color
}

int calculate_matrix_sum(const char* filepath, unsigned int n)
{
    FILE* file;                         // the input file
    file = fopen(filepath, "r");        // try open the given file

    if (file == NULL)
    {
        report_error("Range: cannot open file");
        return -1;
    }

    unsigned int result = 0;            // the sum to be calculated
    unsigned int count = 0;             // count the amount of numbers on a line
    unsigned int row = 1;               // current row/line
    int num;                            // contains the current number read
    char ch = 0;                        // contains the char read
    bool ignore = false;                // ignore the number read if true

    while (ch != EOF)
    {
        // if `ch` is a number
        if (isdigit(ch) || ch == '-')
        {                                   
            ungetc(ch, file);           // push ch back and scan the whole number
            fscanf(file, "%d", &num);

            if (!ignore)                // skip if ignore is set
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
    fclose(file);                       // close the file
    return result;                      // return the result calculated
}

int process_file(int argc, char** argv, int depth)
{
    if (depth == 0) return 0;

    int n = (int)strtol(argv[argc - 1], (char**)NULL, 10);
    int pid = fork();

    int* shared_mem = mmap(NULL, n * sizeof(int),
                           PROT_READ|PROT_WRITE,
                           MAP_SHARED|MAP_ANONYMOUS,
                           -1, 0);
    // if fork failed
    if (pid < 0)
    {
        report_error("Forked Failed");
        return -1;
    }
    // if child process
    if (pid == 0)
    {
        int ret_val = process_file(argc, argv, depth - 1);
        if (ret_val = -1) return -1;
        int sum = calculate_matrix_sum(argv[depth], n);
        printf("%d\n", *shared_mem);
        shared_mem += sum;
    }
    // if parent process
    else
    {
        printf("parent\n");
        wait(NULL);
    }
    return *shared_mem;
}