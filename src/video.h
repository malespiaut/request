/*
video.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef VIDEO_H
#define VIDEO_H

#define  COL_WHITE  15
#define  COL_RED    31
#define  COL_GREEN  47
#define  COL_BLUE   63
#define  COL_YELLOW 79
#define  COL_PURPLE 95
#define  COL_CYAN   111
#define  COL_TAN    127
#define  COL_LBLUE  143
#define  COL_LGREEN 159
#define  COL_PINK   175


#define  RES_640x480    1
#define  RES_800x600    2
#define  RES_1024x768   3
#define  RES_1280x1024  4
#define  RES_1600x1200  5

#define MAX_RES (RES_1600x1200+1)


#define OutCode3D(x,y,z,code)            \
{                                        \
	(code)=0;                             \
	if(((x)-0.0001)>(z))  (code) |= 0x01; \
	if(((x)+0.0001)<-(z)) (code) |= 0x02; \
	if(((y)-0.0001)>(z))  (code) |= 0x04; \
	if(((y)+0.0001)<-(z)) (code) |= 0x08; \
	if(((z)-0.0001)>1)    (code) |= 0x10; \
	if(((z)+0.0001)<0)    (code) |= 0x20; \
}

typedef struct
{
	unsigned char *ScreenBuffer;
	int            ScreenBufferSize;
	int            ScreenWidth;
	int            ScreenHeight;
	unsigned char  pal[768];
} video_t;

extern video_t video;


void DrawDot(int vport,svec_t v,int color);
void DrawVertex(int vport,svec_t v,int color);
void DrawBox(int vport,int x,int y,int size,int color);
void DrawSolidBox(int x1,int y1,int x2,int y2,int color);
void DrawSolidSquare(int x,int y,int size,int color);

int OutCode2D(int vport_num,int x,int y);

void ClipDrawLine2D(int vport_num,int x0,int y0,int x1,int y1,int color);
void ClipDrawLine3D(int vport_num,float x0,float y0,float z0,float x1,float y1,float z1,int color);

void Clip2D(int vport_num,int x0,int y0,int x1,int y1,int *x2,int *y2,int code);
void Clip3D(float x0,float y0,float z0,float x1,float y1,float z1,float *x2,float *y2,float *z2,float *t,int code);

void Draw2DSpecialLine(int vport,int x0,int y0,int x1,int y1,int color);

void DrawLine(int x0,int y0,int x1,int y1,int color);


void DrawArrow2D(int vport,int x0,int y0,int x1,int y1,int col);
void DrawArrow3D(int vport,float x0,float y0,float z0,
                           float x1,float y1,float z1,int col);


/*
The following definitions are system dependant stuff. They AREN'T implemented
in 'video.c'. They should be in some other file.
*/

#define RES_TEXT 0

int InitVideo(int vid_mode);
void DisposeVideo(void);

void SetMode(int vid_mode);

void RefreshPart(int x1,int y1,int x2,int y2);
void RefreshScreen(void);

void WaitRetr(void);
void SetGammaPal(unsigned char *pal);
void GetPal(unsigned char *pal);

void DrawSelWindow(int x0,int y0,int x1,int y1,int draw);

#endif

