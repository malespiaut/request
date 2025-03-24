/*
vesa.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

/*
DOS specific video handling.
*/

#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/farptr.h>
#include <sys/movedata.h>
#include <sys/nearptr.h>

#include "defines.h"
#include "types.h"

#include "error.h"
#include "file.h"
#include "memory.h"
#include "qassert.h"
#include "quest.h"
#include "status.h"
#include "vbe.h"
#include "video.h"

static unsigned char* vidMem;

typedef struct
{
  int xsize;
  int ysize;
} res_t;

static res_t res[MAX_RES] =
  {
    {-1, -1},
    {640, 480},
    {800, 600},
    {1024, 768},
    {1280, 1024},
    {1600, 1200}};

static int
FindMode(int vid_mode)
{
  unsigned short* modes;
  VBE_modeInfo mi;

  if (vid_mode < MAX_RES)
  {
    if (res[vid_mode].xsize == -1)
    {
      HandleError("FindMode", "Illegal video mode number.");
      return -1;
    }
    video.ScreenWidth = res[vid_mode].xsize;
    video.ScreenHeight = res[vid_mode].ysize;
  }
  else
  {
    HandleError("FindMode", "Illegal video mode number.");
    return -1;
  }

  if (SV_init() == 0)
  {
    HandleError("FindMode", "No VESA driver preset.");
    return -1;
  }

  for (modes = modeList; *modes != 0xFFFF; modes++)
  {
    VBE_getModeInfo(*modes, &mi);
    if ((mi.XResolution == video.ScreenWidth) &&
        (mi.YResolution == video.ScreenHeight) &&
        (mi.BitsPerPixel == 8))
    {
      if (status.use_vbe2)
      {
        if (mi.modeAttr & 128)
          return *modes;
      }
      else
      {
        return *modes;
      }
    }
  }

  return -1;
}

static unsigned int
MapMem(unsigned int Base, unsigned int size)
{
  union REGS Regs;

  Regs.x.ax = 0x0800;
  Regs.x.bx = Base >> 16;
  Regs.x.cx = Base;
  Regs.x.si = size >> 16;
  Regs.x.di = size;
  int86(0x31, &Regs, &Regs);
  if (Regs.x.flags & 1)
    return 0;

  return (Regs.x.bx << 16) + (Regs.x.cx & 0x0000ffff);
}

int
InitVideo(int vid_mode)
{
  FILE* fp;
  int i;
  int mode;
  VBE_modeInfo mi;
  char palname[256];

  dprint("InitVideo()\n");

  mode = FindMode(vid_mode);

  dprint("Found mode %04x %ix%i\n",
         mode,
         video.ScreenWidth,
         video.ScreenHeight);

  if (mode != -1)
  {
    if (!SV_setMode(mode))
    {
      HandleError("InitVideo", "Unable to set requested video mode.");
      return FALSE;
    }
  }
  else
  {
    HandleError("InitVideo", "Requested video mode is not available.");
    return FALSE;
  }

  VBE_getModeInfo(mode, &mi);

  dprint("modeAttr=%08x bankA=%08x bankB=%08x\n",
         mi.modeAttr,
         mi.bankAAttr,
         mi.bankBAttr);
  dprint("gran=%i segA=%08x segB=%08x\n",
         mi.bankGranularity,
         mi.bankASegment,
         mi.bankBSegment);
  dprint("PhysBase=%08x bpsl=%i\n",
         mi.PhysBase,
         mi.bytesPerScanLine);

  video.ScreenBufferSize = video.ScreenWidth * video.ScreenHeight;

  video.ScreenBuffer = Q_malloc(video.ScreenBufferSize);
  if (video.ScreenBuffer == NULL)
  {
    HandleError("InitVideo", "Unable to allocate screen buffer.");
    return FALSE;
  }
  memset(video.ScreenBuffer, 0, video.ScreenBufferSize);

  FindFile(palname, "quest.pal");
  if ((fp = fopen(palname, "rb")) == NULL)
  {
    HandleError("InitVideo", "Unable to open quest.pal.");
    return FALSE;
  }

  for (i = 0; i < 768; i++)
    video.pal[i] = fgetc(fp) >> 2;

  SetGammaPal(video.pal);

  fclose(fp);

  status.vid_mode = vid_mode;

  if (status.use_vbe2)
  {
    vidMem =
      (unsigned char*)MapMem(mi.PhysBase, video.ScreenWidth * video.ScreenHeight);
    if (!vidMem)
    {
      HandleError("InitVideo", "Unable to map frame buffer");
      return FALSE;
    }
    if (!__djgpp_nearptr_enable())
    {
      HandleError("InitVideo", "Unable to enable nearptr system");
      return FALSE;
    }
  }
  else
  {
    vidMem = (unsigned char*)0x000A0000;
  }

  return TRUE;
}

