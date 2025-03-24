/*
token.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef TOKEN_H
#define TOKEN_H

extern char token[16384];
extern int token_linenum;
extern char* token_curpos;

#define T_CFG 1 // accept # style comments
#define T_C 2   // accept // and /* */ style comments

#define T_ALLNAME 4 // parse all non-whitespace as names

#define T_NUMBER 8  // parse numbers
#define T_NAME 16   // parse names
#define T_MISC 32   // parse other stuff (punctation)
#define T_STRING 64 // parse "" strings

int TokenFile(const char* filename, int fileflags, void (*parsecomment)(char* buf));
int TokenBuf(char* buf, int fileflags, void (*parsecomment)(char* buf));
void TokenDone(void);

int TokenGet(int crossline, int tflags);
int TokenAvailable(int crossline);

#endif
