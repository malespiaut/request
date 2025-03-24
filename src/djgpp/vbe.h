/*
vbe.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

/*
 VBE 1.2 header file

 HEAVILY based on code written by: Robert C. Pendleton
 (in fact, I've only made minor revisions in adopting it)

 His code is Copyright 1993 by Robert C. Pendleton
 more of his work can be found at http://www.pendleton.com/rants.html
*/
#ifndef VBE_H
#define VBE_H

typedef struct
{
  char vesa[4] __attribute__((packed));
  unsigned char minorMode __attribute__((packed));
  unsigned char majorMode __attribute__((packed));
  char* vendorName __attribute__((packed));
  unsigned int capabilities __attribute__((packed));
  unsigned short* modes __attribute__((packed));
  unsigned short memory __attribute__((packed));
  char reserved_236[236] __attribute__((packed));
} vbeInfo;

typedef struct
{
  unsigned short modeAttr __attribute__((packed));
  unsigned char bankAAttr __attribute__((packed));
  unsigned char bankBAttr __attribute__((packed));
  unsigned short bankGranularity __attribute__((packed));
  unsigned short bankSize __attribute__((packed));
  unsigned short bankASegment __attribute__((packed));
  unsigned short bankBSegment __attribute__((packed));
  unsigned int posFuncPtr __attribute__((packed));
  unsigned short bytesPerScanLine __attribute__((packed));
  unsigned short XResolution __attribute__((packed));
  unsigned short YResolution __attribute__((packed));
  unsigned char charWidth __attribute__((packed));
  unsigned char charHeight __attribute__((packed));
  unsigned char numberOfPlanes __attribute__((packed));
  unsigned char BitsPerPixel __attribute__((packed));
  unsigned char numberOfBanks __attribute__((packed));
  unsigned char memoryModel __attribute__((packed));
  unsigned char videoBankSize __attribute__((packed));
  unsigned char imagePages __attribute__((packed));

  unsigned char reserved_1 __attribute__((packed));

  unsigned char redMaskSize __attribute__((packed));
  unsigned char redFieldPos __attribute__((packed));
  unsigned char greenMaskSize __attribute__((packed));
  unsigned char greenFieldPos __attribute__((packed));
  unsigned char blueMaskSize __attribute__((packed));
  unsigned char blueFieldPos __attribute__((packed));
  unsigned char rsvdMaskSize __attribute__((packed));
  unsigned char rsvdFieldPos __attribute__((packed));
  unsigned char DirectColorInfo __attribute__((packed));

  unsigned int PhysBase __attribute__((packed));

  unsigned int OffScreenMemOffset __attribute__((packed));
  unsigned short OffScreenMemSize __attribute__((packed));

  unsigned char reserved_216[206] __attribute__((packed));
} VBE_modeInfo;

int vbeGetInfo(vbeInfo* infoPtr);
int VBE_getModeInfo(int mode, VBE_modeInfo* infoPtr);
int SV_setMode(int mode);
int vbeGetMode(int* mode);
int SV_init(void);
void SV_setBank(unsigned int bank);
void SV_setPalette(int start, int count, unsigned char* p);

extern unsigned short* modeList;
extern unsigned int curBank;

#endif
