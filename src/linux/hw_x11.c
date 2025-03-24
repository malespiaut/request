/*
hw_x11.c file of the Quest Source Code

Copyright 1997, 1998, 1999 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/xpm.h>

#include "defines.h"
#include "types.h"

#include "hw_x11.h"
#include "video.h"

#include "error.h"
#include "file.h"
#include "memory.h"
#include "quest.h"
#include "status.h"
#include "version.h"

#include "quest_icon.xpm"

extern int argc;
extern char** argv;

static const char* windowTitle = "Quest " QUEST_VER " - ";

static u_char* vidMem;

/* stuff that has to do with X only */
Display* xdisplay;
static int xscreen;
static XVisualInfo xvisinfo;
Window xwindow;

static GC xgc;
static int xwidth, xheight;
static int xvisual;
static Bool xshm_supported;

static XImage* ximage;
static Colormap xcolmap;
static uint8* xdispbuffer;

static XShmSegmentInfo xshminfo;
static int xshmid;
static int xshm_complete;
static uint8* xshmaddr;

#define XVIS_8BIT_PSEUDO 0
#define XVIS_15BIT_TRUE 1
#define XVIS_16BIT_TRUE 2
#define XVIS_24BIT_TRUE 3
#define XVIS_32BIT_TRUE 4
static int xtabledepth[] = {8, 15, 16, 24, 32, 0};
static int xtableclass[] = {PseudoColor, TrueColor, TrueColor, TrueColor, TrueColor};

static XColor xtable8[256];
static uint16 xtable8to1516[256];
static uint32 xtable8to2432[256];

static struct
{
  uint8 red;
  uint8 green;
  uint8 blue;
} xpalette[256];

/*
** This doesn't exist in extensions/XShm.h, but it's in the docs and libXext.
** Weird.
*/
extern int XShmGetEventBase(Display*);

void X_SetWindowTitle(const char* fileName);
void
X_SetWindowTitle(const char* fileName)
{
  XTextProperty textProp;
  char* title;

  if (xdisplay == NULL)
    return;

  if (fileName[0] == 0 || fileName == NULL)
  {
    title = Q_malloc(strlen(windowTitle) + sizeof("New map") + 1);
    strcpy(title, windowTitle);
    strcat(title, "New map");
  }
  else
  {
    title = Q_malloc(strlen(windowTitle) + strlen(fileName) + 1);
    strcpy(title, windowTitle);
    strcat(title, fileName);
  }

  XStringListToTextProperty(&title, 1, &textProp);
  XSetWMName(xdisplay, xwindow, &textProp);
  XSetWMIconName(xdisplay, xwindow, &textProp);
  XFlush(xdisplay);

  Q_free(title);
  XFree(textProp.value);
}

