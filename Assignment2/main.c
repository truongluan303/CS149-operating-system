/**
 * @file summatrix_parallel.c
 * 
 * @author Hoang (Luan) Truong, Shubham Goswami
 * 
 * @date 2022-02-27
 */


#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>


//===========================================================================//
//============================ Functions Prototypes =========================//
//===========================================================================//

/**
 * @brief Check if the input entered is valid
 * @param argc number of arguments entered
 * @param argv the arguments entered
 * @return true if all inputs entered are valid
 */
int validate_input(int argc, char** argv);

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
 * @brief Print an error message on the console
 * @param message the error message to be printed
 */
void print_error(char* message);

/**
 * @brief Calculate the sum of all non-negative numbers in the
 *        matrix in a given text file. If the number of columns
 *        in the matrix exceeds the given number n, then the
 *        calculation will stop at column (n)th
 * @param filepath  the path to the file containing the matrix
 * @param n         the number of columns to calculate up to
 * @return the sum calculated
 */
int calculate_matrix_sum(char* filepath, int n);



//===========================================================================//
//                                Main Function                              //
//===========================================================================//

int main(int argc, char** args)
{
    // check if the command line input is valid
    int ret_code = validate_input(argc, args);
    if (ret_code != 0)
    {
        return ret_code;
    }
    return 0;
}



//===========================================================================//
//============================ Functions Definitions ========================//
//===========================================================================//



int validate_input(int argc, char** argv)
{
    char* error_message;
    int return_code = 0;

    // if not enough arguments
    if (argc < 3)
    {
        error_message = "Error: Not enough input arguments\n";
    }
    // if number of arguments is valid
    else
    {
        // check whether the file's extension is txt
        for (unsigned short i = 0; i < argc - 1; i++)
        {
            char *filepath = *(argv + i);
            if (strcmp(get_file_extension(filepath), "txt") != 0) {
                return_code = 1;
                error_message = "Error: An argument input is not a text file\n";
            }
        }
        // check whether the given N parameter is a valid non-negative integer
        char* n = *(argv + argc - 1);
        for (unsigned short i = 0; n[i] != '\0'; i++)
        {
            if (isdigit(n[i]) == 0)
            {
                return_code = 1;
                error_message = "Error: N parameter is not valid\n";
            }
        }
    }
    // print error message if not valid
    if (return_code != 0)
    {
        print_error(error_message);
    }
    return return_code;
}



const char* get_file_extension(const char* filepath)
{
    const char *dot = strrchr(filepath, '.');
    if (!dot || dot == filepath)
    {
        return "";
    }
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



void print_error(char* message)
{
    printf("\033[1;31m");           // change text color to red
    printf("%s", message);          // print the error message
    printf("\033[0m");              // reset text color
}



int calculate_matrix_sum(char* filepath, int n)
{
    FILE* file;                             // the input file
    file = fopen(filepath, "r");            // try open the given file
    // if the file cannot be opened, print error message and exit with code 1
    if (file == NULL)
    {
        print_error("Range: cannot open file");
        exit(1);
    }
    unsigned int result = 0;                // the sum to be calculated
    unsigned int count = 0;                 // keep track of the amount of numbers on a line
    unsigned int row = 1;                   // current row/line
    int num;                                // contains the current number read
    char ch = 0;                            // contains the char read
    bool ignore = false;                    // ignore the number read if true

    while (ch != EOF)
    {
        if (isdigit(ch) || ch == '-')       // if is ch is a number,
        {                                   
            ungetc(ch, file);               // then push ch back and scan the whole number
            fscanf(file, "%d", &num);

            if (!ignore)                    // skip if ignore is set
            {
                if (num < 0)
                {
                    print_warning(num, row);
                }
                else
                {
                    result += num;
                }
            }
            // if there are more nums on the line than input N,
            // ignore the remaining nums on that line
            if (++count >= n)
            {
                ignore = true;
            }
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