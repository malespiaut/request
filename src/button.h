/*
button.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef BUTTON_H
#define BUTTON_H

#define MAX_BUTTONS 200

typedef struct button_s
{
   int used;
   int level;

   int x,y;
   int sx,sy;
   int flags;
   int type;
   int on;
   void *data;
} button_t;

// Button types
#define B_TEXT 0
#define B_PIC  1

// Button flags
#define B_RAPID    1   // Don't wait for user to let go of mouse button
#define B_TOGGLE   2   // A button you can toggle

#define B_ENTER    4   /* Pretend this button was hit when you press enter */
#define B_ESCAPE   8   /* Pretend this button was hit when you press escape */

extern button_t button[MAX_BUTTONS];

extern int lasthit;

int AddButtonPic(int x,int y,int flags,const char *name);

int AddButtonText(int x,int y,int flags,const char *text);

void RemoveButton(int i);

void MoveButton(int i,int x,int y);

void ToggleButton(int i,int on);

void DrawButton(int i);

void EraseButton(int i);

void DrawButtons(void);

void PushButtons(void);

void PopButtons(void);

void RefreshButton(int i);

int UpdateButtons(void);

void InitButton(void);

#endif

