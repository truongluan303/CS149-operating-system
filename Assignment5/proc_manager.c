/******************************************************************************
 * 
 * @file        proc_manager.c
 * 
 * @author      Luan Truong
 *              Shubham Goswami
 * 
 * @brief       A program that tracks data on processes, restart processes,
 *              and time processes.
 * 
 * @version     0.1
 * 
 * @date        April 2022
 * 
 *****************************************************************************/

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define CONSOLE_ERROR       "\033[0;31m%s\033[0;00m"
#define CONSOLE_WARNING     "\033[0;33m%s\033[0;00m"
#define CONSOLE_SUCCESS     "\033[0;32m%s\033[0;00m"
#define RESTART_MSG         "RESTARTING..."
#define FAST_MSG            "Spawning too fast..."
#define FINISH_MSG          "Finished at %d, runtime: %d"
#define TIME_THRESHOLD      2
#define MAX_NUM_LINES       1024
#define MAX_LINE_LEN        10

                /*******************************************/
                /*                                         */
                /*            Utility Functions            */
                /*                                         */
                /*******************************************/

#define PRECISION       1000000000.0

/******************************************************************************
 * @brief   Get the elapsed time from the given time range
 * 
 * @param start_t       The start time
 * @param end_t         The end time
 * 
 * @return  The time elasped
 *****************************************************************************/
double get_elapsed_time(struct timespec start_t, struct timespec end_t) 
{
    double elapsed = 0;
    elapsed += end_t.tv_sec - start_t.tv_sec;
    elapsed += (end_t.tv_nsec - start_t.tv_nsec) / PRECISION;
    return elapsed;
}

/******************************************************************************
 * @brief   Create a duplicate of a given string.
 * 
 * @param s         The string to create duplicate for.
 * 
 * @return  The duplicated string created.
 *****************************************************************************/
char* duplicate_str(char *s)
{
    char* p = (char*) malloc(strlen(s) + 1);
    if (p != NULL) {
        strcpy(p, s);
    }
    return p;
}

/******************************************************************************
 * @brief   Get the extension/file from a file's name.
 * 
 * @param path      The path to the file.
 * 
 * @return  The extension of the given file.
 *****************************************************************************/
const char* get_file_extension(const char* path)
{
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path) {
        return "";
    }
    return dot + 1;
}

/******************************************************************************
 * @brief   Check if the input arguments are valid.
 * 
 * @param argc      The arguments count.
 * @param argv      The arguments vector.
 * 
 * @return  True if the input arguments are valid, false otherwise.
 *****************************************************************************/
void validate_input(int argc, char** argv)
{
    // if not enough arguments
    if (argc != 2)
    {
        printf(CONSOLE_ERROR, "Error: Invalid number of arguments.\n");
        exit(1);
    }
    // check whether the file's extension is txt
    char *filepath = *(argv + 1);
    if (strcmp(get_file_extension(filepath), "txt") != 0)
    {
        printf(CONSOLE_ERROR, "Error: The argument input is not a text file");
        exit(1);
    }
}

                /*******************************************/
                /*                                         */
                /*                Hash Table               */
                /*                                         */
                /*******************************************/

#define HASHSIZE        101

/******************************************************************************
 * @brief   A node to store one command's info in the hash table
 *****************************************************************************/
struct nlist {
    struct nlist*       next;       /* next entry in chain */
    int                 pid;        /* the process ID */
    size_t              index;      /* the line index in the input file */
    char*               command;    /* the command stored */
    struct timespec*    starttime;  /* the start time */
    struct timespec*    endtime;    /* the end time */
};

/******************************************************************************
 * @brief   Pointer table
 *****************************************************************************/
static struct nlist*    hashtab[HASHSIZE];
size_t                  htable_size = 0;

/******************************************************************************
 * @brief   The hash function.
 * 
 * @param pid       The process ID to be hashed.
 * 
 * @return  The hash result based on the given process ID.
 *****************************************************************************/
unsigned _hash(int pid)
{
    return pid % HASHSIZE;
}

/******************************************************************************
 * @brief   Look for the given pid in hashtab.
 * 
 * @param pid       The pid to look for.
 * 
 * @return  The node with the given pid. If no such node exists, return NULL.
 *****************************************************************************/
struct nlist* lookup(int pid)
{
    struct nlist *np;
    for (np = hashtab[_hash(pid)]; np != NULL; np = np->next) {
        if (pid == np->pid) {
            return np;  // found
        }
    }
    return NULL;        // not found
}

