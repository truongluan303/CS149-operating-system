/******************************************************************************
 * 
 * @file        summatrix_threaded.c
 * 
 * @author      Luan Truong
 * 
 * @brief       A program to compute sum of matrices contained in text files.
 * 
 * @date        2022-04-28
 * 
 * @copyright   Copyright (c) 2022
 * 
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define NUM_OF_FILES    3           /* Number of input files. */
#define ERROR           -1          /* A number flagged as erroneous. */
#define ERROR_COLOR     ""          /* Console color for error messages. */
#define WARNING_COLOR   ""          /* Console color for warning messages. */
#define RESET_COLOR     ""          /* Reset the console color */

/*---------------------------------------------------------------------------*/
/* extract_extension                Extract the extension of a file out of   */
/*                                  its path                                 */
/*---------------------------------------------------------------------------*/
const char* extract_extension(filepath)

    const char* filepath;           /* Path to the file */

{
    const char *dot = strrchr(filepath, '.');
    if (!dot || dot == filepath) return "";
    return dot + 1;
}

/*---------------------------------------------------------------------------*/
/* validate_input                   Validate the input arguments. If the     */
/*                                  input arguments are bad, print the error */
/*                                  message and exit with code 1.            */
/*---------------------------------------------------------------------------*/
void validate_input(argc, argv)

    int         argc;               /* Arguments count. */
    char**      argv;               /* Arguments vector. */

{
    if (argc != (NUM_OF_FILES + 2)) {
        printf(
            "%sError: Invalid number of arguments. Expected %d, got %d\n%s",
            ERROR_COLOR,
            NUM_OF_FILES + 2,
            argc,
            RESET_COLOR
        );
        exit(EXIT_FAILURE);
    }
    char*   lastarg = *(argv + argc - 1);
    char*   filepath;
    /*
    --  Check each file if its extension is "txt" or nothing.
    */
    for (unsigned short i = 1; i < argc - 1; i++) {
        filepath = *(argv + i);
        if (
            strcmp(extract_extension(filepath), "txt") != 0 &&
            strcmp(extract_extension(filepath), "") != 0
        ) {
            printf(
                "%sError: Given file %s is not a valid type.\n%s",
                ERROR_COLOR,
                filepath,
                RESET_COLOR
            );
            exit(EXIT_FAILURE);
        }
    }
    /*
    -- Check whether the given N parameter is a valid non-negative integer.
    */
    for (unsigned short i = 0; lastarg[i] != '\0'; i++) {
        if (isdigit(lastarg[i]) == 0) {
            printf(
                "%sError: N parameter is not valid.\n%s",
                ERROR_COLOR,
                RESET_COLOR
            );
        }
    }
}

/*---------------------------------------------------------------------------*/
/* calc_matrix_sum                  Calculate the sum of the matrix that is  */
/*                                  contained in a given file. The function  */
/*                                  will ignore all non-positive numbers.    */
/*---------------------------------------------------------------------------*/
long calc_matrix_sum(filepath, n)

    char*       filepath;           /* Path to the file. */
    size_t      n;                  /* Number of columns to progress */

{
    FILE* file;
    if ((file = fopen(filepath, "r")) == NULL) {
        
        return -1;
    }

    long    result  = 0;            /* The sum to be calculated. */
    size_t  count   = 0;            /* Count the no. of numbers on a line. */
    size_t  row     = 1;            /* current row/line. */
    int     num;                    /* Contains the current number read. */
    char    ch      = 0;            /* Contains the char read. */
    bool    ignore  = false;        /* Ignore the number read if true. */

    while (ch != EOF)
    {
        /*
        --  If `ch` is a digit, it means we have encountered a new number.
        --  Consider adding this new number to `result`
        */
        if (isdigit(ch) || ch == '-') {                    
            // push ch back & scan the whole number               
            ungetc(ch, file);
            fscanf(file, "%d", &num);
            /*
            --  Only read in the number if ignore flag is not set
            */
            if (!ignore)
            {
                if (num < 0) {
                    printf(
                        "%sWarning: Negative number %d found on line %ld "
                        "of file \"%s\".\n%s",
                        WARNING_COLOR,
                        num,
                        row,
                        filepath,
                        RESET_COLOR
                    );
                }
                else {
                    result += num;
                }
            }
            /*
            --  If there are more nums on the line than input N,
                ignore the remaining nums on that line
            */
            if (++count >= n) {
                ignore = true;
            }
        }
        ch = getc(file);
        /*
        --  If a newline char is detected, 
            reset to read in data from different row.
        */
        if (ch == '\n') {
            row++;                  // increase the row number
            count = 0;              // reset the numbers counted
            ignore = false;         // reset ignore flag
        }
    }
    fclose(file);                   // close the file
    return result;                  // return the result calculated
}

                /**********************************/
                /*                                */
                /*             M A I N            */
                /*                                */
                /**********************************/

int main(argc, argv)

    int         argc;               /* Arguments count. */
    char**      argv;               /* Arguments vector */

{
    /*
    --  Validate the input arguments
    */
    validate_input(argc, argv);
    /*
    --  Ignore the program name
    */
    argc--;
    argv++;

    return EXIT_SUCCESS;
}