/*
error.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef ERROR_H
#define ERROR_H

void HandleError(const char *proc, const char *format, ...) __attribute__ ((format(printf,2,3)));

//  Like HandleError, but will terminate the program after attempting to
// save the current map to 'q_backup.map'.
void Abort(const char *proc,const char *format, ...) __attribute__ ((noreturn,format(printf,2,3)));

#endif

