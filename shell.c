/* Name: Jordan Mata
 * Class: San Diego State University
 *        CS570 - Operating Systems
 *        Professor Carroll
 * Description: Rudimentary shell that will prompt user for input and execute
 *              the input command as with a standard unix shell.
 */

#include "shell.h"

int main(){
    //Declare locals
    int length;
    int waitReturn;
    int childPID;
    int nullInputFD;
    char workingDir[BUFF_SIZE] = "";

    // setup signal catcher and a new PID for our shell
    (void) signal(SIGTERM, myhandler);
    setpgid(0, 0);
    
    while( 1 ){
        reset();

        printf("%s:Jordan's Shell: ", workingDir);
        length = parse(); // read input and set global flags as needed        
       
        // stop reading if first word is EOF
        if( length == TERMINATE ) break;
        
        // length isn't useful
        length = abs(length);
        
        // ignore 0 length words
        if( length == 0 ) continue;
   
        //handle builtin cd
        if ( newArgv[0] != NULL && strcmp(newArgv[0], "cd") == 0 ){
            char* dir = getenv("HOME"); // directory is home by default
            // cd expects 0-1 arguments
            if ( length > 2 && !backgroundFlag ){ 
                fprintf(stderr, "Too many arguments given for command cd.\n");
                continue;
            }
            
            // if filename provided means we want to go to a specific directory
            if (newArgv[1] != NULL) dir = newArgv[1];

            if (chdir(dir) != 0){
                fprintf( stderr, "cd command failed! Check if directory name "\
                                                                "is valid.\n");
            }
            // get the tail of the new path to print before the prompt
            else getWorkingDir( workingDir );
            // continue to the next command
            continue; 
        }
        // handle builtin environ
        else if(newArgv[0] != NULL && strcmp(newArgv[0], "environ") == 0){
            // environ expects 1-2 arguments
            if ((length < 2 || length > 3) && 
                                !backgroundFlag){
                fprintf(stderr, "Too many arguments given for command "\
                                                           "environ.\n");
                continue;
            }
            // print the environment variable
            if (length == 2 || (length == 3 && backgroundFlag)){ 
                char* dir = getenv(newArgv[1]); 
                if (dir == NULL) dir = ""; // env variable doesn't exist
                printf("%s\n", dir);
            }
            // set/replace an environment variable
            if (length == 3 && !backgroundFlag) setenv(newArgv[1], newArgv[2], 1);
            // read next command
            continue;
        }
        
        // child shouldn't inherit stdout/stderr of parent. Avoid printing twice
        fflush(stdout);
        fflush(stderr);
        
        if((childPID=fork()) == -1){
            fprintf(stderr, "Failed to fork a child process!\n");
        }
        // child forks off and does the command processing
        else if (childPID == 0){
            
            // background tasks should ignore user input and exec their task
            if(backgroundFlag){
                // open dev null
                if ((nullInputFD=open(DEV_NULL, READFLAGS, RPERMISSION)) < 0){
                    fprintf( stderr, "Could not open %s to avoid background " \
                                    "job reading user input.\n", inputFile);
                    exit(-3);
                }
                // set stdin to dev null
                if (dup2(nullInputFD, STDIN_FILENO) < 0){
                    fprintf(stderr, "Failed to change background process' "\
                                        "input file descriptor to /dev/null.");
                    exit(-2);
                }
            }

            if (pipeFlag){
                // recursive pipeline, kill child if it fails
                if (pipeHandler() < 0) exit(-6);
            }

            else if (inputFile != NULL){
                // open the input file
                if ((input_fd=open(inputFile, READFLAGS, RPERMISSION)) < 0){
                    fprintf( stderr, "Could not open file: %s\n", inputFile);
                    exit(-3);
                }
                // set output of the child to be the file opened
                if (dup2(input_fd, STDIN_FILENO) < 0){
                    fprintf( stderr, "Could not create input file "\
                                                            "descriptor!\n");
                    exit(-2);
                }
            }
        
            if (outputFile != NULL){
                // error if file already exists. We shouldn't overwrite it
                if (access(outputFile, F_OK) == 0){
                    fprintf(stderr, "%s: File exists.\n", outputFile);
                    exit(-1);
                }
                // open the output file
                if ((output_fd=open(outputFile, WRITEFLAGS, WPERMISSION)) < 0){
                    fprintf( stderr, "Could not open file: %s\n", outputFile);
                    exit(-3);
                }
                // set output of the child to be the file opened
                else if (dup2(output_fd, STDOUT_FILENO) < 0){
                    fprintf( stderr, "Could not create output file "\
                                                            "descriptor!\n");
                    exit(-2);
                }
            }
            
            // if no valid command to execute
            if (newArgv[pipeOffsets[numPipes-1]] == NULL){
                fprintf(stderr, "Could not find null binary.\n");
                exit(-4);
            }
            
            // run the command
            if (execvp(newArgv[pipeOffsets[numPipes-1]], 
                        newArgv+pipeOffsets[numPipes-1]) != 0){
                fprintf( stderr, "Could not run binary: %s\n", bigBuf );
                exit(-5);
            }
        }
        
        // this is the parent. If not a background task, wait for child
        else if (!backgroundFlag){
            while(waitReturn != childPID){
                if ((waitReturn=wait(NULL)) < 0){
                    fprintf( stderr, "Failed to wait for child task!");
                }
            }
        }
        
        // background tasks just print PID of child and reprompt
        else printf("%s [%d]\n", *newArgv, childPID);
    }
    
    // make sure the all subprocesses are killed when we terminate
    killpg(getpgrp(), SIGTERM);
    printf("Shell terminated.\n");
    exit(0);
}



