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
#include <pthread.h>

#define FILES_NO        3           /* Number of input files. */
#define ERROR           -1          /* A number flagged as erroneous. */
#define ERROR_COLOR     ""          /* Console color for error messages. */
#define WARNING_COLOR   ""          /* Console color for warning messages. */
#define SUCCESS_COLOR   ""          /* Console color for success messages. */
#define INFO_COLOR      ""          /* Console color for info messages. */
#define RESET_COLOR     ""          /* Reset the console color */

/*---------------------------------------------------------------------------*/
/* Global variables                                                          */
/*---------------------------------------------------------------------------*/
bool            efound  = false;    /* Flag if an error is encountered. */
unsigned long   msum    = 0;        /* The result matrix sum. */
size_t          n;                  /* Number of columns to read up to. */
char**          files[FILES_NO];    /* List of files to be read. */
pthread_t       tids[FILES_NO];     /* IDs of the threads. */
pthread_mutex_t locks[FILES_NO];    /* Thread locks. */

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
    if (argc != (FILES_NO + 2)) {
        printf(
            "%sError: Invalid number of arguments. Expected %d, got %d\n%s",
            ERROR_COLOR,
            FILES_NO + 2,
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
void* calc_matrix_sum(filepath)

    char*       filepath;           /* Path to the file. */

{
    FILE* file;
    /*
    --  If the file does not exist, simply print error message and return.
    */
    if ((file = fopen(filepath, "r")) == NULL) {
		printf(
			"%sError: File not found!\n%s",
			ERROR_COLOR,
			RESET_COLOR
		);
        efound = true;              /* Flag that an error is encountered. */
        pthread_exit(NULL);         /* Exit the thread. */
        return NULL;
    }
    size_t  count   = 0;            /* Count the no. of numbers on a line. */
    size_t  row     = 1;            /* current row/line. */
    int     num;                    /* Contains the current number read. */
    char    ch      = 0;            /* Contains the char read. */
    bool    ignore  = false;        /* Ignore the number read if true. */

    while (ch != EOF) {
        /*
        --  If `ch` is a digit, it means we have encountered a new number.
        --  Consider adding this new number to `result`
        */
        if (isdigit(ch) || ch == '-') {                    
            // Push ch back & scan the whole number.        
            ungetc(ch, file);
            fscanf(file, "%d", &num);
            /*
            --  Only read in the number if ignore flag is not set.
            */
            if (!ignore) {
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
                    msum += num;
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
            row++;                  /* Increase the row number. */
            count = 0;              /* Reset the numbers counted. */
            ignore = false;         /* Reset ignore flag. */
        }
    }
    fclose(file);                   /* Close the file. */
    pthread_exit(NULL);             /* Exit the thread. */
    return NULL;
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
    --  Validate the input arguments.
    */
    validate_input(argc, argv);
    /*
    --  Ignore the program name.
    */
    argc--;
    argv++;

    unsigned short  i;              /* Used as loop index. */

    /*
    --  Initialize global variables.
    */
    n = (int)strtol(argv[argc - 1], (char**)NULL, 10);

    /*
    --  Initialize the locks and create the threads.
    */
	for (i = 0; i < FILES_NO; ++i) {
        if (efound) {
            continue;
        }
        if (pthread_mutex_init(&locks[i], NULL) != 0) {
            efound = true;
            continue;
        }
        printf("Creating thread #%d...\n", i + 1);
		pthread_create(
			&tids[i],
			NULL,
			calc_matrix_sum,
            *(argv + i)
		);
	}
    /*
    --  Wait for the threads.
    */
    for (i = 0; i < FILES_NO; ++i) {
        if (tids[i] == NULL) {
            continue;
        }
        printf("Waiting for thread #%d...\n", i + 1);
        pthread_join(tids[i], NULL);
        printf("Thread #%d exited!\n", i + 1);
    }

    if (efound) {
        return EXIT_FAILURE;
    }
    printf("\nThe matrix sum is: %lu\n\n", msum);

    return EXIT_SUCCESS;
}
