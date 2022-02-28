/**
 * @file summatrix.c
 * 
 * @author Hoang (Luan) Truong
 */


#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>



//===========================================================================//
//=========================== Functions Prototypes ==========================//
//===========================================================================//

/**
 * @brief check if the input entered is valid
 * @param argc number of arguments entered
 * @param argv the arguments entered
 * @return true if all inputs entered are valid
 */
bool validate_input(int argc, char** argv);

/**
 * @brief get the extension of a file
 * @param filepath path to the file
 * @return the given file's extension
 */
const char* get_file_extension(const char* filepath);

/**
 * @brief print a negative value warning on the console
 * @param value     the negative value
 * @param row_num   the row number where the value is on
 */
void print_warning(int value, unsigned int row_num);

/**
 * @brief print an error message on the console
 * @param message the error message to be printed
 */
void print_error(char* message);





//===========================================================================//
//                               Main Function                               //
//===========================================================================//

int main(int argc, char** argv)
{
    // check if inputs arguments are valid
    if (!validate_input(argc, argv))
    {
        return 1;
    }

    // get the input arguments and convert N to integer
    char* filename = *(argv + 1);
    int n = (int)strtol(*(argv + 2), (char**)NULL, 10);

    FILE* file;                             // the input file
    
    file = fopen(filename, "r");            // try open the given file
    if (file == NULL)                       // show error and return 1 if the unable to open
    {
        print_error("Error: Unable to open the given file");
        return 1;
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
            // if there are more nums on line than input N, ignore the remaining nums on line
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

    printf("\nSum: %d\n", result);      // print out the result
    return 0;
}





//===========================================================================//
//=========================== Functions Definitions =========================//
//===========================================================================//



bool validate_input(int argc, char** argv)
{
    char* error_message;
    bool is_valid = true;

    // if not enough arguments
    if (argc < 3)
    {
        is_valid = false;
        error_message = "Error: Not enough input arguments\n";
    }
    // if too many arguments
    else if (argc > 3)
    {
        is_valid = false;
        error_message = "Error: Too many input arguments\n";
    }
    // if number of arguments is valid
    else
    {
        char* filepath = *(argv + 1);
        char* n = *(argv + 2);

        // check whether the file's extension is txt
        if (strcmp(get_file_extension(filepath), "txt") != 0)
        {
            is_valid = false;
            error_message = "Error: Given file is not a text file\n";
        }
        // check whether the given N parameter is a valid non-negative integer
        else
        {
            for (unsigned short i = 0; n[i] != '\0'; i++)
            {
                if (isdigit(n[i]) == 0)
                {
                    is_valid = false;
                    error_message = "Error: N parameter is not valid\n";
                }
            }
        }
    }
    if (is_valid)
    {
        return true;
    }
    // print error message if not valid
    print_error(error_message);
    return false;
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
    printf("\033[1;33m");       // change text color to yellow
    printf("Warning: value");
    printf("\033[1;31m");       // change text color to red
    printf(" %d ", value);
    printf("\033[1;33m");       // change text color to yellow
    printf("found on row %d\n", row_num);
    printf("\033[0m");          // reset text color
}



void print_error(char* message)
{
    printf("\033[1;31m");           // change text color to red
    printf("%s", message);          // print the error message
    printf("\033[0m");              // reset text color
}
