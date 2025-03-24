#ifndef _VIDEO_X11_H
#define _VIDEO_X11_H

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

typedef float float32;
typedef double float64;

/* hw_x11.c */
extern Display* xdisplay;
extern Window xwindow;

/* key_x11.c */
#include "keyboard.h"

extern int keys[NUM_UNIQUE_KEYS];
void X_HandleEvents(void);

#endif /* _VIDEO_X11_H */
