/******************************************************************************
 * 
 * @file    mem_tracer.c
 * 
 * @author  Luan Truong, Shubham Goswami
 * 
 * @date    04/11/2022
 * 
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

#define MAX_NUM_LINES   1024
#define MAX_LINE_LENGTH 10

#define main_realloc(a,b) REALLOC(a, b, __FILE__, __LINE__, __FUNCTION__)
#define main_malloc(a) MALLOC(a ,__FILE__, __LINE__, __FUNCTION__)
#define main_free(a) FREE(a, __FILE__, __LINE__, __FUNCTION__)


            /*********************************************/
            /*                                           */
            /*             Function Prototypes           */
            /*                                           */
            /*********************************************/

bool validate_input(int argc, char** argv);

const char* get_file_extension(const char* path);

void print_warning(int value, unsigned int row_num);

void report_error(const char* message, bool exit_program);

char* PRINT_TRACE();

void* REALLOC(void* p, int t, char* file, int line, const char* function);

void* MALLOC(int t,char* file,int line,const char* function);

void FREE(void* p,char* file,int line, const char* function);


            /*********************************************/
            /*                                           */
            /*                Stack Trace                */
            /*                                           */
            /*********************************************/

/*===========================================================================*/
/* TRACE_NODE_STRUCT            The node to form the stack                   */
/*===========================================================================*/

struct TRACE_NODE_STRUCT
{
    char* functionid;                       // pointer to function identifier
    struct TRACE_NODE_STRUCT* next;         // pointer to next node
};

typedef struct TRACE_NODE_STRUCT TRACE_NODE;

static TRACE_NODE* TRACE_TOP = NULL;        // the top of the stack

/*===========================================================================*/
/* PUSH_TRACE                   Add to the top of the stack                  */
/*===========================================================================*/

void PUSH_TRACE(char* p)
{
    TRACE_NODE*     tnode;
    static char     glob[] = "global";

    if (TRACE_TOP == NULL) 
    {
        TRACE_TOP = (TRACE_NODE*) malloc(sizeof(TRACE_NODE));
        if (TRACE_TOP == NULL)
        {
            report_error("PUSH_TRACE: memory allocation error\n", true);
        }
        TRACE_TOP->functionid = glob;
        TRACE_TOP->next = NULL;
    }

    // create the node for p
    tnode = (TRACE_NODE*) malloc(sizeof(TRACE_NODE));

    if (tnode == NULL) 
    {
        report_error("PUSH_TRACE: memory allocation error\n", true);
    }

    tnode->functionid   = p;
    tnode->next         = TRACE_TOP;        // prepend fnode
    TRACE_TOP           = tnode;            // TRACE_TOP points to the head
}

/*===========================================================================*/
/* POP_TRACE                    Pop out the top of the stack                 */
/*===========================================================================*/

void POP_TRACE() 
{
   TRACE_NODE* temp;
   temp = TRACE_TOP;                        // set temp to be the top node
   TRACE_TOP = temp->next;                  // remove the top node
   free(temp);                              // deallocate
}

/*===========================================================================*/
/* PRINT_TRACE                  Prints out the sequence of function calls    */
/*                              that are on the stack at this instance       */
/*===========================================================================*/

char* PRINT_TRACE() 
{
    int         depth = 50;
    int         i, length, j;
    TRACE_NODE* current;
    static char buf[100];

    if (TRACE_TOP == NULL) 
    {
        strcpy(buf, "global");
        return buf;
    }
    sprintf(buf, "%s", TRACE_TOP->functionid);

    length = strlen(buf);

    for(i = 1, current = TRACE_TOP->next; current != NULL && i < depth; 
        i++, current = current->next) 
    {
        j = strlen(current->functionid);
        if (length + j + 1 < 100) 
        {
            sprintf(buf + length, ":%s", current->functionid);
            length += j + 1;
        }
        else 
        {
            break;
        }
    }
    return buf;
}


            /*********************************************/
            /*                                           */
            /*                Linked List                */
            /*                                           */
            /*********************************************/

/*===========================================================================*/
/* SINGLY_LINKED_NODE           The node for a singly linked list            */
/*===========================================================================*/

struct SINGLY_LINKED_NODE 
{
    char* line;                             // pointer to function identifier
    unsigned int index;
    struct SINGLY_LINKED_NODE* next;        // pointer to next node
};

typedef struct SINGLY_LINKED_NODE NODE;

NODE* head = NULL;                          // head of the linked list
NODE* tail = NULL;                          // tail of the linked list

/*===========================================================================*/
/* add_to_list                  Add a new string to the linked list          */
/*===========================================================================*/

void add_to_list(char* line, int index)
{
    NODE *nnode  = (NODE*) malloc(sizeof(NODE));        // new node
    nnode->line  = (char*) malloc(strlen(line) + 1);    // initialize data

    memset(nnode->line, '\0', strlen(line) + 1);

    strncpy(nnode->line, line, strlen(line) + 1);
    nnode->index = index;
    nnode->next  = NULL;

    if (head == NULL)
    {
        head = nnode;
        tail = nnode;
    }
    else            // append the new node
    {
        tail->next  = nnode;
        tail        = tail->next;
    }
}

