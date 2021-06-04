#ifndef GETWORD_H
#define GETWORD_H

/* @Author: Dr. John Carroll (modified by Jordan Mata)
 * getword.h - header file for the getword() function used in
 * CS570 Fall 2020
 * San Diego State University 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "shell.h" // to access the extern ignoreMeta boolean

extern bool ignoreMeta; // tells our shell if we should ignore the metachar

// constants we will use. Not really needed, but looks nicer in many cases.
#define BACKSLASH '\\'
#define DOLLAR '$'
#define NEWLINE '\n'
#define TAB '\t'
#define SPACE ' '
#define NULL_CHAR '\0'
#define LESS_THAN '<'
#define GREATER_THAN '>'
#define PIPE_CHAR '|'
#define AND_CHAR '&'
#define TILDE '~'

/* (Note: the preceding line is an ANSI C prototype statement for getword().
    It will work fine with the edoras gcc or cc compiler.)
* The getword() function gets one word from the input stream.
* It returns -255 if end-of-file is encountered;
* otherwise, it [usually] returns the number of characters in the word [but
* note the exceptions listed in the details below]
*
* INPUT: a pointer to the beginning of a character string [a character array]
* OUTPUT: the number of characters in the word (or the negative of that number)
* SIDE EFFECTS: bytes beginning at address w will be overwritten.
*   Anyone using this routine should have w pointing to an
*   available area at least STORAGE bytes long before calling getword().
*
*   IMPORTANT: unlike Program0, repeated characters are NOT to be dropped!

Upon return, the string pointed to by w contains the next word in the line from
stdin. A "word" is a string containing one metacharacter OR a string consisting
of non-metacharacters delimited by blanks, tabs, newlines, metacharacters or EOF.
The basic metacharacters for Program1 are "<", ">", "|", "&", and "<<".

The last word on a line may be terminated by a space, the newline character
OR by end-of-file.  "$", "~", and "\" also follow special rules, as
described later (unlike the other metacharacters, these are NOT delimiters).

getword() skips leading blanks AND tabs, so if getword() is called and there are
no more words on the line, then w points to an empty string. All strings,
including the empty string, will be delimited by a zero-byte (eight 0-bits),
as per the normal C convention (this delimiter is not 'counted' when determining
the length of the string that getword will report as a return value).

The backslash character "\" is special, and may change the behavior of
the character that directly follows it on the input line.  When "\" precedes
a metacharacter, that metacharacter is treated like most other characters.
(That is, the symbol will be part of a word rather than a word delimiter.)

Thus, three calls applied to the input
Null<<void
will return 4,2,4 and produce the strings "Null", "<<", "void", respectively
(because the metacharacter "<<" terminates the word "Null").

However, one call to getword() applied to the input
Null\<void
returns 9 and produces the string "Null<void".
Note that the '\' is NOT part of the resulting string!

Similarly, "\>" is treated as the [non-meta]character ">", "\<\<" is "<<",
and so on for all the metacharacters.  "\\" represents the [non-special]
character "\".  The combination "\ " should be treated as " ", and therefore
allow a space to be embedded in a word:
Null\ void
returns 9 and produces the string "Null void".  A backslash preceding a newline
makes the newline work like a space:  it remains a word terminator, but is NOT a
line terminator.  A backslash preceding any other character should simply be ignored.

The integer that getword() returns is the length of the resultant
string to which w points. There is an exception to this: If the rest of
the line consists of zero or more blanks/tabs followed by end-of-file, then w
still points to an empty string, but the returned integer is (-255).
(Also, see below regarding how '$' affects the length returned.)

Example: Suppose the input line were
Hi>there
(Assume there are two trailing tabs and two trailing blanks, followed by EOF.)
Four calls to getword(w) would return 2,1,5,-255 and fill each of the areas
pointed to by w with the strings "Hi", ">", "there", and "", respectively.

Example: Suppose the input line were
Hi there<
(Assume there are two trailing blanks, followed by a newline character.)
Four calls to getword() would return 2,5,1,0 and fill each of the
areas pointed to by w with the strings "Hi", "there", "<", and "",
respectively.  (If EOF followed the newline, then a fifth call would
produce "" and return -255.)

Note that we would obtain exactly the same results if the input line had been
    Hi   there  <
(This example has leading blanks and a newline right after the '<'.)

The specs imply that your code will have to treat spaces differently, depending
on whether they are leading spaces, or whether the space is found when you are
in the 'middle' of a word.  The '$' metacharacter has similar complexities.
If '$' is found in the 'middle' of a word, it is treated like any normal
character (thus, "123$56" will just be a six-letter word, and +6 is returned).
But when '$' is the first character of a word, the '$' is not considered
part of the word, and the return value should be negated.  Thus, the four
characters "$^*#" will return -3, rather than the usual +4.  The six
characters \$abba produces the string "$abba", and returns +5 (since
the '\' negates the effect of the '$' -- in this case, '$' is treated like
a normal character.  (By contrast, $abba returns -4, and puts ONLY "abba" into
the array.)  Consider: Why do these specs imply that the word "$" returns 0 ?

If the word scanned is longer than STORAGE-1, then getword() constructs the
string consisting of the first STORAGE-1 bytes only. (As usual, a zero-byte
is appended. The next getword() call will begin with the rest of that word.)

The metacharacter "~" causes getword to append to the buffer a string of
characters representing the path to your home directory (but we ONLY do this
if '~' is the first character in the word).  For example, if $HOME is
/home/alone, then when the single character "~" is read, it causes the
eleven characters "/home/alone" to be placed in the buffer (returning 11).
Similarly, if getword() reads "~ABCD", then "/home/aloneABCD" goes into
the buffer pointed to by w (and getword() returns 15).  But, if getword()
reads "AB~CD", then simply return the five-character string "AB~CD" (since
'~' was NOT the first character in the word).  Caution: don't hard-code
your own home directory into your code, or else your program will fail for
every user except you.  Instead, use the getenv() system call to have the
system report the correct string to copy into the buffer.

Useful manpages to consider are those for getenv(), ungetc(), and getchar(). */



/* Description: Reads characters from stdin and outputs the length of the 
 *              string read. Handles metacharacters as delimiters and strings 
 *              of length 1.
 * Parameters: char *w - pointer to the char arrary where we will write the 
 *                       input into.
 * Return: - Length of the string we are reading from stdin. If the string
 *           begins with it will '$', the first char will not be read and we
 *           will return the length as negative.
 *         - 0 if the string has no length.
 *         - -255 if the end of file is the only input.
 * Side effect: Will write the input of the stdin to the string starting at
 *              the char pointer input. If empty stirng or EOF, writes an
 *              empty string.
 */
int getword(char *w);

/* Description: Appends a string (or single char) the end of the input string.
 *              Does NOT null terminate the string since we may continue to
 *              append to w.
 * Parameters: char* w - the string that will be append to
 *              int* count - pointer to the int storing size of w so far
 *              char* toAdd - string to append
 *              int size - size of the string of toAdd
 * Return: void
 * Side Effect: Adds size to the value of the integer pointed to by count
 */
void appendToString( char* w, int* count, char* toAdd, int size );

#endif /* GETWORD_H */