int
InitVideo(int vid_mode)
{
  static int resolutions[][2] =
    {{0, 0},
     {640, 480},
     {800, 600},
     {1024, 768},
     {1280, 1024},
     {1600, 1280}};

  int i;
  int major, minor, size;
  int xvisdepth, xpixdepth; /* xvisdepth in bits, xpixdepth in bytes */

  if (vid_mode <= 0 || vid_mode >= MAX_RES)
  {
    HandleError("InitVideo", "Illegal video mode number.");
    return FALSE;
  }
  xwidth = video.ScreenWidth = resolutions[vid_mode][0];
  xheight = video.ScreenHeight = resolutions[vid_mode][1];

  printf("Video mode %i, %ix%i\n", vid_mode, xwidth, xheight);

  /* Allocate hidden frame buffer */
  video.ScreenBufferSize = video.ScreenWidth * video.ScreenHeight;
  video.ScreenBuffer = (u_char*)Q_malloc(video.ScreenBufferSize);
  if (video.ScreenBuffer == NULL)
  {
    HandleError("InitVideo", "Unable to allocate screen buffer.");
    return FALSE;
  }
  memset(video.ScreenBuffer, 0, video.ScreenBufferSize);

  /* Connect to Xserver. */
  if ((xdisplay = XOpenDisplay(NULL)) == NULL)
  {
    HandleError("InitVideo", "Could not connect to XServer.");
    return FALSE;
  }
  xscreen = XDefaultScreen(xdisplay);

  /* Try to find out which visual we can use. */
  for (xvisual = 0; xtabledepth[xvisual]; xvisual++)
  {
    if (XMatchVisualInfo(xdisplay, xscreen, xtabledepth[xvisual], xtableclass[xvisual], &xvisinfo))
      break;
  }
  if (!xtabledepth[xvisual])
  {
    HandleError("InitVideo", "Couldn't find suitable visual!");
    return FALSE;
  }

  xvisdepth = xtabledepth[xvisual];
  {
    XPixmapFormatValues* p;
    int num;

    if (!(p = XListPixmapFormats(xdisplay, &num)))
    {
      HandleError("InitVideo", "Couldn't get pixmap format list!");
      return FALSE;
    }

    for (i = 0; i < num; i++)
      if (p[i].depth == xvisdepth)
        break;
    if (i == num)
    {
      HandleError("InitVideo", "Couldn't find pixmap format!");
      return FALSE;
    }

    xpixdepth = p[i].bits_per_pixel;
    if (xpixdepth & 7)
    {
      HandleError("InitVideo", "Confused: xpixdepth=%i", xpixdepth);
      return FALSE;
    }
    xpixdepth /= 8;
  }

  printf("Using %i bit visual, %i bits/pixel\n", xvisdepth, xpixdepth * 8);

  switch (xvisual)
  {
      /* 8, 15, and 16 bit should be fine by now, but 24 and 32 bit might
         need some tweaking since the number of bits/pixel might not match
         the depth. */
    case XVIS_8BIT_PSEUDO:
    case XVIS_15BIT_TRUE:
    case XVIS_16BIT_TRUE:
      if (xpixdepth != (xvisdepth + 7) / 8)
      {
        HandleError("InitVideo",
                    "Confused: xvisual=%i xvisdepth=%i xpixdepth=%i",
                    xvisual,
                    xvisdepth,
                    xpixdepth);
        return FALSE;
      }
      break;

    case XVIS_24BIT_TRUE:
    case XVIS_32BIT_TRUE:
      if (xpixdepth == 3)
        xvisual = XVIS_24BIT_TRUE;
      else if (xpixdepth == 4)
        xvisual = XVIS_32BIT_TRUE;
      else
      {
        HandleError("InitVideo", "Confused: xvisual=%i xpixdepth=%i", xvisual, xpixdepth);
        return FALSE;
      }
      break;
  }

  /* If Xserver supports shared memory, we should also use it. */
  XShmQueryVersion(xdisplay, &major, &minor, &xshm_supported);
  xshm_supported = 0;
  if (xshm_supported && xvisual != XVIS_8BIT_PSEUDO)
  {
    printf("Using shared memory (MIT-shm version %i.%i).\n", major, minor);

    ximage = XShmCreateImage(xdisplay, xvisinfo.visual, xvisinfo.depth, ZPixmap, 0, &xshminfo, xwidth, xheight);

    if ((xshmid = shmget(IPC_PRIVATE, video.ScreenBufferSize * xpixdepth, IPC_CREAT | 0700)) == -1)
    {
      HandleError("InitVideo",
                  "Could not get shared memory id for X display buffer.");
      XCloseDisplay(xdisplay);
      return FALSE;
    }
    if ((xshmaddr = ximage->data = shmat(xshmid, NULL, 0700)) == NULL)
    {
      HandleError("InitVideo",
                  "Could not get shared memory id for X display buffer.");
      XCloseDisplay(xdisplay);
      return FALSE;
    }
    printf("shmid: %d, shmaddr: %p\n", xshmid, xshmaddr);

    xshminfo.shmid = xshmid;
    xshminfo.shmaddr = xshmaddr;
    xshminfo.readOnly = False;
    XShmAttach(xdisplay, &xshminfo);

    xshm_complete = 1;

    vidMem = (u_char*)xshmaddr;
  }
  else
  {
    /* !xshm_supported. I will use an XImage here, because an XPixmap
       does not make much sense due to the complete refresh from the
       framebuffer. */

    size = video.ScreenWidth * video.ScreenHeight * xpixdepth;
    if (xvisual == XVIS_8BIT_PSEUDO)
    {
      xdispbuffer = video.ScreenBuffer;
      xcolmap = XCreateColormap(
        xdisplay, RootWindow(xdisplay, xscreen), xvisinfo.visual, AllocAll);
      if (!xcolmap)
      {
        HandleError("InitVideo", "Couldn't create colormap!");
        return FALSE;
      }
      memset(xtable8, 0, 256 * sizeof(XColor));
    }
    else
    {
      xdispbuffer = (u_char*)Q_malloc(size);
      if (xdispbuffer == NULL)
      {
        HandleError("InitVideo", "Unable to allocate display buffer.");
        XCloseDisplay(xdisplay);
        return FALSE;
      }
      memset(xdispbuffer, 0, size);
    }

    ximage = XCreateImage(xdisplay, xvisinfo.visual, xvisinfo.depth, ZPixmap, 0, xdispbuffer, xwidth, xheight, 32, 0);

    vidMem = (u_char*)xdispbuffer;
  }

  {
    int attribmask;
    XSetWindowAttributes attribs;

    attribmask = CWEventMask | CWBorderPixel;
    if (xvisual == XVIS_8BIT_PSEUDO)
    {
      attribmask |= CWColormap;
      attribs.colormap = xcolmap;
    }
    attribs.event_mask =
      ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
      PointerMotionMask | KeyPressMask | KeyReleaseMask | ExposureMask;
    attribs.border_pixel = 0;
    xwindow = XCreateWindow(xdisplay, DefaultRootWindow(xdisplay), 0, 0, xwidth, xheight, 0, xvisdepth, InputOutput, xvisinfo.visual, attribmask, &attribs);
    if (xwindow == 0)
    {
      HandleError("InitVideo", "Could not get X11 window.");
      return FALSE;
    }
  }

  {
    int valuemask;
    XGCValues xgcvalues;

    valuemask = GCGraphicsExposures;
    xgcvalues.graphics_exposures = False;
    xgc = XCreateGC(xdisplay, xwindow, valuemask, &xgcvalues);
  }

  XMapWindow(xdisplay, xwindow);

  XSetCommand(xdisplay, xwindow, argv, argc);

  {
    XSizeHints* sh;

    sh = XAllocSizeHints();
    sh->min_width = sh->max_width = xwidth;
    sh->min_height = sh->max_height = xheight;
    sh->height_inc = 0;
    sh->width_inc = 0;
    sh->flags = PMaxSize | PMinSize | PResizeInc;
    XSetWMNormalHints(xdisplay, xwindow, sh);

    XFree(sh);
  }

  {
    XClassHint* classHint;

    classHint = XAllocClassHint();
    classHint->res_name = "quest";
    classHint->res_class = "main_window";
    XSetClassHint(xdisplay, xwindow, classHint);

    XFree(classHint);
  }

  {
    int xpmerror;
    XWMHints* WMHints;

    WMHints = XAllocWMHints();
    if ((xpmerror = XpmCreatePixmapFromData(
           xdisplay,
           DefaultRootWindow(xdisplay),
           (char**)quest_icon_xpm,
           &WMHints->icon_pixmap,
           &WMHints->icon_mask,
           NULL)) == 0)
    {
      WMHints->flags = IconPixmapHint | IconMaskHint;
      XSetWMHints(xdisplay, xwindow, WMHints);
    }
    else
    {
      printf("warning: Couldn't set program icon: %s\n",
             XpmGetErrorString(xpmerror));
    }
    XFree(WMHints);
  }

  XFlush(xdisplay);

  {
    FILE* fp;
    char questloadname[256];

    FindFile(questloadname, "quest.pal");
    if ((fp = fopen(questloadname, "rb")) == NULL)
    {
      HandleError("InitVideo", "Unable to open quest.pal.");
      return FALSE;
    }
    for (i = 0; i < 768; i++)
    {
      video.pal[i] = fgetc(fp) >> 2;
    }
    SetGammaPal(video.pal);
    fclose(fp);
  }

  return TRUE;
}