static void
SetStandardMode(int mode)
{
  union REGS regs;

  regs.x.ax = mode;

  int86(0x10, &regs, &regs);
}

void
SetMode(int vid_mode)
{
  int mode;

  if (vid_mode == RES_TEXT)
  {
    SetStandardMode(0x03);
    return;
  }

  mode = FindMode(vid_mode);

  if (mode != -1)
  {
    if (!SV_setMode(mode))
    {
      HandleError("SetMode", "Unable to set requested video mode!");
      return;
    }
  }
  else
  {
    HandleError("SetMode", "Requested video mode is not available!");
    return;
  }
}

static void
SetBank(int b)
{
  if (b != curBank)
  {
    SV_setBank(b);
    curBank = b;
  }
}

static void
SetScanLine(int Scanline, int index, int width, unsigned char* Bytes, int OneColor)
{
  if (status.use_vbe2)
  {
    memcpy((void*)((int)vidMem +
                   __djgpp_conventional_base +
                   Scanline * video.ScreenWidth +
                   index),
           Bytes,
           width);
    return;
  }
  {
    int l;
    int Sto, h;
    int b;

    l = Scanline * video.ScreenWidth + index;
    b = l >> 16;
    SetBank(b);
    Sto = l & 65535;

    if (Sto < (65536 - video.ScreenWidth))
    {
      if (OneColor)
      {
      }
      else
      {
        dosmemput(&Bytes[0], width, (int)vidMem + Sto);
      }
    }
    else
    {
      h = 1 + (Sto ^ 65535);
      if (h >= width)
        h = width;
      if (OneColor)
      {
      }
      else
      {
        dosmemput(&Bytes[0], h, (int)vidMem + Sto);
      }
      SetBank(b + 1);
      if (OneColor)
      {
      }
      else
      {
        dosmemput(&Bytes[h], width - h, (int)vidMem);
      }
    }
  }
}

void
PutPixel(int x, int y, char color)
{
  if (status.use_vbe2)
  {
    *(unsigned char*)((int)vidMem +
                      __djgpp_conventional_base +
                      x +
                      y * video.ScreenWidth) = color;
    return;
  }
  {
    int l;
    int Sto;
    int b;

    l = y * video.ScreenWidth + x;
    b = l >> 16;
    SetBank(b);
    Sto = l & 65535;

    _farpokeb(_dos_ds, (int)vidMem + Sto, color);
  }
}

void
RefreshPart(int x1, int y1, int x2, int y2)
{
  int i;

  QAssert((x1 >= 0) && (x2 < video.ScreenWidth) &&
          (y1 >= 0) && (y2 < video.ScreenHeight) &&
          (x1 <= x2) && (y1 <= y2));

  for (i = y1; i <= y2; i++)
    SetScanLine(i, x1, x2 - x1 + 1, &video.ScreenBuffer[i * video.ScreenWidth + x1], FALSE);
}

void
RefreshScreen(void)
{
  if (status.use_vbe2)
  {
    memcpy((void*)((int)vidMem +
                   __djgpp_conventional_base),
           video.ScreenBuffer,
           video.ScreenBufferSize);
    return;
  }
  {
    int i;
    int num_banks = video.ScreenBufferSize / 0x10000;
    int last_bank = video.ScreenBufferSize % 0x10000;
    unsigned char* buffer_ptr;

    buffer_ptr = video.ScreenBuffer;

    for (i = 0; i < num_banks; i++)
    {
      SetBank(i);
      dosmemput(buffer_ptr, 0x10000, (int)vidMem);
      buffer_ptr += 0x10000;
    }

    if (last_bank)
    {
      SetBank(i);
      dosmemput(buffer_ptr, last_bank, (int)vidMem);
    }
  }
}

