#ifndef P2_H
#define P2_H

/* @Author: Jordan Mata
 * Class: San Diego State University
 *        CS570 - Operating Systems
 *        Professor Carroll
 * Description: Header files, function declarations, and constants used within
 *              our rudimentary shell. 
 */

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "getword.h"

#define MAXITEM 100 // max number of commands that can be read
#define MAXPIPES 10
#define BUFF_SIZE 255
#define TERMINATE -255

#define RPERMISSION S_IRUSR // flags to read and write for
#define READFLAGS O_RDONLY  // redirected input/output
#define WPERMISSION S_IRUSR | S_IWUSR
#define WRITEFLAGS O_RDWR | O_CREAT
#define READ_FOPEN "r"
#define DEV_NULL "/dev/null"
#define TMP_FILE "/tmp/cs570_temp_file"
#define PWD_FILE "/etc/passwd"

char bigBuf[BUFF_SIZE*MAXITEM]; // stores ALL strings read (100 words x 255 len)
char* newArgv[MAXITEM]; // stores strings that aren't metas and will be exec'd

int pipeOffsets[MAXPIPES]; // where to start second command if we are piping
int numPipes;

bool pipeFlag; // flags used to indicate if we read metas
bool backgroundFlag;
bool dollarFlag;
bool ignoreMeta;
bool hereIsFlag;

char* hereIsTerminator; // character to determine when hereis is done
char* outputFile; // input/output files to read from and write to
char* inputFile;

int input_fd; // file descriptors
int output_fd;

extern char metaChars[]; // get the list of metachars from getword.c

// C library function declarations included to make splint happy
int killpg(int pgrp, int sig);
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
int setenv(const char *name, const char *value, int overwrite);



/* Description: Simulates a unix shell that continuously prompts user for input
 *              and exec's the command. Terminates when -255 returned by parse 
 *              (when EOF is read). Will be able to handle the following 
 *              metacharacters:
 *              & - runs the task in the background if at end of the line
 *              | - pipes the output of a command to a second command (limit 1)
 *              > - pipes the output of a command to a file
 *              < - pipes the input of a file to a command
 * Parameters:  None.
 * Return:      0 on successful execution.
 * Side effect: Forked child will exit with the following codes based on the 
 *              corresponding failure:
 *              -1 = fork (pipe grandchild)
 *              -2 = dup2
 *              -3 = open (file)
 *              -4 = execvp (null binary passed in)
 *              -5 = execvp (negative return value of execvp)
 *              -6 = pipe
 */
int main();



/* Description: Reads a line of input from the user.
 * Parameters:  None.
 * Return:      Number of words read (non metachars).
 *              0 if invalid input or blank command (users will be reprompted 
 *              with helpful message).
 *              -255 when EOF is the only word read on this line.
 * Side effect: Populates the input into array bigBuf and stores pointers to
 *              the start of non metacharacters in newArgv. Sets global flags
 *              to help main program handle metachars.
 */
int parse();



/* Description: Resets global flags needed for each command line being read 
 *              from input. Ensures previous line flags aren't applied to 
 *              future command lines.
 * Parameters:  None.
 * Return:      void
 * Side effect: Deletes the file held in variable TMP_FILE that is used as an
 *              intermediary file for the hereis command.
 */
void reset();



/* Description: Handles piping if user supplied a valid pipe command. Will
 *              open a pipe and fork a child for the calling process 
 *              (grandchild of our shell). Will assign calling process' stdin 
 *              to read end of the pipe and child stdout to pipe's read end. 
 *              Both will close their file descriptors for both ends of the 
 *              pipe to avoid deadlock. Child execs the command and outputs to 
 *              calling process input via pipe. Handles the first pipe and
 *              recursively calls repeated pipe for the (num_pipes-1) pipes.
 *              Total of 10 pipes handled in total.
 * Parameters:  None.
 * Return:      int - 0 if success
 *                  - -1 if pipe failed
 *                  - -1 if fork failed.
 * Side effect: Forked grandchild will exit with the following codes based on 
 *              the corresponding failure:
 *              -2 = dup2
 *              -3 = open (the input file)
 *              -4 = execvp (null binary passed in)
 *              -5 = execvp (negative return value of execvp)
 *              -7 = close pipe file descriptors
 */
int pipeHandler();



/* Description: Handles piping if user supplied a valid pipe command. Will
 *              open a pipe and fork a child for the calling process 
 *              (grandchild of our shell). Will assign calling process' stdin 
 *              to read end of the pipe and child stdout to pipe's read end. 
 *              Both will close their file descriptors for both ends of the 
 *              pipe to avoid deadlock. Child execs the command and outputs to 
 *              calling process input via pipe. Can be called up to 9 times.
 * Parameters:  None.
 * Return:      int - 0 if success
 *                  - -1 if pipe failed
 *                  - -1 if fork failed.
 * Side effect: Forked grandchild will exit with the following codes based on 
 *              the corresponding failure:
 *              -2 = dup2
 *              -3 = open (the input file)
 *              -4 = execvp (null binary passed in)
 *              -5 = execvp (negative return value of execvp)
 *              -7 = close pipe file descriptors
 */
int repeatedPipe( int* FDArr, int pipeIter );



/* Description: Gets the tail of the directory that this process is currently
 *              in. eg, if in /home/cs/carroll/cssc0057/Two, then Two is 
 *              returned.
 * Parameters:  Destination - the string of where to write the output to.
 * Return:      void
 * Side effect: Overwrites the string input.
 */
void getWorkingDir( char* destination );



/* Description: Takes in a username input and searches the PWD_FILE 
 *              (/etc/passwd) for that entry. Extracts the 6th column
 *              for that entry (absolute path). If the username provided
 *              is part of a path (eg. ~cssc0057/someFile) then the 
 *              returned value also has the trailing path appanded
 *              (eg. /home/cs/carroll/cssc0057/someFile).
 * Parameters:  username - the string to try and find the absolute path for.
 * Return:      expanded path.
 * Side effect: None.
 */
char* extractAbsPath( char* username );



/* Description: Used to handle the hereis document. Will keep reading
 *              input from the user and output it to a new file TMP_FILE 
 *              (see p2.h). Will open the temp file, and write to it until the
 *              global variable hereIsTerminator is found.
 * Parameters:  None.
 * Return:      void
 * Side effect: appends a newline to the hereIsTerminator.
 */
void hereIsHandler();



/* Description: Function that will catch out SIGTERM signal and do nothing as 
 *              opposed to immediately killing our program.
 * Parameters:  None.
 * Return:      void
 * Side effect: Prevents the program from being killed in the middle of a 
 *              process. Allows us to exit gracefully.
 */
void myhandler(int signum);

#endif /* P2_H */
