/*
message.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef MESSAGE_H
#define MESSAGE_H

void InitMesgWin(void);

void UpdateMsg(void);

void DrawMessages(void);

void NewMessage(const char* format, ...) __attribute__((format(printf, 1, 2)));

void DumpMessages(void);

#endif
