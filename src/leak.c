/*
leak.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

For non-commercial use only. More rules apply, read legal.txt
for a full description of usage restrictions.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "defines.h"
#include "types.h"

#include "leak.h"

#include "error.h"
#include "memory.h"
#include "message.h"

// for memory tracking code
#include "bsptree.h"
#include "game.h"
#include "quest.h"
#include "tex.h"
#include "undo.h"

// undefine to get rid of memory tracking code
// #define DEBUG

#ifdef DEBUG

#if 0
   vec3_t v;
   int i,j,k,l;
   float f1=4096,f2=1.0/128.0;

/*   asm volatile
   ("finit");*/


//   printf("%08x %08x\n",_control87(0,0),_status87());
   _control87(0x3fe,-1);
//   printf("%08x %08x\n",_control87(0,0),_status87());

   signal(SIGFPE,test);

/*   start_time=0;end_time=start_time+1;
   printf("%g\n",end_time/start_time);*/
   printf("%08x %08x\n",_control87(0,0),_status87());

   v.z=0;
#define TV(a, b)                                         \
  v.x = a;                                               \
  v.y = b;                                               \
  /*   printf("%5i (%8g %8g)\n",HashVec(&v),v.x,v.y); */ \
  printf("%5i\n", HashVec(&v));                          \
  printf("%08x %08x\n", _control87(0, 0), _status87());

/*   TV(100,100)

   TV(10000,100)
   TV(100,10000)

   TV(-10000,100)
   TV(100,-10000)

   TV(10000,-10000)
   TV(-10000,10000)

   TV(10000,10000)
   TV(-10000,-10000)*/

   i=0xffffffff;
//   TV(*((float *)&i),100)
//   TV(100,*((float *)&i))
//   TV(*((float *)&i),*((float *)&i))

/*   asm volatile("
	flds %0
	fadds (%%eax)
   flds %1
	fmulp %%st,%%st(1)
	fnstcw %2
	movl %2,%%edx
	movb $12,%%dh
	movl %%edx,%3
	fldcw %3
	fistpl %4
	fldcw %2
   "
   : "=m" (f1),"=m" (f2),"=m" (j),"=m" (k),"=m" (l)
   : "a" (&i)
   : "%eax","%edx");

   CheckFPU();*/
   CheckFPU();

   printf("j=%08x k=%08x l=%i\n",j,k,l);

#undef TV


/*   asm volatile
   ("
      fstp %st(1)
      fwait
      fstp %st(2)
      fwait
      fstp %st(3)
      fwait
   ");
   printf("%08x %08x\n",_control87(0,0),_status87());*/

//   CheckFPU();


   exit(0);
#endif

#ifdef DJGPP

typedef unsigned short word16;
typedef unsigned int word32;

typedef struct
{
  word16 sig0;
  word16 sig1;
  word16 sig2;
  word16 sig3;
  word16 exponent : 15;
  word16 sign : 1;
} NPXREG;

typedef struct
{
  word32 control;
  word32 status;
  word32 tag;
  word32 eip;
  word32 cs;
  word32 dataptr;
  word32 datasel;
  NPXREG reg[8];
} NPX;

static NPX npx;

static inline void
save_npx(void)
{
  asm("inb     $0xa0, %%al
      testb $0x20,
      % % al jz 1f xorb % % al,
      % % al outb % % al,
      $0xf0
        movb $0x20,
      % % al outb % % al,
      $0xa0
          outb %
        % al,
      $0x20 1 : fnsave % 0 fwait "
      : "=m"(npx)
      : /* No input */
      : "%eax");
}

static inline void
load_npx(void)
{
  asm("frstor %0" : "=m"(npx));
}