int parse(){
    char* charIter = bigBuf;
    int wordLen = 0;
    int numWords = 0;
    int numMetas = 0;
    bool ignoreAmpersand = false;

    while( wordLen != TERMINATE ){
        ignoreMeta = false; // assume metachars are not "escaped" by default
        wordLen = getword( charIter ); // read the word

        // if end of the line, we are done reading the line
        if (wordLen == 0) break;        
        // if escaped ampersand, don't treat it as a meta
        if (strcmp(charIter, "&") == 0 && ignoreMeta) ignoreAmpersand = true;
        // flag if we need to do redirected input/output
        if (!ignoreMeta && (strcmp(charIter, "<") == 0 || 
                            strcmp(charIter, ">") == 0 || 
                            strcmp(charIter, "<<") == 0 )){
            char* filename;
            char* delimeter;
            int fileLen = 0;
            int delimLen = 0;
            numMetas++;
            // hereis document since '<<' is read
            if (strcmp(charIter, "<<") ==  0){
                hereIsFlag = true;
                delimLen = getword(charIter+3); // delim is 3 chars after 1st <
                delimeter = charIter+3;
                if ( delimLen == 0 ){
                    fprintf(stderr, "Missing delimeter for hereis.\n");
                    return 0;
                }
                if ( delimLen < 0 ){
                    fprintf(stderr, "Cannot use a variable as a delimeter "\
                                                        "for hereis.\n");
                    return 0;
                }
                hereIsTerminator = delimeter;
                wordLen = delimLen + 3; // size of delim + "<<"
                inputFile = ""; // make non null (flag there is an in redirect)
            }

            // second char is NULL so it's a file redirect
            else{
                fileLen = getword(charIter+2); // filename is 2 chars after </>
                filename = charIter+2;

                // filename should immediately proceed a redirect
                if ( fileLen == 0 || strchr(metaChars, *(filename)) != NULL ){
                    fprintf(stderr, "Missing filename for redirect.\n");
                    return 0;
                }
            
                if ( fileLen < 0 ){
                    char * variable = getenv(filename);
                    if (variable == NULL){ // env variable doesn't exist
                        fprintf(stderr, "Invalid variable. Cannot find %s\n", 
                                    filename);
                        return 0;
                    }
                    strcpy(filename, variable); // deep copy for bigBuf to exec
                    fileLen = (int)strlen(variable);
                }
                                              
                // Assign the output file to its respective string
                if ( strcmp(charIter, ">") == 0 ){
                    if ( outputFile != NULL ){ 
                        fprintf(stderr, "Syntax error! Only one > allowed.\n");
                        return 0;
                    }
                    // mark the start of the filename
                    outputFile = filename;
                }
            
                // Assign the input file to its respective string
                else if ( strcmp(charIter, "<") == 0 ){
                    if ( inputFile != NULL ){ 
                        fprintf(stderr, "Syntax error! Only one < allowed.\n");
                        return 0;
                    }
                    // mark the start of the filename
                    inputFile = filename;
                }
                wordLen = fileLen + 2; // size of delim + redirect symbol
            }
        }
        
        // flag if we need to pipe later
        else if ( !ignoreMeta && strcmp(charIter, "|") == 0 ){
            pipeFlag= true;
            numMetas++;
            newArgv[numWords] = NULL; // null terminate this command
            numWords++; // prepare to read next command
            pipeOffsets[numPipes] = numWords; // flag start of second command
            numPipes++;
        }
        
        // save words that are nonmeta chars in newArgv
        else if ( wordLen != TERMINATE ){
            // if the word starts with a tilde, expand to the directory
            if (!ignoreMeta && charIter[0] == TILDE){
                char* directory = NULL;
                directory = extractAbsPath(charIter+1);
                if ( directory == NULL){
                    fprintf(stderr, "Invalid username. Cannot find %s.\n", 
                                                              charIter+1);
                    return 0;
                }
                strcpy(charIter, directory); // deep copy to bigBuf to exec on
                wordLen = (int)strlen(directory);
            }
            // if length is negative, it starts with a dollar (environ variable)
            if (wordLen < 0){
                char * variable = getenv(charIter);
                if (variable == NULL){
                    fprintf(stderr, "Invalid variable. Cannot find %s.\n", 
                                                                charIter);
                    return 0;
                }
                strcpy(charIter, variable); // deep copy to bigBuf to exec on
                wordLen = (int)strlen(variable);
            }
            // add the word to our array
            newArgv[numWords] = charIter;
            numWords++;
        }
        // start next word after the previous word null terminator
        charIter += (abs(wordLen) + 1);
    }

    // null terminate the command list so execvp knows where to stop
    newArgv[numWords] = NULL;

    // if second to last word is non escaped '&', this is a background job
    if (!ignoreAmpersand && numWords > 0 && 
                  newArgv[numWords-1] != NULL && 
                  strcmp(newArgv[numWords-1], "&") == 0){
        newArgv[numWords-1] = NULL; // avoid reading & as part of the exec
        backgroundFlag = true;
        numMetas++;
    }

    if (hereIsFlag) hereIsHandler();

    // metacharacters read, but no commands to be exec'd => error
    if (numWords == 0 && numMetas > 0){
        fprintf(stderr, "Could not find null binary.\n");
        return 0;
    }
    
    // end of file when TERMINATE is read
    if (wordLen == TERMINATE) return TERMINATE;
    else return numWords;
}



