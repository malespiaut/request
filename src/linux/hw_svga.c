//VIDEO.C module of Quest v1.1 Source Code
//Info on the Quest source code can be found in qsrcinfo.txt

//Copyright 1996, Trey Harrison and Chris Carollo
//For non-commercial use only. More rules apply, read legal.txt
//for a full description of usage restrictions.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vga.h>

#include "defines.h"
#include "types.h"

#include "error.h"
#include "file.h"
#include "memory.h"
#include "quest.h"
#include "status.h"
#include "video.h"


#undef FAST_VIDEO


typedef signed short   int16;

u_char *vidMem;
unsigned int curBank;


void SetVESAMode(short vid_mode);
void SetPalette(int16 start, int16 count, unsigned char *p);

void X_SetWindowTitle(char *unused)
{
  /* This function only used for X11 */
}


int InitVideo(int vid_mode)
{
	FILE         *fp;
	int          i, mode;
	int          found_mode = FALSE;
	vga_modeinfo *mi;
	char  questloadname[256];
	int X_texture_move_mode;

	X_texture_move_mode=FALSE;
	switch(vid_mode) {
		case RES_640x480:
				video.ScreenWidth = 640;
				video.ScreenHeight = 480;
				break;
		case RES_800x600:
				video.ScreenWidth = 800;
				video.ScreenHeight = 600;
				break;
		case RES_1024x768:
				video.ScreenWidth = 1024;
				video.ScreenHeight = 768;
				break;
		case RES_1280x1024:
				video.ScreenWidth = 1280;
				video.ScreenHeight = 1024;
				break;
		default:
				HandleError("InitVideo", "Illegal video mode number.");
				return FALSE;
	}

	if(vga_init()!=0) {
	  HandleError("InitVideo", "No (S)VGA driver present.");
	  return FALSE;
	}
	vga_setmousesupport(1);

	for(mode = 1; mode < 49; mode++) {
	  mi = vga_getmodeinfo(mode);
	  if((mi->width == video.ScreenWidth) && (mi->height == video.ScreenHeight) && (mi->bytesperpixel == 1)) {
	    printf("  %d x %d, %d bpp\n", mi->width, mi->height, mi->bytesperpixel);
	    found_mode = TRUE;
	    break;
	  }
	}

	if(found_mode) {
	  fflush(stdout);
	  if(vga_setmode(mode)) {
	    HandleError("InitVideo", "Unable to set requested video mode.");
	    return FALSE;
	  }
	} else {
	  HandleError("InitVideo", "Requested video mode is not available.");
	  return FALSE;
	}
	

	video.ScreenBufferSize = video.ScreenWidth * video.ScreenHeight;

	video.ScreenBuffer = (u_char *)Q_malloc(video.ScreenBufferSize);
	if(video.ScreenBuffer == NULL) {
		HandleError("InitVideo","Unable to allocate screen buffer.");
		return FALSE;
	}
	memset(video.ScreenBuffer, 0, video.ScreenBufferSize);

	/* Load and set default palette. */
	FindFile(questloadname,"quest.pal");
	if((fp = fopen(questloadname, "rb")) == NULL) {
		HandleError("InitVideo", "Unable to open quest.pal.");
		return FALSE;
	}
	for (i=0; i<768; i++) {
	  video.pal[i]=fgetc(fp) >> 2;
	}
	SetGammaPal(video.pal);

	fclose(fp);

	vidMem = (u_char *)vga_getgraphmem();

	return TRUE;
}


void SetMode(int vid_mode)
{
  if (vid_mode==RES_TEXT) {
    vga_setmode(TEXT);
  } else {
    SetVESAMode(vid_mode);
  }
}