void
SetMode(int vid_mode __attribute__((unused)))
{
  /* Nothing to do here, because we never close the window. */
}

void
DisposeVideo()
{
  if (xshm_supported)
  {
    XShmDetach(xdisplay, &xshminfo);
    XDestroyImage(ximage);
    shmdt(xshmaddr);
    shmctl(xshmid, IPC_RMID, 0);
  }
  else
  {
    if (xdispbuffer != video.ScreenBuffer)
      Q_free(xdispbuffer);
  }

  Q_free(video.ScreenBuffer);

  XUnmapWindow(xdisplay, xwindow);
  XCloseDisplay(xdisplay);
}

static void
ConvertRectangle(int x1, int y1, int x2, int y2)
{
  switch (xvisual)
  {
    case XVIS_8BIT_PSEUDO:
      break;

    case XVIS_15BIT_TRUE:
    case XVIS_16BIT_TRUE:
      {
        int x, y, rem;
        uint8* src_ptr;
        uint16* dest_ptr;

        rem = video.ScreenWidth - x2 + x1;
        src_ptr = video.ScreenBuffer + y1 * xwidth;
        dest_ptr = (uint16*)((uint8*)vidMem + y1 * ximage->bytes_per_line);

        src_ptr += x1;
        dest_ptr += x1;
        for (y = y1; y < y2; y++)
        {
          for (x = x1; x < x2; x++)
          {
            *dest_ptr++ = xtable8to1516[*src_ptr++];
          }
          dest_ptr += rem;
          src_ptr += rem;
        }
      }
      break;

    case XVIS_32BIT_TRUE:
      {
        int x, y, rem;
        uint8* src_ptr;
        uint32* dest_ptr;

        rem = video.ScreenWidth - x2 + x1;
        src_ptr = video.ScreenBuffer + y1 * xwidth;
        dest_ptr = (uint32*)((uint8*)vidMem + y1 * ximage->bytes_per_line);

        src_ptr += x1;
        dest_ptr += x1;
        for (y = y1; y < y2; y++)
        {
          for (x = x1; x < x2; x++)
          {
            *dest_ptr++ = xtable8to2432[*src_ptr++];
          }
          dest_ptr += rem;
          src_ptr += rem;
        }
      }
      break;

    case XVIS_24BIT_TRUE:
      {
        int x, y;
        uint8* src_ptr;
        uint8* dest_ptr;

        for (y = y1; y < y2; y++)
        {
          src_ptr = video.ScreenBuffer + y * xwidth;
          dest_ptr = (uint8*)((uint8*)vidMem + y * ximage->bytes_per_line);
          for (x = x1; x < x2; x++)
          {
            *(uint16*)(&dest_ptr[x * 3]) = xtable8to2432[src_ptr[x]];
            dest_ptr[x * 3 + 2] = *(2 + (uint8*)&xtable8to2432[src_ptr[x]]);
          }
        }
      }
      break;

    default:
      Abort("ConvertRectangle", "Unsupported visual type.");
      break;
  }
}