int pipeHandler(){
    int grandchildPID;
    int fileDescriptors[10];

    if (pipe(fileDescriptors) < 0){ // read end is idx 0, write end is idx 1
        fprintf(stderr, "Failed to open a pipe.\n");
        return -1;
    }

    if ((grandchildPID = fork()) < 0){
        fprintf(stderr, "Failed to wait for grandchild.\n");
        return -2;
    }
    // grandchild writes to the input end of the pipe and execs the command
    else if (grandchildPID == 0){
        // file may be the input of the first pipe
        if (inputFile != NULL){
            // open the input file
            if ((input_fd=open(inputFile, READFLAGS, RPERMISSION)) < 0){
                fprintf( stderr, "Could not open file: %s\n", inputFile);
                exit(-3);
            }
            // set output of the child to be the file opened
            if (dup2( input_fd, STDIN_FILENO ) < 0){
                fprintf( stderr, "Could not create input file "\
                                                        "descriptor!\n");
                exit(-2);
            }
        } 
        // this is grandchild of our shell. We change stdout to pipe's write FD
        if (dup2(fileDescriptors[1], STDOUT_FILENO) < 0){
            fprintf(stderr,"Failed to change a child's STDOUT to the pipe.\n");
            exit(-2);
        }
        // close pipe file descriptors to avoid deadlock
        if (close(fileDescriptors[0]) < 0 || close(fileDescriptors[1]) < 0){
            fprintf(stderr,"Failed to close pipe file descriptor in "\
                                                            "grandchild.\n");
            exit(-7);
        }
        // check if there's anything to exec
        if (newArgv[0] == NULL){
            fprintf(stderr, "Could not find null binary.\n");
            exit(-4);
        }
        // exec the piped command
        if (execvp(newArgv[0], newArgv) != 0){
            fprintf( stderr, "Could not run binary: %s\n", newArgv[0] );
            exit(-5);
        }
    }
    // child reads the output end of the pipe
    else{
        // this is child of our shell. We change stdin to pipe's read FD
        if (dup2(fileDescriptors[0], STDIN_FILENO) < 0){
            fprintf(stderr,"Failed to change a child's STDIN to the pipe.\n");
            exit(-2);
        }
        // close pipe file descriptors to avoid deadlock
        if (close(fileDescriptors[0]) < 0 || close(fileDescriptors[1]) < 0){
            fprintf(stderr,"Failed to close pipe file descriptor in child.\n");
            exit(-7);
        }
        // call recursive pipe as many times as needed (up to 9 times)
        if ( numPipes == 1 ) return 0;
        else repeatedPipe(fileDescriptors, 1);
    }
    return 0;
}