/*===========================================================================*/
/* print_list                   Print the values in the linked list          */
/*===========================================================================*/

void print_list(NODE* head) 
{
    printf("\nThe content of the linked list:\n");
    NODE* current = head;
    while (current)
    {
        printf("\tIndex: %d\tLine: %s", current->index, current->line);
        current = current->next;
    }
}

/*===========================================================================*/
/* free_list                    Free the memory in the list                  */
/*===========================================================================*/

void free_list() 
{
    NODE* current = head;
    while (current)
    {
        NODE* next_node = current->next;    // save the next node
        free(current->line);                // free current node's content
        free(current);                      // free current node
        current = next_node;                // move on to the next node
    }
}


            /*********************************************/
            /*                                           */
            /*                   M A I N                 */
            /*                                           */
            /*********************************************/

int main(int argc, char **argv) 
{
    // make sure the input arguments are valid
    validate_input(argc, argv);

    char            lineread[MAX_NUM_LINES];
    char**          array;
    FILE*           fptr    = fopen(argv[1], "r");
    unsigned int    i       = 0;
    unsigned int    size    = 0;

    if (fptr == NULL)
    {
        report_error("Cannot open input file", true);
    }

    printf("Program Started...\n");

    int fdesc = open("memtrace.out", O_WRONLY|O_CREAT|O_TRUNC);

    dup2(fdesc, 1);

    array = main_malloc(MAX_NUM_LINES * sizeof(char*));

    for (i = 0; i < MAX_NUM_LINES; i++) 
    {
        array[i] = main_malloc(MAX_LINE_LENGTH * sizeof(char));
    }

    while((fgets (lineread, MAX_NUM_LINES, fptr)) != NULL) 
    {
        int len = strlen(lineread);

        if(size >= MAX_NUM_LINES)
        {
            array        = main_realloc(array, size + 1);
            array[size]  = main_malloc(len * sizeof(char));
        }
        else if (len > MAX_LINE_LENGTH) 
        {
            array[size] = main_realloc(array[i], MAX_LINE_LENGTH + len);
        }
        strncpy(array[size], lineread, len);
        add_to_list(lineread, size);
        size++;
    }

    array = main_realloc(array, ((MAX_NUM_LINES+1) * sizeof(char *)));
    array[MAX_LINE_LENGTH] = main_malloc(MAX_LINE_LENGTH * sizeof(char));

    close(fdesc);
    fclose(fptr);

    print_list(head);   // print the linked list
    free_list();        // free the linked list memory

    // print the array
    printf("\n\nArray Content:\n");
    for (i = 0; i < size; i++) 
    {
        printf("\t%d: %s", i, array[i]);
    }

    // free the array
    for (i = 0; i < size; i++) 
    {
        main_free(array[i]);
    }
    main_free(array);

    printf("Program Finished!");
    return 0;
}


            /*********************************************/
            /*                                           */
            /*            Function Definitions           */
            /*                                           */
            /*********************************************/

/*===========================================================================*/
/* REALLOC                      calls realloc                                */
/*===========================================================================*/

void* REALLOC(void* p, int t, char* file, int line, const char* function )
{
   printf("File %s, line %d, function %s reallocated the " 
          "memory at %p to a new size %d\n", 
          file, line, function, p, t);

   p = realloc(p,t);
   printf("FUNCTION STACK TRACE: %s\n", PRINT_TRACE());
   return p;
}

/*===========================================================================*/
/* MALLOC                       calls malloc                                 */
/*===========================================================================*/

void* MALLOC(int t,char* file,int line,const char* function) 
{
    void* p = malloc(t);
    printf("File %s, line %d, function %s allocated new memory segment "
           "at %p to size %d\n", 
           file, line, function, p, t);
    printf("FUNCTION STACK TRACE: %s\n", PRINT_TRACE());
    return p;
}

/*===========================================================================*/
/* FREE                         calls free                                   */
/*===========================================================================*/

void FREE(void* p,char* file,int line, const char* function)
{
    printf("File %s, line %d, function %s deallocated the memory segment "
           "at %p\n", 
           file, line, function, p);
    free(p);
    printf("FUNCTION STACK TRACE: %s\n", PRINT_TRACE());
}

/*===========================================================================*/
/* validate_input               Check if the command line input is valid     */
/*===========================================================================*/

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

/*===========================================================================*/
/* get_file_extension           Extract the extension from a given file path */
/*===========================================================================*/

const char* get_file_extension(const char* path)
{
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path) 
    {
        return "";
    }
    return dot + 1;
}

/*===========================================================================*/
/* print_warning                Print a warning message to the console       */
/*===========================================================================*/

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

/*===========================================================================*/
/* report_error                 Print an error message and optionally exit   */
/*                              program with code 1                          */
/*===========================================================================*/

void report_error(const char* message, bool exit_program)
{
    printf("\033[1;31m");                   // change text color to red
    printf("%s", message);                  // print the error message
    printf("\033[0m");                      // reset text color
    if (exit_program) 
    {
        exit(1);
    }
}
