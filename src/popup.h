/*
popup.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef POPUP_H
#define POPUP_H

void Popup_Init(int x,int y);

void Popup_AddStr(const char *format, ...) __attribute__ ((format(printf,1,2)));

int Popup_Display(void);

void Popup_Free(void);

#endif