int repeatedPipe(int* FDArr, int pipeIter){
    int grandchildPID;
    int inFD = 2*pipeIter; // idx of in file descriptors to be created
    int outFD = 2*pipeIter+1; // idx of the out file descriptor to be created
    int commandIdx = pipeOffsets[pipeIter-1]; // idx of the string in newArgv
    
    if (pipe(FDArr+inFD) < 0){
        fprintf(stderr, "Failed to open a pipe.\n");
        return -1;
    }

    if ((grandchildPID = fork()) < 0){
        fprintf(stderr, "Failed to wait for grandchild.\n");
        return -2;
    }
    // grandchild writes to the input end of the pipe and execs the command
    else if (grandchildPID == 0){
        // this is grandchild of our shell. We change stdout to pipe's write FD
        if (dup2(FDArr[outFD], STDOUT_FILENO) < 0){
            exit(-2);
        }
        // close pipe file descriptors to avoid deadlock
        if (close(FDArr[inFD]) < 0 || close(FDArr[outFD]) < 0){
            fprintf(stderr,"Failed to close pipe file descriptor in "\
                                                            "grandchild.\n");
            exit(-7);
        }
        // check if there's anything to exec
        if (newArgv[commandIdx] == NULL){
            fprintf(stderr, "Could not find null binary.\n");
            exit(-4);
        }
        // exec the piped command
        if (execvp(newArgv[commandIdx], newArgv+commandIdx ) != 0){
            fprintf(stderr, "Could not run binary: %s\n", newArgv[commandIdx]);
            exit(-5);
        }
    }
    // child reads the output end of the pipe
    else{
        // this is child of our shell. We change stdin to pipe's read FD
        if (dup2(FDArr[inFD], STDIN_FILENO) < 0){
            fprintf(stderr,"Failed to change a child's STDIN to the pipe.\n");
            exit(-2);
        }
        // close pipe file descriptors to avoid deadlock
        if (close(FDArr[inFD]) < 0 || close(FDArr[outFD]) < 0){
            fprintf(stderr,"Failed to close pipe file descriptor in child.\n");
            exit(-7);
        }
        // check if need to call the recursive pipe again.
        if (pipeIter >= numPipes-1) return 0;
        else repeatedPipe(FDArr, pipeIter+1);
    }
    return 0;
}



