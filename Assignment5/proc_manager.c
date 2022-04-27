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
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define CONSOLE_ERROR       "\033[0;31m%s\033[0;00m"
#define RESTART_MSG         "RESTARTING...\n"
#define IN_TIME_MSG         "Spawning too fast!!!\n"
#define EXCEED_TIME_MSG     "Exceeded the limit time. Will be restarted...\n"
#define FINISH_MSG          "Finished at %d, runtime: %d"
#define TIME_THRESHOLD      2
#define MAX_NUM_LINES       1024
#define MAX_FNAME_LEN       15

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
char* duplicate_str(const char *s)
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

/******************************************************************************
 * @brief   Count the number of tokens of a string after being tokenized.
 * 
 * @param str       The string to be tokenized.
 * @param sep       The seperator.
 * 
 * @return  Number of tokens.
 *****************************************************************************/
size_t count_tokens(char* str, const char* sep) {
    if (str == NULL) {
        return 0;
    }
    size_t  tokcount    = 0;                    // No. of tokens.
    char*   dupstr      = duplicate_str(str);   // Duplicate of given string.
    char*   token       = strtok(dupstr, sep);  // The first token.

    // loop through each token to count the no. of tokens
    while (token) {
        token = strtok(NULL, sep);
        tokcount++;
    }
    free(dupstr);
    return tokcount;
}

/******************************************************************************
 * @brief   Remove the newline character (if exists) in a given string.
 * 
 * @param str       The string to remove newline character from.
 *****************************************************************************/
void trim_newline(char* str)
{
    size_t last = strlen(str) - 1;
    if (*str && str[last] == '\n') {
        str[last] = '\0';
    }
}

int redirect_to_file(int pid, int fd)
{
    
    char    fout[MAX_NUM_LINES];
    char*   extension;
    
    switch (fd)
    {
    case STDOUT_FILENO:
        extension = "out";
        break;
    case STDERR_FILENO:
        extension = "err";
        break;
    default:
        extension = "txt";
        break;
    }
    sprintf(fout, "%d.%s", pid, extension);
    int fdout = open(fout, O_RDWR | O_CREAT | O_APPEND);
    fchmod(
        fdout,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
    );
    if (fdout != -1) {
        dup2(fdout, fd);
    }
    return fdout;
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
    struct timespec     starttime;  /* the start time */
};

static struct nlist*    hashtab[HASHSIZE];  /* Pointer table. */
size_t                  htable_count = 0;   /* No. of elements in table */

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
    struct nlist *np = hashtab[_hash(pid)];
    while (np) {
        if (pid == np->pid) {
            return np;          // found
        }
        np = np->next;
    }
    return NULL;                // not found
}

/******************************************************************************
 * @brief   Insert a new process ID and its command to the hash table. In case
 *          the same process ID already existed in the table, replace its
 *          command and index with the new ones.
 * 
 * @param pid           The process ID.
 * @param command       The command.
 * @param index         The index.
 * @param starttime     The time the command was executed
 * 
 * @return  The node inserted (or modified)
 *****************************************************************************/
struct nlist* insert(
    int pid,
    char* command,
    int index,
    struct timespec starttime
)
{
    struct nlist*   np;
    unsigned        hashval;
    /*
    --  Case 1: The pid is not found.
    --  Create the pid using malloc.
    */
    if ((np = lookup(pid)) == NULL) {
        // create the pid
        np = (struct nlist*) malloc(sizeof(*np));
        if (np == NULL || (np->command = duplicate_str(command)) == NULL) {
            return NULL;
        }
        hashval             = _hash(pid);
        np->pid             = pid;
        np->next            = hashtab[hashval];
        hashtab[hashval]    = np;
        htable_count++;
    }
    /* 
    --  Case 2: The pid is already in the hashslot.
    --  Override the previous data with the new one.
    */
    else {
        free((void*) np->command);
        if ((np->command = duplicate_str(command)) == NULL) {
            return NULL;
        }
    }
    np->index       = index;
    np->starttime   = starttime;

    return np;
}

void free_htable()
{
    for (size_t i = 0; i < HASHSIZE; ++i) {
        struct nlist* element = hashtab[i];
        while (element) {
            struct nlist* next = element->next;     // save the next node
            free(element->command);                 // free the command
            free(element);                          // free the element
            element = next;                         // move on to next node
        }
    }
}

                /*******************************************/
                /*                                         */
                /*                  M A I N                */
                /*                                         */
                /*******************************************/

