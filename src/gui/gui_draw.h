#ifndef GUI_DRAW_H
#define GUI_DRAW_H

#define BG_COLOR 6

void GUI_HLine(int y,int x1,int x2,int col);
void GUI_VLine(int x,int y1,int y2,int col);

void GUI_Frame(int x1,int y1,int x2,int y2);

void GUI_Box(int x1,int y1,int x2,int y2,int col);
void GUI_SolidBox(int x1,int y1,int x2,int y2,int col);

#endif