void reset(){
    int i;
    for (i = 0; i < 10; i++){ 
        pipeOffsets[i]=0;
    }
    hereIsTerminator = NULL;
    inputFile = NULL;
    outputFile = NULL;
    input_fd = -1;
    output_fd = -1;
    numPipes = 0;
    pipeFlag = false,
    backgroundFlag = false;
    dollarFlag = false;
    hereIsFlag = false;
    if ( access(TMP_FILE, F_OK) == 0 ){
        unlink(TMP_FILE);
    }
}



void getWorkingDir( char* destination ){
    char temp[BUFF_SIZE];
    char* workingDir = getcwd(temp, BUFF_SIZE-1);
    workingDir = basename(temp); // extract the last directory
    strcpy( destination, workingDir); // copy to avoid modifying actual path
}



char* extractAbsPath( char* username ){
    int i;
    bool found = false;
    bool extend = false;
    char* line = NULL;
    size_t size;
    char* iter;
    char* end = ""; // there is no path after the username by default
    FILE* filePtr;
    filePtr = fopen(PWD_FILE, READ_FOPEN);
    if (filePtr == NULL){
        fprintf(stderr, "Cannot open %s to read username paths.\n", PWD_FILE); 
        return NULL;
    }
    // username is part of a path, need to add back the rest
    if (strchr(username, '/') != NULL){
        extend = true; // flag that we need to add back the path
        username = dirname(username);
        end = username + strlen(username)+1;
    } 
    // find the correct user (match 'directory' to first column)
    while ((int)getline(&line, &size, filePtr) != -1){
        char copy[BUFF_SIZE];
        strcpy(copy, line);
        // extract the first column from the copy
        if (strtok(copy, ":") == NULL) return NULL;
        if (strcmp(copy, username) == 0){
            found = true;
            break;
        }
    }

    if (!found) return NULL; // did not find this user
    
    // iterate to the 6th column (absolute path) of the entry
    iter = strtok(line, ":");
    for (i=0; i<5; i++){
        iter = strtok(NULL, ":");
    }
    fclose(filePtr);
    
    if (extend) strcat(iter, "/"); // add delimeter for the path
    return strcat(iter, end); // add path or empty string if no path
}



void hereIsHandler(){
    char* line = NULL;
    size_t size;
    int oldStdout = dup(STDOUT_FILENO); // save our STDOUT to restore later
    strcat(hereIsTerminator, "\n"); // user commands ended by a newline, need 
                                    // it on the terminator
    
    // check if the temp file already exists and is writable
    if (access(TMP_FILE, F_OK) == 0){
        fprintf(stderr, "Cannot write the hereis to temp file: %s\n", TMP_FILE);
    }
    // open it for writing
    if ((output_fd=open(TMP_FILE, WRITEFLAGS, WPERMISSION)) < 0){
        fprintf( stderr, "Could not open file: %s\n", TMP_FILE);
    }
    // avoid writing old buffer to temp file
    fflush(stdout);
    // set output of the child to be the file opened
    if (dup2(output_fd, STDOUT_FILENO) < 0){
        fprintf( stderr, "Could not create output file descriptor!\n");
    }
    // write user input to file line by line
    while ((int)getline(&line, &size, stdin) != -1){
        // stop writing when we see the terminator
        if(strcmp(line, hereIsTerminator) == 0) break;
        // write the line to the temp file
        else printf("%s", line);
    }
    inputFile = TMP_FILE; // set our exec to get input from the file

    // restore the stdout to the output console
    if (dup2(oldStdout, STDOUT_FILENO) < 0){
        fprintf( stderr, "Could not create output file descriptor!\n");
    }
    if (close(output_fd) < 0){
        fprintf(stderr,"Failed to close pipe file descriptor in child.\n");
    }
}



void myhandler(int signum){}