int main(int argc, char** argv)
{
    /*
    --  Validate input arguments.
    */
    validate_input(argc, argv);
    /*
    --  Ignore the program name.
    */
    argc--;
    argv++;

    printf("Reading from \"%s\"...\n", *argv);

    char                line[MAX_NUM_LINES];        // A line in the file.
    FILE*               fptr = fopen(*argv, "r");   // The text file.
    int                 pid;                        // The process ID.
    struct timespec     starttime;                  // When execution started.
    struct timespec     endtime;                    // When execution ended.
    double              elapsed;                    // The elapsed time.
    int                 status;                     // The wait status.
    int                 fdout;                      // Out file descriptor.
    int                 fderr;                      // Err file descriptor.

    /*
    --  The first loop.
    --  Read the commands in the text file.
    --  Put them into the hash table, which is used for recording each exec.
    */
    for (size_t i = 0; fgets(line, MAX_NUM_LINES, fptr); ++i) {
        /*
        --  Tokenize the line read.
        */
        trim_newline(line);
        char*   cmdline     = duplicate_str(line);
        size_t  tokcount    = count_tokens(line, " ");
        size_t  tidx        = 0;
        char*   token       = strtok(line, " ");
        char*   arglist[tokcount + 1];
        while (token) {
            arglist[tidx++] = token;
            token           = strtok(NULL, " ");
        }
        arglist[tokcount]   = NULL;

        pid = fork();
        /*
        --  If fork error.
        */
        if (pid < 0) {
            fprintf(stderr, "Fork Error!");
            exit(2);
        }
        /* 
        --  If parent process.
        */
        else if (pid > 0) {
            fdout = redirect_to_file(pid, STDOUT_FILENO);
            clock_gettime(CLOCK_MONOTONIC, &starttime);
            struct nlist* nentry = insert(pid, cmdline, i, starttime);
            dprintf(
                fdout,
                "Child %d of parent %d.\n"
                "Starting command `%s` at index %ld.\n\n",
                pid,
                getpid(),
                nentry->command,
                nentry->index
            );
            close(fdout);
        }
        /*
        --  If child process.
        */
        else {
            pid = getpid();
            redirect_to_file(pid, STDOUT_FILENO); 
            // Execute the command.
            execvp(arglist[0], arglist);
        }
        free(cmdline);
    }

    /*
    --  The second loop.
    --  Wait until everything is finished.
    --  When there is no more child process, the parent process will exit.
    */
    while ((pid = wait(&status)) >= 0) {
        if (pid <= 0) {
            continue;
        }
        struct nlist* entry = lookup(pid);

        /*
        --  Check for abnormal termination.
        */
        if (WIFEXITED(status)) {
            fderr = redirect_to_file(pid, STDERR_FILENO);
            dprintf(
                fderr,
                "Child %d exits normally with code %d\n",
                pid,
                WEXITSTATUS(status)
            );
            close(fderr);
        } 
        else if (WIFSIGNALED(status)) {
            fderr = redirect_to_file(pid, STDERR_FILENO);
            dprintf(
                fderr,
                "Child %d terminated abnormally with signal number %d\n",
                pid,
                WTERMSIG(status)
            );
            close(fderr);
        }

        clock_gettime(CLOCK_MONOTONIC, &endtime);
        elapsed = get_elapsed_time(starttime, endtime);

        /*
        --  If 2 seconds have elapsed:
        --  Restart the command in a new process.
        */
        if (elapsed > TIME_THRESHOLD) {
            int ppid = pid;
            fdout = redirect_to_file(pid, STDOUT_FILENO);
            dprintf(fdout, EXCEED_TIME_MSG);
            close(fdout);
            pid = fork();

            /*
            --  If fork error.
            */
            if (pid < 0) {
                fprintf(stderr, "Fork Error!");
                exit(2);
            }
            /*
            --  If parent process.
            */
            else if (pid > 0) {
                insert(pid, entry->command, entry->index, starttime);
            }
            /*
            --  If child process.
            */
            else {
                char*   cmd         = entry->command;
                size_t  tokcount    = count_tokens(cmd, " ");
                size_t  tidx        = 0;
                char*   token       = strtok(cmd, " ");
                char*   arglist[tokcount + 1];
                while (token) {
                    arglist[tidx++] = token;
                    token           = strtok(NULL, " ");
                }
                arglist[tokcount]   = NULL;

                pid     = getpid();
                fdout   = redirect_to_file(pid, STDOUT_FILENO);

                clock_gettime(CLOCK_MONOTONIC, &starttime);
                dprintf(fdout, RESTART_MSG);
                dprintf(
                    fdout,
                    "Child %d of parent %d.\n"
                    "Restarting command `%s` at index %ld.\n\n",
                    ppid,
                    pid,
                    entry->command,
                    entry->index
                );
                close(fdout);
                execvp(arglist[0], arglist);
            }
        }
        /*
        --  If command finished within 2 seconds:
        --  Print the in time message to file and exit.
        */
        else {
            clock_gettime(CLOCK_MONOTONIC, &endtime);
            fderr = redirect_to_file(pid, STDERR_FILENO);
            dprintf(fderr, IN_TIME_MSG);
            close(fderr);
            fdout = redirect_to_file(pid, STDOUT_FILENO);
            dprintf(
                fdout, 
                "\nStarted at: %ld\nFinished at: %ld\nElapsed time: %fs",
                entry->starttime.tv_sec,
                endtime.tv_sec,
                elapsed
            );
            close(fdout);
        }
    }

    /*
    --  Perform the exit protocols
    */
    fclose(fptr);                                       // Close text file.
    free_htable();                                      // Free hash table.
    return EXIT_SUCCESS;                                // Exit with code 0.
}