void SetVESAMode(short vid_mode)
{
	int          mode;
	int          found_mode = FALSE;
	vga_modeinfo *mi;
//	char  questlibdir[256], questloadname[256];
//	char  *tmpstrptr;

	switch(vid_mode) {
		case RES_640x480:
				video.ScreenWidth = 640;
				video.ScreenHeight = 480;
				break;
		case RES_800x600:
				video.ScreenWidth = 800;
				video.ScreenHeight = 600;
				break;
		case RES_1024x768:
				video.ScreenWidth = 1024;
				video.ScreenHeight = 768;
				break;
		case RES_1280x1024:
				video.ScreenWidth = 1280;
				video.ScreenHeight = 1024;
				break;
		default:
				HandleError("InitVideo", "Illegal video mode number.");
				return;
	}

	if(vga_init()!=0) {
	  HandleError("InitVideo", "No (S)VGA driver present.");
	}
	vga_setmousesupport(1);

	for(mode = 1; mode < 49; mode++) {
	  mi = vga_getmodeinfo(mode);
	  if((mi->width == video.ScreenWidth) && (mi->height == video.ScreenHeight) && (mi->bytesperpixel == 1)) {
	    printf("  %d x %d, %d bpp\n", mi->width, mi->height, mi->bytesperpixel);
	    found_mode = TRUE;
	    break;
	  }
	}

	if(found_mode) {
	  fflush(stdout);
	  if(vga_setmode(mode)) {
	    HandleError("InitVideo", "Unable to set requested video mode.");
	    return;
	  }
	} else {
	  HandleError("InitVideo", "Requested video mode is not available.");
	  return;
	}

	if(found_mode) {
	  fflush(stdout);
	  if(vga_setmode(mode)) {
	    HandleError("InitVideo", "Unable to set requested video mode.");
	  }
	} else {
	  HandleError("InitVideo", "Requested video mode is not available.");
	}
}


static void SetStandardMode(short mode)
{
  switch(mode)
  {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x03:
    vga_setmode(TEXT);
    break;
  case 0x04:
  case 0x05:
    vga_setmode(G320x200x16);
    break;
  case 0x06:
    vga_setmode(G320x200x16);
    break;
  case 0x13:
    vga_setmode(G320x200x256);
    break;
  default:
    printf("Unsupported standard graphics mode %d.\n", mode);
    break;
  }
}


void WaitRetr(void)
{
//  vga_waitretrace();
}


static inline void SetBank(int b)
{
	if (b!=curBank) {
	        vga_setpage(b);
		curBank=b;
	}
}


static void SetScanLine(int Scanline, int index, int width, u_char *Bytes, int OneColor)
{
#if FAST_VIDEO
  int l;
  int Sto,h;
  int b;

  l = Scanline*video.ScreenWidth+index;
  b = l >> 16;
  SetBank(b);
  Sto= l & 65535;

  if (Sto<(65536-video.ScreenWidth)) {
    if (OneColor)
      memset(&vidMem[Sto],Bytes[0],width);
    else
      memcpy(&vidMem[Sto],&Bytes[0],width);
  } else {
    h = 1 + (Sto ^ 65535);
    if (h >= width)
      h = width;
    if (OneColor)
      memset(&vidMem[Sto],Bytes[0],h);
    else
      memcpy(&vidMem[Sto],&Bytes[0],h);
    SetBank(b+1);
    if (OneColor)
      memset(vidMem,Bytes[0],width-h);
    else
      memcpy(vidMem,&Bytes[h],width-h);
  }
#else
  vga_drawscansegment(Bytes, index, Scanline, width);
#endif
}

void PutPixel(int x, int y, char color)
{
#ifdef FAST_VIDEO
  int l;
  int Sto;
  int b;
	
  l = y*video.ScreenWidth+x;
  b = l >> 16;
  SetBank(b);
  Sto = l & 65535;

  memset(&vidMem[Sto],color,1);
#else
  vga_setcolor(color);
  vga_drawpixel(x, y);
#endif
}

void RefreshPart(int x1, int y1, int x2, int y2)
{
  int i;

  for (i=y1;i<=y2;i++) {
    SetScanLine(i,x1,x2-x1+1,&video.ScreenBuffer[i*video.ScreenWidth+x1],FALSE);
  }
}


