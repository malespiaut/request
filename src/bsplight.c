/*
bsplight.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "bsplight.h"

#include "brush.h"
#include "bspleak.h"
#include "entity.h"
#include "game.h"
#include "geom.h"
#include "message.h"
#include "quest.h"
#include "status.h"
#include "tex.h"

#define MAXLIGHTS 2048

typedef struct
{
  vec3_t org;
  vec3_t normal;
  int type;
  int level;
  float col[3];
} light_t;

static light_t lights[MAXLIGHTS];
static int nlights;

static light_t* vlights[MAXLIGHTS];
static int nvlights;

static int glc;

static int g_shadows;

static vec3_t normal;

static float
GetAlQ1(light_t* l, vec3_t pos)
{
  vec3_t d;
  float dist;
  float dot;
  float al;

  d.x = l->org.x - pos.x;
  d.y = l->org.y - pos.y;
  d.z = l->org.z - pos.z;

  dist = d.x * d.x + d.y * d.y + d.z * d.z;
  if (dist > l->level * l->level)
    return 0;

  dot = DotProd(normal, d);
  if (dot < -0.01)
    return 0;

  dist = sqrt(dist);
  al = (l->level - dist) * (0.5 + 0.5 * dot / dist);

  if (al < 1)
    return 0;

  if (g_shadows)
    if (!Trace(l->org, pos))
      return 0;

  return al;
}

static float
GetAlQ2(light_t* l, vec3_t pos)
{
  vec3_t d;
  float dist;
  float dot, dot2;
  float al;

  d.x = l->org.x - pos.x;
  d.y = l->org.y - pos.y;
  d.z = l->org.z - pos.z;

  dist = d.x * d.x + d.y * d.y + d.z * d.z;

  dot = DotProd(normal, d);

  if (dot < -0.01)
    return 0;

  if (l->type)
  {
    if (dist > l->level)
      return 0;

    dot2 = -DotProd(l->normal, d);

    if (dot2 < -0.01)
      return 0;
    al = ((float)l->level) * dot * dot2 / (dist * dist);
  }
  else
  {
    if (dist > l->level * l->level)
      return 0;
    dist = sqrt(dist);
    al = (l->level - dist) * dot / dist;
  }

  if (al < 1)
    return 0;

  if (g_shadows)
    if (!Trace(l->org, pos))
      return 0;

  return al;
}

static float (*GetAl)(light_t* l, vec3_t pos);

float
GetLight(vec3_t pos)
{
  int i;
  float al;
  float light;

  glc++;

  light = 0;

  pos.x += normal.x;
  pos.y += normal.y;
  pos.z += normal.z;

  for (i = 0; i < nvlights; i++)
  {
    al = GetAl(vlights[i], pos);

    light += al;
  }

  if (light < 0)
    light = 0;
  else if (light > 255)
    light = 1;
  else
    light /= 255;

  return light;
}

void
GetLightCol(vec3_t pos, float* r, float* g, float* b)
{
  int i;
  light_t* l;
  float al;
  float col[3];
  float c;

  glc++;

  col[0] = col[1] = col[2] = 0;

  pos.x += normal.x;
  pos.y += normal.y;
  pos.z += normal.z;

  for (i = 0; i < nvlights; i++)
  {
    l = vlights[i];

    al = GetAl(l, pos);

    col[0] += l->col[0] * al;
    col[1] += l->col[1] * al;
    col[2] += l->col[2] * al;
  }

  if (col[0] < 0)
    col[0] = 0;
  if (col[1] < 0)
    col[1] = 0;
  if (col[2] < 0)
    col[2] = 0;

  col[0] *= 3;
  col[1] *= 3;
  col[2] *= 3;

  c = col[0];
  if (col[1] > c)
    c = col[1];
  if (col[2] > c)
    c = col[2];
  if (c > 255)
  {
    c = 1 / c;
    *r = col[0] * c;
    *g = col[1] * c;
    *b = col[2] * c;
  }
  else
  {
    *r = col[0] / 255;
    *g = col[1] / 255;
    *b = col[2] / 255;
  }
}

void
FaceLight(vec3_t norm, float dist)
{
  float t;
  int i;
  light_t* l;

  normal = norm;
  nvlights = 0;
  for (i = 0; i < nlights; i++)
  {
    l = &lights[i];
    t = DotProd(norm, l->org) - dist;
    if (t < 0)
      continue;

    if (l->type)
    {
      if (t * t > l->level)
        continue;
    }
    else
    {
      if (t > l->level)
        continue;
    }
    vlights[nvlights++] = l;
  }
}

void
InitLights(int shadows)
{
  entity_t* e;
  const char* key;
  light_t* l;
  float temp;

#if 0
   brush_t *b;
   plane_t *p;
   int i,j;
   texture_t *t;
   int light;
   float col[3];
   vec3_t v1,v2,cr;
   float area;
#endif

  g_shadows = shadows;
  DoneLights();

  for (e = M.EntityHead; e && (nlights < MAXLIGHTS); e = e->Next)
  {
    if (strncmp(GetKeyValue(e, "classname"), "light", 5))
      continue;

    key = GetKeyValue(e, "origin");
    if (!key)
      continue;

    l = &lights[nlights++];
    sscanf(key, "%f %f %f", &l->org.x, &l->org.y, &l->org.z);
    l->type = 0;

    key = GetKeyValue(e, "light");
    if (key)
    {
      l->level = atoi(key);
      if (l->level <= 0)
        l->level = 300;
    }
    else
      l->level = 300;

    key = GetKeyValue(e, "_color");
    if (key)
    {
      sscanf(key, "%f %f %f", &l->col[0], &l->col[1], &l->col[2]);
      temp = l->col[0];
      if (l->col[1] > temp)
        temp = l->col[1];
      if (l->col[2] > temp)
        temp = l->col[2];
      if (temp)
      {
        temp = 1 / temp;
        l->col[0] *= temp;
        l->col[1] *= temp;
        l->col[2] *= temp;
      }
    }
    else
      l->col[0] = l->col[1] = l->col[2] = 1;
  }
#if 0 /* needs to be updated, never really worked */
   if (Game.light.lfaces && Game.tex.q2flags)
   {
      for (b=M.BrushHead;b;b=b->Next)
      {
         if (b->bt->flags&BR_F_BTEXDEF)
            continue;
      
         for (i=0;i<b->num_planes;i++)
         {
            p=&b->plane[i];
            light=0;
            t=ReadMIPTex(p->tex.name,0);
            if (p->tex.g.q2.flags&1)
               light=p->tex.g.q2.value;
            else
            {
               if (t)
                  if (t->g.q2.flags&1)
                     light=t->g.q2.value;
            }
            if (light)
            {
               l=&lights[nlights++];
               l->type=1;
               l->level=light;

               l->org.x=p->center.x+p->normal.x*2;
               l->org.y=p->center.y+p->normal.y*2;
               l->org.z=p->center.z+p->normal.z*2;

               l->normal=p->normal;

               if (t)
               {
                  GetTexColor(t);
                  col[0]=t->colv[0];
                  col[1]=t->colv[1];
                  col[2]=t->colv[2];

                  temp=col[0];
                  if (col[1]>temp) temp=col[1];
                  if (col[2]>temp) temp=col[2];

                  if (temp<0.5)
                     temp*=2;
                  else
                     temp=1;
               }
               else
               {
                  temp=col[0]=col[1]=col[2]=0.5;
               }

               area=0;
               for (j=2;j<p->num_verts;j++)
               {
                  v1.x=b->verts[p->verts[j  ]].x-b->verts[p->verts[0]].x;
                  v1.y=b->verts[p->verts[j  ]].y-b->verts[p->verts[0]].y;
                  v1.z=b->verts[p->verts[j  ]].z-b->verts[p->verts[0]].z;

                  v2.x=b->verts[p->verts[j-1]].x-b->verts[p->verts[0]].x;
                  v2.y=b->verts[p->verts[j-1]].y-b->verts[p->verts[0]].y;
                  v2.z=b->verts[p->verts[j-1]].z-b->verts[p->verts[0]].z;

                  CrossProd(v1,v2,&cr);

                  area+=sqrt(cr.x*cr.x+cr.y*cr.y+cr.z*cr.z);
               }

               l->level=((float)l->level)*temp*0.4*area;

               if (temp)
               {
                  temp=1/temp;
                  l->col[0]=col[0]*temp;
                  l->col[1]=col[1]*temp;
                  l->col[2]=col[2]*temp;
               }
               else
               {
                  l->col[0]=l->col[1]=l->col[2]=0;
               }
            }
         }
      }
   }
#endif

  if (shadows)
  {
    if (!TraceBSPInit())
    {
      g_shadows = 0;
      NewMessage("Unable to build BSP tree for ray tracing!");
    }
  }

  //   printf("InitLights(): %i lights\n",nlights);
  glc = 0;

  if (Game.light.model == L_QUAKE2)
    GetAl = GetAlQ2;
  else
    GetAl = GetAlQ1;
}

void
DoneLights(void)
{
  if (g_shadows)
    TraceBSPDone();

  nlights = 0;
  //   printf("GetLight() called %i times\n",glc);
}
