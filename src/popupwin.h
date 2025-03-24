/*
popupwin.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef POPUPWIN_H
#define POPUPWIN_H

#define LOAD_IT 0
#define SAVE_IT 1
#define WAD_IT 2
#define GROUP_LOAD 3
#define GROUP_SAVE 4
#define PTS_LOAD 5

int SelectFile(const char* title, const char* extension, const char* button_text, char* result);

int FileCmd(int ST);

void EntityPicker(char* EndWithThis, int fixsize);

void AutoBuildPopup(void);

#endif