void RefreshScreen(void)
{
#ifdef FAST_VIDEO
  int i;
  long num_banks = video.ScreenBufferSize / 0x10000;
  long last_bank = video.ScreenBufferSize % 0x10000;
  u_char *buffer_ptr;

  buffer_ptr = video.ScreenBuffer;

  for(i=0; i<num_banks; i++) {
    SetBank(i);
    memcpy(vidMem, buffer_ptr, 0x10000);
    buffer_ptr += 0x10000;
  }

  if(last_bank) {
    SetBank(i);
    memcpy(vidMem, buffer_ptr, last_bank);
  }
#else
  RefreshPart(0, 0, video.ScreenWidth-1, video.ScreenHeight-1);
#endif
}

void DrawSelWindow(int x0,int y0,int x1,int y1,int draw)
{
	u_char *vid_ptr;
	short   i;
	short   dx, dy;

	dx = (x1>x0)?1:-1;
	dy = (y1>y0)?1:-1;

	if(draw) {
		for(i=x0; i!=x1; i+=dx)
			PutPixel(i, y0, COL_BLUE);
		for(i=x0; i!=x1; i+=dx)
			PutPixel(i, y1, COL_BLUE);
		for(i=y0; i!=y1; i+=dy)
			PutPixel(x0, i, COL_BLUE);
		for(i=y0; i!=y1; i+=dy)
			PutPixel(x1, i, COL_BLUE);
	} else {
		if(dx == 1) {
			vid_ptr = (u_char *)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x0);
			for(i=x0; i!=x1; i++, vid_ptr++)
				PutPixel(i, y0, *vid_ptr);
			vid_ptr = (u_char *)(video.ScreenBuffer + (y1 * video.ScreenWidth) + x0);
			for(i=x0; i!=x1; i++, vid_ptr++)
				PutPixel(i, y1, *vid_ptr);
		} else {
			vid_ptr = (u_char *)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x0);
			for(i=x0; i!=x1; i--, vid_ptr--)
				PutPixel(i, y0, *vid_ptr);
			vid_ptr = (u_char *)(video.ScreenBuffer + (y1 * video.ScreenWidth) + x0);
			for(i=x0; i!=x1; i--, vid_ptr--)
				PutPixel(i, y1, *vid_ptr);
		}
		if(dy == 1) {
			vid_ptr = (u_char *)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x0);
			for(i=y0; i!=y1; i++, vid_ptr += video.ScreenWidth)
				PutPixel(x0, i, *vid_ptr);
			vid_ptr = (u_char *)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x1);
			for(i=y0; i!=y1; i++, vid_ptr += video.ScreenWidth)
				PutPixel(x1, i, *vid_ptr);
		} else {
			vid_ptr = (u_char *)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x0);
			for(i=y0; i!=y1; i--, vid_ptr -= video.ScreenWidth)
				PutPixel(x0, i, *vid_ptr);
			vid_ptr = (u_char *)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x1);
			for(i=y0; i!=y1; i--, vid_ptr -= video.ScreenWidth)
				PutPixel(x1, i, *vid_ptr);
		}
	}
}

void DisposeVideo(void)
{
	Q_free(video.ScreenBuffer);
}


void SetGammaPal(unsigned char *pal)
{
  int i;
  float gammaset;
  float temp;
  unsigned char tmp_pal[768];

  gammaset = gammaval;

  gammaset = gammaset * 2;
  gammaset = gammaset - 1;
  gammaset = gammaset * 64;

  for (i=0; i<768; i++)
    {
      temp = pal[i] + gammaset;
      if (temp<0)
        tmp_pal[i] = 0;
      else
        if (temp>63)
          tmp_pal[i] = 63;
        else
          tmp_pal[i] = (char)temp;
    }

  SetPalette(0, 256, tmp_pal);
}

void SetPalette(int16 start, int16 count, unsigned char *p)
{
    int16 i;

    if (start < 0 || (start + count - 1) > 255) return;    

    WaitRetr();

    for (i = 0; i < count; i++) {
//      vga_setpalette(i, p[0], p[1], p[2]);
      p += 3;
    }
}


void GetPal(unsigned char *pal)
{
  int i, r, g, b;

  for (i=0;i<256;i++) {
    vga_getpalette(i, &r, &g, &b);
    pal[i*3+0] = r;
    pal[i*3+1] = g;
    pal[i*3+2] = b;
  }
}