void
WaitRetr(void)
{
  XEvent dummy;

  if (!xshm_supported)
    return;

  if (!xshm_complete)
    while (!XCheckTypedEvent(xdisplay, XShmGetEventBase(xdisplay), &dummy))
      ;

  xshm_complete = 1;
}

static void
PutPixel(int x, int y, unsigned char color)
{
  switch (xvisual)
  {
    case XVIS_8BIT_PSEUDO: /* warning: this relies on DrawSelWindow calls
                              being matched */
      vidMem[x + ximage->bytes_per_line * y] ^= COL_BLUE;
      break;

    case XVIS_15BIT_TRUE:
    case XVIS_16BIT_TRUE:
      *(uint16*)&vidMem[2 * x + ximage->bytes_per_line * y] = xtable8to1516[color];
      break;

    case XVIS_24BIT_TRUE:
      *(uint16*)&vidMem[3 * x + ximage->bytes_per_line * y] = xtable8to2432[color];
      *(uint8*)&vidMem[3 * x + ximage->bytes_per_line * y + 2] = *(2 + (uint8*)&xtable8to2432[color]);
      break;

    case XVIS_32BIT_TRUE:
      *(uint32*)&vidMem[4 * x + ximage->bytes_per_line * y] = xtable8to2432[color];
      break;
  }
}