/******************************************************************************
 * @brief   Insert a new process ID and its command to the hash table. In case
 *          the same process ID already existed in the table, replace its
 *          command and index with the new ones.
 * 
 * @param pid           The process ID.
 * @param command       The command.
 * @param index         The index.
 * 
 * @return  The node inserted (or modified)
 *****************************************************************************/
struct nlist* insert(int pid, char* command, int index)
{
    struct nlist*   np;
    unsigned        hashval;
    /*
    --  Case 1: The pid is not found.
    --  Create the pid using malloc.
    */
    if ((np = lookup(pid)) == NULL) { 
        // create the pid
        np = (struct nlist *) malloc(sizeof(*np));
        if (np == NULL || (np->command = strdup(command)) == NULL) {
            return NULL;
        }
        hashval             = _hash(pid);
        np->next            = hashtab[hashval];
        hashtab[hashval]    = np;
        htable_size++;
   }
    /* 
    --  Case 2: The pid is already in the hashslot.
    --  Free the previous command to make room for the new one.
    */
    else {
        free((void*) np->command);
    }
    /*
    --  Assign the command and index to the node.
    */
    if ((np->command = strdup(command)) == NULL) {
        return NULL;
    }
    np->index = index;

    return np;
}

                /*******************************************/
                /*                                         */
                /*               Linked List               */
                /*                                         */
                /*******************************************/

/******************************************************************************
 * @brief   A node for the singly linked list.
 *****************************************************************************/
typedef struct SINGLY_LINKED_NODE
{
    char*                       line;   /* pointer to function identifier */
    size_t                      index;  /* the node's index */
    struct SINGLY_LINKED_NODE*  next;   /* pointer to next node */
} NODE;

size_t  llist_size = 0;                 /* the size of the linked list */
NODE*   head = NULL;                    /* head of the linked list */
NODE*   tail = NULL;                    /* tail of the linked list */

/******************************************************************************
 * @brief   Add a new element to the linked list.
 * 
 * @param line      The new string to be added
 * @param index     The index
 *****************************************************************************/
void append_to_list(char* line, size_t index)
{
    NODE *temp      = (NODE*) malloc(sizeof(NODE));
    temp->line      = (char*) malloc(strlen(line) + 1);

    memset(temp->line, '\0', strlen(line) + 1);

    strncpy(temp->line, line, strlen(line) + 1);
    temp->index     = index;
    temp->next      = NULL;

    if (head == NULL) {
        head        = temp;
        tail        = temp;
    }
    else {
        tail->next  = temp;
        tail        = temp;
    }
    llist_size++;
}

/******************************************************************************
 * @brief   Print all elements of the linked list.
 *****************************************************************************/
void print_list() 
{
    printf("\nThe content of the linked list:\n");
    NODE* current = head;
    while (current) {
        printf("\tIndex: %ld\tLine: %s", current->index, current->line);
        current = current->next;
    }
    printf("\n");
}

/******************************************************************************
 * @brief   Free the linked list's memory.
 *****************************************************************************/
void free_list() 
{
    NODE* current = head;
    while (current) {
        NODE* next_node = current->next;    // save the next node
        free(current->line);                // free current node's content
        free(current);                      // free current node
        current = next_node;                // move on to the next node
    }
    llist_size = 0;
}

                /*******************************************/
                /*                                         */
                /*                  M A I N                */
                /*                                         */
                /*******************************************/

int main(int argc, char** argv)
{
    /*
    --  Validate input arguments
    */
    validate_input(argc, argv);
    /*
    --  Ignore the program name
    */
    argc--;
    argv++;

    printf("Program Started!\n");
    printf("Reading from \"%s\"...\n", *argv);

    /*
    --  Create a linked list of commands from the file
    */
    char    line[MAX_NUM_LINES];
    FILE*   fptr = fopen(*argv, "r");

    for (size_t i = 0; fgets(line, MAX_NUM_LINES, fptr); ++i) {
        append_to_list(line, i);
    }
    print_list();

    /*
    --  Execute the commands and add them to a hash table
    */
    for (size_t i = 0; i < llist_size; ++i) {

    }

    fclose(fptr);                   // close the text file
    free_list();                    // free the linked list
    printf("Program Finished!\n");
    return EXIT_SUCCESS;
}