void
DisposeVideo(void)
{
  Q_free(video.ScreenBuffer);
}

void
SetGammaPal(unsigned char* pal)
{
  int i;
  float gammaset;
  float temp;
  unsigned char tmp_pal[768];

  gammaset = gammaval;

  gammaset = gammaset * 2;
  gammaset = gammaset - 1;
  gammaset = gammaset * 64;

  for (i = 0; i < 768; i++)
  {
    temp = pal[i] + gammaset;
    if (temp < 0)
      tmp_pal[i] = 0;
    else if (temp > 63)
      tmp_pal[i] = 63;
    else
      tmp_pal[i] = (char)temp;
  }

  SV_setPalette(0, 256, tmp_pal);
}

void
GetPal(unsigned char* pal)
{
  int i;

  outportb(0x3C7, 0);
  for (i = 768; i; i--)
    *pal++ = inportb(0x3C9) << 2;
}

/*
Special, hardware dependant graphics stuff.
*/

#if 0 // these draw directly to the screen, but aren't used
void PaintBox(int x1, int y1, int x2, int y2, char color)
{
   int i;
   for (i=y1;i<=y2;i++)
      SetScanLine(i,x1,x2-x1+1,&color,TRUE);
}

void PaintPic(int x1, int y1, int x2, int y2, char *Picture)
{
   int i;
   int width;
   width = x2-x1;
   for (i=y1; i<y2; i++)
      SetScanLine(i,x1,width,&(Picture[width*(i-y1)]),FALSE);
}
#endif

void
DrawSelWindow(int x0, int y0, int x1, int y1, int draw)
{
  unsigned char* vid_ptr;
  int i;
  int dx, dy;

  dx = (x1 > x0) ? 1 : -1;
  dy = (y1 > y0) ? 1 : -1;

  if (draw)
  {
    for (i = x0; i != x1; i += dx)
      PutPixel(i, y0, COL_BLUE);
    for (i = x0; i != x1; i += dx)
      PutPixel(i, y1, COL_BLUE);
    for (i = y0; i != y1; i += dy)
      PutPixel(x0, i, COL_BLUE);
    for (i = y0; i != y1; i += dy)
      PutPixel(x1, i, COL_BLUE);
  }
  else
  {
    if (dx == 1)
    {
      vid_ptr = (unsigned char*)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x0);
      for (i = x0; i != x1; i++, vid_ptr++)
        PutPixel(i, y0, *vid_ptr);
      vid_ptr = (unsigned char*)(video.ScreenBuffer + (y1 * video.ScreenWidth) + x0);
      for (i = x0; i != x1; i++, vid_ptr++)
        PutPixel(i, y1, *vid_ptr);
    }
    else
    {
      vid_ptr = (unsigned char*)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x0);
      for (i = x0; i != x1; i--, vid_ptr--)
        PutPixel(i, y0, *vid_ptr);
      vid_ptr = (unsigned char*)(video.ScreenBuffer + (y1 * video.ScreenWidth) + x0);
      for (i = x0; i != x1; i--, vid_ptr--)
        PutPixel(i, y1, *vid_ptr);
    }
    if (dy == 1)
    {
      vid_ptr = (unsigned char*)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x0);
      for (i = y0; i != y1; i++, vid_ptr += video.ScreenWidth)
        PutPixel(x0, i, *vid_ptr);
      vid_ptr = (unsigned char*)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x1);
      for (i = y0; i != y1; i++, vid_ptr += video.ScreenWidth)
        PutPixel(x1, i, *vid_ptr);
    }
    else
    {
      vid_ptr = (unsigned char*)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x0);
      for (i = y0; i != y1; i--, vid_ptr -= video.ScreenWidth)
        PutPixel(x0, i, *vid_ptr);
      vid_ptr = (unsigned char*)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x1);
      for (i = y0; i != y1; i--, vid_ptr -= video.ScreenWidth)
        PutPixel(x1, i, *vid_ptr);
    }
  }
}