void
RefreshPart(int x1, int y1, int x2, int y2)
{
  ConvertRectangle(x1, y1, x2 + 1, y2 + 1);

  WaitRetr();

  if (xshm_supported)
  {
    XShmPutImage(xdisplay, xwindow, xgc, ximage, x1, y1, x1, y1, x2 - x1 + 1, y2 - y1 + 1, True);
    xshm_complete = 0;
  }
  else
  {
    XPutImage(xdisplay, xwindow, xgc, ximage, x1, y1, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  }

  XFlush(xdisplay);
}

void
RefreshScreen(void)
{
  ConvertRectangle(0, 0, xwidth, xheight);

  WaitRetr();

  if (xshm_supported)
  {
    XShmPutImage(xdisplay, xwindow, xgc, ximage, 0, 0, 0, 0, xwidth, xheight, True);
    xshm_complete = 0;
  }
  else
  {
    XPutImage(xdisplay, xwindow, xgc, ximage, 0, 0, 0, 0, xwidth, xheight);
  }
  XFlush(xdisplay);
}

void
DrawSelWindow(int x0, int y0, int x1, int y1, int draw)
{
  u_char* vid_ptr;
  int i;
  int dx, dy;

  if (x1 < x0)
  {
    i = x1;
    x1 = x0;
    x0 = i;
  }
  if (y1 < y0)
  {
    i = y1;
    y1 = y0;
    y0 = i;
  }
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
      vid_ptr = (u_char*)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x0);
      for (i = x0; i != x1; i++, vid_ptr++)
        PutPixel(i, y0, *vid_ptr);
      vid_ptr = (u_char*)(video.ScreenBuffer + (y1 * video.ScreenWidth) + x0);
      for (i = x0; i != x1; i++, vid_ptr++)
        PutPixel(i, y1, *vid_ptr);
    }
    else
    {
      vid_ptr = (u_char*)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x0);
      for (i = x0; i != x1; i--, vid_ptr--)
        PutPixel(i, y0, *vid_ptr);
      vid_ptr = (u_char*)(video.ScreenBuffer + (y1 * video.ScreenWidth) + x0);
      for (i = x0; i != x1; i--, vid_ptr--)
        PutPixel(i, y1, *vid_ptr);
    }
    if (dy == 1)
    {
      vid_ptr = (u_char*)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x0);
      for (i = y0; i != y1; i++, vid_ptr += video.ScreenWidth)
        PutPixel(x0, i, *vid_ptr);
      vid_ptr = (u_char*)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x1);
      for (i = y0; i != y1; i++, vid_ptr += video.ScreenWidth)
        PutPixel(x1, i, *vid_ptr);
    }
    else
    {
      vid_ptr = (u_char*)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x0);
      for (i = y0; i != y1; i--, vid_ptr -= video.ScreenWidth)
        PutPixel(x0, i, *vid_ptr);
      vid_ptr = (u_char*)(video.ScreenBuffer + (y0 * video.ScreenWidth) + x1);
      for (i = y0; i != y1; i--, vid_ptr -= video.ScreenWidth)
        PutPixel(x1, i, *vid_ptr);
    }
  }

  WaitRetr();

  if (xshm_supported)
  {
    XShmPutImage(xdisplay, xwindow, xgc, ximage, x0, y0, x0, y0, x1 - x0 + 1, y1 - y0 + 1, True);
    xshm_complete = 0;
  }
  else
  {
    XPutImage(xdisplay, xwindow, xgc, ximage, x0, y0, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
  }

  XFlush(xdisplay);
}

static void
SetPalette(int16 start, int16 count, unsigned char* p)
{
  int16 i;
  uint32 red, green, blue;

  if (start < 0 || (start + count) > 256)
    return;

  WaitRetr();
  for (i = 0; i < count; i++)
  {
    /* Note that red, green, and blue are from 0-63. The shifting further
       down takes care of converting it to 0-255. */
    red = xpalette[start + i].red = p[3 * i];
    green = xpalette[start + i].green = p[3 * i + 1];
    blue = xpalette[start + i].blue = p[3 * i + 2];

    switch (xvisual)
    {
      case XVIS_8BIT_PSEUDO:
        /* we don't need to fill a conversion table here, but to update
          the 8bit-pseudocolor palette (not done yet) */
        xtable8[start + i].red = red << 10;
        xtable8[start + i].green = green << 10;
        xtable8[start + i].blue = blue << 10;
        xtable8[start + i].flags = DoRed | DoGreen | DoBlue;
        xtable8[start + i].pixel = start + i;
        break;

      /* TODO : use actual masks instead of guessing */
      case XVIS_15BIT_TRUE:
        /* 15 bit visuals are equally weighted */
        xtable8to1516[start + i] =
          ((red << 9) & 0x7c00) |
          ((green << 4) & 0x03e0) |
          ((blue >> 1) & 0x001f);
        break;

      case XVIS_16BIT_TRUE:
        /* 16 bit visuals are usually weighted 5-6-5 */
        xtable8to1516[start + i] =
          ((red << 10) & 0xf800) |
          ((green << 5) & 0x07e0) |
          ((blue >> 1) & 0x001f);
        break;

      case XVIS_24BIT_TRUE:
      case XVIS_32BIT_TRUE:
        /* these two should be clear enough */
        xtable8to2432[start + i] =
          (red << 18) |
          (green << 10) |
          (blue << 2);
        break;

      default:
        Abort("SetPalette", "Unsupported visual type.");
        break;
    }
  }

  if (xvisual == XVIS_8BIT_PSEUDO)
  {
    XStoreColors(xdisplay, xcolmap, xtable8, 256);
  }
  else
  {
    RefreshScreen();
  }
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

  SetPalette(0, 256, tmp_pal);
}

void
GetPal(unsigned char* pal)
{
  int i;

  for (i = 0; i < 256; i++)
  {
    pal[i * 3 + 0] = xpalette[i].red;
    pal[i * 3 + 1] = xpalette[i].green;
    pal[i * 3 + 2] = xpalette[i].blue;
  }
}
