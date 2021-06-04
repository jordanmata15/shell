/* @Author: Jordan Mata
 * Class: San Diego State University
 *        CS570 - Operating Systems
 *        Professor Carroll
 * Description: Reads input strings and outputs the length of the string. Will
 *              also write the string to an input char array.
 */

#include "getword.h"

char metaChars[] = {LESS_THAN, GREATER_THAN, PIPE_CHAR, AND_CHAR, NULL_CHAR};
char delimeters[] = {SPACE, TAB, NEWLINE, LESS_THAN, GREATER_THAN, PIPE_CHAR, 
                                                    AND_CHAR, EOF, NULL_CHAR};


int getword(char* w){
/* Function for parsing stdin input into words (see header file)*/
    
    int currIdx = 0; // we start at index 0 of the word to be read
    char currChar = EOF;
    char prevChar = EOF;
    int leadingDollarInd = 1; // words have positive length initially

    // read from the input stdin while not at end of file (EOF)
    // break from this loop when it reaches the end of a word
    while ( (currChar=getchar()) != EOF ){

        if (currIdx > 0) prevChar = w[currIdx-1];
        
        // We should end the word if no more space in the array
        if (currIdx == BUFF_SIZE-1){
            ungetc(currChar, stdin); // this char is not being appended so we 
            break;                   // need to put it back for the next call
        }
        
        // handle negated metacharacters
        if (currChar == BACKSLASH){
            ignoreMeta = true;
            currChar = getchar();
            
            // backslash + newline is treated as a space
            if (currChar == NEWLINE) currChar = SPACE;
            // all other metacharacters should be appended as a char
            else{
                appendToString(w, &currIdx, &currChar, 1);
                continue;
            }
        }
        
        // expand the home directory of current user if the word is only a tilde
        if (prevChar == TILDE && currIdx == 1 && // 1st char is tilde
                  leadingDollarInd > 0 &&        // didn't start with a '$'
                  strchr(delimeters, currChar)){ // next char is a delimeter
            currIdx--; //overwrite the tilde with the home path
            char* home = getenv("HOME");
            appendToString(w, &currIdx, home, (int)strlen(home));
            break;
        }
        
        // metacharacter was not negated and needs to be handled appropriately
        if (strchr(metaChars, currChar) != NULL){
            // we have chars before this, use the metachar as a delimeter
            // save it for the next word
            if (currIdx > 0 || leadingDollarInd < 0) ungetc(currChar, stdin);
           
            else if (currChar == LESS_THAN){
                appendToString(w, &currIdx, &currChar, 1);
                // '<<' metachar is really 2 chars
                // check if another '<' character is next to append
                if ((currChar = getchar()) == LESS_THAN){
                    appendToString(w, &currIdx, &currChar, 1);
                }
                // if not a second '<', put back the character we just read
                else ungetc(currChar, stdin);
            }
            
            // all other metachars have length 1, so append to string
            else appendToString(w, &currIdx, &currChar, 1);
            
            break; // metachars always end the word
        }
        
        // if first char of the word is '$', don't append the char and flag 
        // this as having negative length
        if (currChar == DOLLAR && currIdx == 0 && leadingDollarInd > 0){
            leadingDollarInd = -1;
            continue;
        }

        if (currChar == NEWLINE){
            // if any characters have been read up to this point, we need to 
            // make sure the newline is it's own/separate word
            if ( currIdx > 0 || leadingDollarInd < 0 ) ungetc(currChar, stdin);
            
            // newline always is the end of a word
            break;
        }
        
        // if space or tab characters encountered, this is the end of a word
        if (currChar == TAB || currChar == SPACE){
            // if string has positive length or a leading '$' was found,
            // this is the end of word
            if ( currIdx > 0 || leadingDollarInd < 0 ) break;
            // if no valid previous characters, skip past them
            else continue;
        }
        
        // no special handling needed, the char should be appended to the word
        appendToString(w, &currIdx, &currChar, 1);
    }

    // Once finished reading the word, null terminate the string
    w[currIdx] = NULL_CHAR;
    
    // check if this is actually the EOF or just end of the word
    if (currChar == EOF && currIdx <= 0){
        // leading dollar sign just before EOF, then this isn't really EOF
        if (leadingDollarInd < 0) ungetc(EOF, stdin);        
        // if no precededing dollar sign, this is truly EOF
        else return -255; // if no precededing dollar sign, this is truly EOF
    }

    // Return the signed length. Negative iff there was a leading dollar sign.
    return currIdx *= leadingDollarInd;
}

void appendToString( char* w, int* count, char* toAdd, int size ){
/* Auxilary function for copying char toAdd to string w (see header file)*/
    int i;
    for (i = 0; i < size; i++){
        w[i+(*count)] = toAdd[i]; // start w array at last empty slot (count)
    }
    (*count) += i; // record new size of w
}