void
CheckFPU(void)
{
  char buf[2048];

  save_npx();

  /*   printf("c=%08x s=%08x eip=%08x cs=%08x dp=%08x ds=%08x\n",
        npx.control,npx.status,npx.eip,npx.cs,npx.dataptr,npx.datasel);
     printf("  0x%08x\n",npx.eip);*/

  if (npx.status & 0x5f)
  {
    sprintf(buf, "status=%04x eip=%08x ", npx.status, npx.eip);
#define C(a, b)       \
  if (npx.status & a) \
    strcat(buf, b);
    C(1, ", invalid operation")
    C(2, ", denormalized operand")
    C(4, ", division by zero")
    C(8, ", overflow")
    C(16, ", underflow")
    C(32, ", loss of precision")
    C(64, ", stack over/under flow")
    C(128, ", set if any errors")
#undef C
    fprintf(stderr, "%s", buf);
    HandleError("CheckFPU", buf);
  }

  /*   {
     int i;
     int tos;
     int tag;

     tos=(npx.status>>11)&7;
     for (i=0;i<8;i++)
     {
        NPXREG *n=&npx.reg[i];
        int e;
        long long int f;
        long double num;

        printf("%i: ",i);

        tag=(npx.tag>>(((i+tos)&7)*2))&3;
        switch (tag)
        {
        case 0:
           num=*(long double *)&npx.reg[i];
           printf("%Lg",num);
           break;

        case 1:
           printf("Zero");
           break;

        case 2:
           if (n->sign) printf("-"); else printf("+");

           e=n->exponent;
           f=(((long long int)n->sig0)<< 0)+
             (((long long int)n->sig1)<<16)+
             (((long long int)n->sig2)<<32)+
             (((long long int)n->sig3)<<48);

           if (e==32767)
           {
              if (f==0x8000000000000000L)
                 printf("Inf");
              else
                 printf("Nan");
           }
           else
              printf("Unknown");
           break;

        case 3:
           printf("Empty");
           break;
        }
        printf("\n");
     }
    }*/
  npx.status &= ~0x3f;
  load_npx();
}
#endif

#if 0
static int D_MapTextureSize(void)
{
   int total=0;
   int i;
   texture_t *t;

   total+=M.num_textures*sizeof(texture_t);
   if (!Game.tex.cache)
      for (t=M.Cache,i=0;i<M.num_textures;i++,t++)
         total+=t->sx*t->sy;

   printf("textures=%i ",total);

   return total;
}

static int D_BSPSize_r(bsp_node_t *n)
{
   int t=sizeof(bsp_node_t);
   bsp_face_t **f;
   int i;

   if (n->FrontNode) t+=D_BSPSize_r(n->FrontNode);
   if (n->BackNode) t+=D_BSPSize_r(n->BackNode);

   t+=(sizeof(bsp_face_t *)+sizeof(bsp_face_t))*n->numfaces;

   for (f=n->faces,i=0;i<n->numfaces;i++,f++)
      t+=sizeof(int)*(*f)->numpts;

   return t;
}

/* Defined in bspspan.c, needs access to static stuff there. */
int D_BSPSpanSize(void);

static int D_BSPSize(void)
{
   int total=0,i;

   if (M.BSPHead)
   {
      i=M.uniqueverts*(sizeof(vec3_t)+sizeof(vec3_vt)+sizeof(int))+
        sizeof(int)*HASH_SIZE*HASH_SIZE;
      printf("verts=%i ",i);
      total+=i;
   
      i=M.num_mf*(sizeof(bsp_map_face_t *)+sizeof(bsp_map_face_t));
      printf("mf=%i ",i);
      total+=i;
   
      i=D_BSPSize_r(M.BSPHead);
      printf("tree=%i ",i);
      total+=i;

      i=D_BSPSpanSize();
      printf("spans=%i ",i);
      total+=i;
   }

   return total;
}

static int D_MapGroupSize(void)
{
   group_t *g;
   int total=0;

   for (g=M.GroupHead;g;g=g->Next)
      total+=sizeof(group_t);

   return total;
}

static int D_TotalMapSize(void)
{
   int total=0;
   int i;

   vsel_t *v;
   fsel_t *f;
   brushref_t *b;
   entityref_t *e;

   i=MapSize();
   printf("mapsize=%i ",i);
   total+=i;

   i=UndoSize();
   printf("undosize=%i ",i);
   total+=i;

   i=0;
#define ADD(x, y, z)                    \
  for (x = M.display.y; x; x = x->Next) \
    i += sizeof(z);

   ADD(v,vsel,vsel_t);
   ADD(f,fsel,fsel_t);
   ADD(e,esel,entityref_t);
   ADD(b,bsel,brushref_t);

#undef ADD
   printf("select=%i ",i);
   total+=i;

   i=D_MapGroupSize();
   printf("groups=%i ",i);
   total+=i;

   total+=D_MapTextureSize();

   i=M.npts*sizeof(vec3_t)*2;
   printf("pts=%i ",i);
   total+=i;

   total+=D_BSPSize();

   printf("total=%i\n",total);

   return total;
}

