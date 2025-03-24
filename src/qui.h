/*
qui.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef QUI_H
#define QUI_H

#define ROM_CHAR_WIDTH 11
#define ROM_CHAR_HEIGHT 16

#define MAX_NUM_WINDOWS 16
#define MAX_NUM_FONTS 5

#define MAP_WINDOW 0
#define MENU_WINDOW 1
#define MESG_WINDOW 2
#define TOOL_WINDOW 3
#define STATUS_WINDOW 4
#define POP_WINDOW_1 5
#define POP_WINDOW_2 6
#define POP_WINDOW_3 7

#define BG_COLOR 6

typedef struct
{
  unsigned char* data;
  int data_width;
  int data_height;
  int* div;
  int* width;
  int map[128];
  int num_chars;
} font_t;

typedef struct
{
  box_t pos;
  box_t size;
  int type;
} QUI_window_t;

typedef struct
{
  /* Window managing */
  int num_windows;
  QUI_window_t window[MAX_NUM_WINDOWS];
  /* Font managing */
  int num_fonts;
  int num_popups;
  int Active;
  font_t font[MAX_NUM_FONTS];
} QUI_t;

extern QUI_t Q;

extern char oldtexname[256];
extern char oldentstr[256];

/*
All the functions below automatically dither colors, so all parameters
should be in Quest's palette.
*/

void QUI_Box(int x1, int y1, int x2, int y2, int col1, int col2);

void QUI_Frame(int x1, int y1, int x2, int y2);

void QUI_DrawChar(int x, int y, int bg, int fg, int font, int to_screen, char c);

int QUI_strlen(int font, const char* string);
void QUI_DrawStr(int x, int y, int bg, int fg, int font, int to_screen, const char* format, ...) __attribute__((format(printf, 7, 8)));
void QUI_DrawStrM(int x, int y, int maxx, int bg, int fg, int font, int to_screen, const char* format, ...) __attribute__((format(printf, 8, 9)));

void QUI_InitWindow(int win);
void QUI_RedrawWindow(int win);
void QUI_PopUpWindow(int win, const char* Title, unsigned char** TempBuf);
void QUI_PopDownWindow(int win, unsigned char** TempBuf);

void QUI_Dialog(const char* title, const char* string);

int YNgood(char c);
int QUI_YesNo(const char* title, const char* string, const char* op1, const char* op2);

int QUI_PopEntity(const char* title, char** string, char** value, int number);

int QUI_LoadFontMap(const char* filename, font_t* font);
int QUI_LoadFontDiv(const char* filename, font_t* font);
int QUI_LoadFontPCX(const char* filename, font_t* font);
int QUI_RegisterFont(const char* fontloc);

int QUI_Init(void);

#endif
