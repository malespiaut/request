/*
vbe.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

/*
VBE 1.2 module

HEAVILY based on code written by: Robert C. Pendleton
(in fact, I've only made minor revisions in adopting it)

His code is Copyright 1993 by Robert C. Pendleton
more of his work can be found at http://www.pendleton.com/rants.html
*/

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <string.h>
#include <sys/movedata.h>
#include <sys/farptr.h>

#include "defines.h"
#include "types.h"

#include "vbe.h"

#include "status.h"
#include "video.h"

unsigned short *modeList;
unsigned int curBank;
static unsigned int bankShift;

typedef struct
{
    int edi;
    int esi;
    int ebp;
    int reserved_by_system;
    int ebx;
    int edx;
    int ecx;
    int eax;
    short flags;
    short es,ds,fs,gs,ip,cs,sp,ss;
} rminfo;

static unsigned int allocDosMem(int size, unsigned int *segment, unsigned int *selector)
{
    union REGS reg;

    *segment=0;
    *selector=0;
    reg.x.ax = 0x0100;
    reg.x.bx = (size + 15) >> 4;

    int86(0x31, &reg, &reg);

    if (reg.x.cflag)
    {
        return 0;
    }

    *segment = reg.x.ax;
    *selector = reg.x.dx;

    return (unsigned int) ((reg.x.ax & 0xffff) << 4);
}

static void freeDosMem(unsigned int selector)
{
    union REGS reg;

    reg.x.ax = 0x0101;
    reg.x.dx = selector;

    int86(0x31, &reg, &reg);
}

static unsigned int rmp2pmp(unsigned int addr)
{
    return (((addr & 0xffff0000) >> 12) + (addr & 0xffff));
}

static void int386rm(unsigned int inter, rminfo *inout)
{
    union REGS reg;
    struct SREGS sreg;

    memset(&reg, 0, sizeof(reg));
    memset(&sreg, 0, sizeof(sreg));

    reg.x.ax = 0x0300;
    reg.h.bl = inter;
    reg.h.bh = 0;
    reg.x.cx = 0;
    sreg.es = _go32_my_ds();
    reg.x.di = (int)inout;

    int86x(0x31,&reg,&reg,&sreg);
}

int SV_init(void)
{
   vbeInfo VBEINFO;

   if (vbeGetInfo(&VBEINFO))
   {
/*    printf("Debug Information:\n");
      printf("(please include this information if the SVGA is not working correctly)\n");
      printf("Card Manufacturer: %s\n",(char *)((int)VBEINFO.vendorName));
      printf("Card Manufacturer addr: %i\n",(char *)((int)VBEINFO.vendorName));*/
      modeList = VBEINFO.modes;
      return TRUE;
   }
   return FALSE;
}

int vbeGetInfo(vbeInfo *infoPtr)
{
    rminfo rmi;

    unsigned int dosmem;
    unsigned int seg;
    unsigned int sel;

    int len;
    unsigned short *modes;

    dosmem = allocDosMem(sizeof(vbeInfo), &seg, &sel);

    memset(&rmi, 0, sizeof(rmi));
    rmi.eax = 0x4f00;
    rmi.es = (dosmem >> 4) & 0xffff;
    rmi.edi = 0;

    int386rm(0x10, &rmi);

    dosmemget(dosmem,sizeof(vbeInfo),infoPtr);
    freeDosMem(sel);

    if ((rmi.eax&0xffff) == 0x004f)
    {
        infoPtr->vendorName = (char *)rmp2pmp((unsigned int)infoPtr->vendorName);

        modes = (unsigned short *) rmp2pmp((unsigned int)infoPtr->modes);
        len = 0;

        _farsetsel(_dos_ds);
        while (_farnspeekw((int)modes) != 0xffff)
        {
            len++;
            modes++;
        }
        modes = (unsigned short *) rmp2pmp((unsigned int)infoPtr->modes);
        infoPtr->modes = (unsigned short *)malloc(sizeof(unsigned short) * (len + 1));
        dosmemget((int)modes,sizeof(unsigned short)*(len+1),infoPtr->modes);

        return TRUE;
    }

    return FALSE;
}

int VBE_getModeInfo(int mode, VBE_modeInfo *infoPtr)
{
    union REGS reg;
    rminfo rmi;

    unsigned int dosmem;
    unsigned int seg;
    unsigned int sel;

    dosmem = allocDosMem(sizeof(VBE_modeInfo), &seg, &sel);

    memset(&reg, 0, sizeof(reg));

    memset(&rmi, 0, sizeof(rmi));
    rmi.eax = 0x4f01;
    rmi.ecx = mode;
    rmi.es = (dosmem >> 4) & 0xffff;
    rmi.edi = 0;

    int386rm(0x10, &rmi);

    dosmemget(dosmem,sizeof(VBE_modeInfo),infoPtr);
    freeDosMem(sel);

    return ((rmi.eax&0xffff) == 0x004f);
}

int SV_setMode(int mode)
{
   VBE_modeInfo NewModeInfo;
   union REGS r;
   int i;

   VBE_getModeInfo(mode&0x3fff,&NewModeInfo);

   if (!status.use_vbe2)
   {
      i = (64/NewModeInfo.bankGranularity);

      bankShift=0;
      for (i>>=1;i;i>>=1,bankShift++) ;
   }

   memset(&r, 0, sizeof(r));
   r.x.ax = 0x4F02;
   if (status.use_vbe2)
      r.x.bx = mode|0x4000;
   else
      r.x.bx = mode&0x3fff;

   int86(0x10, &r, &r);

   return ((r.x.ax&0xffff) == 0x004f);
}

int vbeGetMode(int *mode)
{
   rminfo rmi;

   memset(&rmi, 0, sizeof(rmi));
   rmi.eax = 0x4f03;

   int386rm(0x10, &rmi);

   *mode = (rmi.ebx&0xffff);

   return ((rmi.eax&0xffff) == 0x004f);
}


void WaitRetr(void)
{
   while ((inportb(0x03DA)&0x08) == 0);
   while ((inportb(0x03DA)&0x08) != 0);
}

/*
static void vgaSetColor(int index, int r, int g, int b)
{
   if (index < 0 || index > 255) return;
   
   WaitRetr();

   outportb(0x3C8,index);
   outportb(0X3C9,r);
   outportb(0X3C9,g);
   outportb(0X3C9,b);
}
*/

void SV_setPalette(int start, int count, unsigned char *p)
{
   int i;

   if (start < 0 || (start + count - 1) > 255) return;

   WaitRetr();

   outportb(0x3C8,start);
   for (i=count*3;i;i--)
   {
      outportb(0x3C9, *p++);
   }
}

void SV_setBank(unsigned int bank)
{        
   union REGS reg;

   reg.x.ax = 0x4F05;
   reg.x.bx = 0;
   reg.x.dx = bank<<bankShift;
   int86(0x10, &reg, &reg);
}