static int D_SizeBrush(brush_t *b)
{
   int total=0;
   int i;
   plane_t *p;

   total+=sizeof(brush_t);
   total+=(sizeof(vec3_t)*2+sizeof(svec_t))*b->num_verts;
   total+=sizeof(edge_t)*b->num_edges;

   total+=sizeof(plane_t)*b->num_planes;
   for (p=b->plane,i=0;i<b->num_planes;i++,p++)
      total+=sizeof(p->num_verts)*sizeof(int);

   return total;
}

static int D_SizeEntity(entity_t *e)
{
   int total=0;
   int i;

   total+=sizeof(entity_t);
   total+=sizeof(char **)*2*e->numkeys;
   for (i=0;i<e->numkeys;i++)
      total+=strlen(e->key[i])+1 + strlen(e->value[i])+1;

   return total;
}

static int D_ClipboardSize(void)
{
   entity_t *e;
   brush_t *b;
   int total;

   total=0;
   for (b=Clipboard.brushes;b;b=b->Next)
      total+=D_SizeBrush(b);
   for (b=Clipboard.ent_br;b;b=b->Next)
      total+=D_SizeBrush(b);
   for (e=Clipboard.entities;e;e=e->Next)
      total+=D_SizeEntity(e);

   printf("clipboard: %i\n",total);
   return total;
}

static int D_TextureCacheSize(void)
{
   int total=0;

   return total;
}

static int D_GetKnownMemSize(void)
{
   int total=0;
   int i;
   int old_map;

   old_map=cur_map;
   for (i=0;i<MAX_MAPS;i++)
   {
      printf("map %i: ",i);
      SwitchMap(i,0);
      total+=D_TotalMapSize();
   }
   SwitchMap(old_map,0);

   total+=D_ClipboardSize();

   if (Game.tex.cache)
      total+=D_TextureCacheSize();

   printf("total: %i\n",total);
   return total;
}
#endif

static int first = 0;

static int o_memknown;
static int o_memused;
static int o_maxused;

// static int sysmem,o_sysmem;

#define WriteNew(x)                                                           \
  {                                                                           \
    NewMessage("Leak? New " #x " %i  old %i diff %i\n", x, o_##x, x - o_##x); \
    o_##x = x;                                                                \
  }

#define SetNew(x) o_##x = x;

#endif /* DEBUG */

void
CheckLeak(void)
{
#ifdef DEBUG
  int d1, d2;
  int memknown;

  if (!first)
  {
    printf("--- first ---\n");

    o_memused = memused;
    o_maxused = maxused;
    o_memknown = /*D_GetKnownMemSize()*/ 0;

    first++;
    return;
  }

#ifdef DJGPP
  CheckFPU();
#endif

  if (memused == o_memused)
    return;

  printf("--- diff ---\n");
  memknown = /*D_GetKnownMemSize()*/ 0;

  d1 = o_memused - o_memknown;
  d2 = memused - memknown;

  if (d1 != d2)
  {
    HandleError("CheckLeak", "L: old (%i %i/%i) new (%i %i/%i)", d1, o_memknown, o_memused, d2, memknown, memused);
    NewMessage("L: old (%i %i/%i) new (%i %i/%i)",
               d1,
               o_memknown,
               o_memused,
               d2,
               memknown,
               memused);
  }
  printf("L: old (%i %i/%i) new (%i %i/%i)\n\n",
         d1,
         o_memknown,
         o_memused,
         d2,
         memknown,
         memused);

  o_memused = memused;
  o_memknown = memknown;

/*   if (memused!=o_memused)
   {
      WriteNew(memused);
   }*/

/*   if (maxused!=o_maxused)
   {
      WriteNew(maxused);
   }*/

/*   sysmem=(int)sbrk(0);
   if (sysmem!=o_sysmem)
   {
      NewMessage("%5i kb core allocated (%i,%i mb)",sysmem/1024,sysmem,sysmem/1024/1024);
      SetNew(sysmem);
   }*/
#endif /* DEBUG */
}
