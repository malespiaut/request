/*
file.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef FILE_H
#define FILE_H

void DirList(char	**Files,	int *NumDirs);

void FileList(char **Files, char	*mask, int *NumFiles);

int readstring(char *string, int	x,	int y, int maxx, int maxlen,int (*isgood)(int c));

int readgname(char *string, int	x,	int y, int maxx, int maxlen);
int readfname(char *string, int	x,	int y, int maxx, int maxlen);
int readtname(char *string, int	x,	int y, int maxx, int maxlen);

void FindFile(char *fullname,const char *org_name);

#endif

