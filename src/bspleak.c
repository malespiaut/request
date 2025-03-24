/*
bspleak.c file of the Quest Source Code

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

#include "bspleak.h"

#include "brush.h"
#include "clip.h"
#include "entity.h"
#include "error.h"
#include "game.h"
#include "geom.h"
#include "memory.h"
#include "message.h"
#include "quest.h"
#include "status.h"
#include "tex.h"
#include "times.h"

static int bsp_error;

#define ERR_NO 0     // no error
#define ERR_NOMEM 1  // out of memory
#define ERR_BSP 2    // error building BSP tree
#define ERR_PORTAL 3 // error portalizing

static const char* bsp_err_msg[] =
  {
    "Unknown error!",
    "Out of memory!",
    "Error building BSP tree!",
    "Error portalizing world!"};

typedef struct vec3_vs
{
  float x, y, z;
} vec3_vt;

typedef struct
{
  vec3_t normal;
  float dist;
} bsp_plane_t;

typedef struct
{
  vec3_t* pts; // these get freed.. dont reference after tree is built
  int numpts;

  bsp_plane_t plane;
} bsp_face_t;

typedef struct bsp_node_s
{
  struct bsp_node_s* FrontNode;
  struct bsp_node_s* BackNode;

  int contents;

  entity_t* ent;
  int dist;

  bsp_plane_t plane;

  struct portal_s* portals;
} bsp_node_t;

#define CONTENTS_EMPTY -1 // -1: node is empty space
#define CONTENTS_SPLIT 0  //  0: node splits space
#define CONTENTS_SOLID 1  // >0: node is solid space

typedef struct portal_s
{
  struct portal_s* next[2];
  bsp_node_t* node[2];

  bsp_face_t face;
} portal_t;

static bsp_node_t* BSPHead;
static bsp_node_t outside;

static float min[3], max[3];

static void DeleteBSPNode(bsp_node_t* Root);

static void
DeleteBSPTree(void)
{
  if (BSPHead)
    DeleteBSPNode(BSPHead);
  BSPHead = NULL;
}

#define MY_DotProd(v1, v2, v3)                                                    \
  {                                                                               \
    (v3) = (((v1).x) * ((v2).x)) + (((v1).y) * ((v2).y)) + (((v1).z) * ((v2).z)); \
  }

static int
BSPFaceSplit(bsp_face_t* F, bsp_plane_t* p, bsp_face_t** FrontFace, bsp_face_t** BackFace)
/*
  Returns:  0 - Face was clipped successfully
           -1 - Face was completely behind the plane
           -2 - Face was completely in front of the plane
           -3 - Error clipping / culling face
           -4 - Face was completely ON the plane
*/
{
  vec3_t p1, p2, p3;
  int i, j;
  int backtest = 0;
  int fronttest = 0;
  int ontest = 0;
  int splitnum = 0;
  int sides[1024];
  float dists[1024];
  vec3_t newfront[1024];
  vec3_t newback[1024];
  float dot;
  int numnewfront;
  int numnewback;

  /* determine sides and distances for each point*/
  for (i = 0; i < F->numpts; i++)
  {
    MY_DotProd(F->pts[i], p->normal, dot);
    dot -= (p->dist);
    if (dot > 0.01)
    {
      sides[i] = 1;
      fronttest = 1;
      dists[i] = dot;
    }
    else
    {
      if (dot < -0.01)
      {
        sides[i] = -1;
        backtest = 1;
        dists[i] = dot;
      }
      else
      {
        dists[i] = 0;
        sides[i] = 0;
        ontest = 1;
      }
    }
  }

  if ((ontest == 1) && (backtest == 0) && (fronttest == 0))
  {
    (*FrontFace) = NULL;
    (*BackFace) = NULL;
    return -4;
  }

  if (((backtest == 0) && (fronttest == 1)) || ((backtest == 1) && (fronttest == 0)))
  {
    /*if all in front*/
    if (fronttest)
    {
      (*FrontFace) = NULL;
      (*BackFace) = NULL;
      return -2;
    }
    /*if all in back*/
    if (backtest)
    {
      (*FrontFace) = NULL;
      (*BackFace) = NULL;
      return -1;
    }
  }

  numnewfront = 0;
  numnewback = 0;
  for (i = 0; i < F->numpts; i++)
  {
    p1 = F->pts[i];

    if (i == F->numpts - 1)
      j = 0;
    else
      j = i + 1;

    if (sides[i] == 0)
    {
      newfront[numnewfront] = p1;
      numnewfront++;
      newback[numnewback] = p1;
      numnewback++;
      if ((splitnum == 2) || (sides[j] == 0))
      {
        return -3;
      }
      splitnum++;
      continue;
    }

    if (sides[i] == 1)
    {
      newfront[numnewfront] = p1;
      numnewfront++;
    }
    if (sides[i] == -1)
    {
      newback[numnewback] = p1;
      numnewback++;
    }

    if ((sides[j] == sides[i]) || (sides[j] == 0))
      continue;

    /* generate a split point*/
    p2 = F->pts[j];

    dot = dists[i] / (dists[i] - dists[j]);

    if (p->normal.x == 1)
      p3.x = p->dist;
    else if (p->normal.x == -1)
      p3.x = -p->dist;
    else
      p3.x = p1.x + dot * (p2.x - p1.x);

    if (p->normal.y == 1)
      p3.y = p->dist;
    else if (p->normal.y == -1)
      p3.y = -p->dist;
    else
      p3.y = p1.y + dot * (p2.y - p1.y);

    if (p->normal.z == 1)
      p3.z = p->dist;
    else if (p->normal.z == -1)
      p3.z = -p->dist;
    else
      p3.z = p1.z + dot * (p2.z - p1.z);

    newfront[numnewfront] = p3;
    numnewfront++;
    newback[numnewback] = p3;
    numnewback++;
    if ((splitnum == 2) || (sides[j] == 0))
    {
      return -3;
    }
    splitnum++;
  }

  if (splitnum != 2)
    return -3;

  (*FrontFace) = (bsp_face_t*)Q_malloc(sizeof(bsp_face_t));
  if ((*FrontFace) == NULL)
  {
    return -3;
  }
  (*FrontFace)->plane = F->plane;

  (*BackFace) = (bsp_face_t*)Q_malloc(sizeof(bsp_face_t));
  if ((*BackFace) == NULL)
  {
    Q_free((*FrontFace));
    return -3;
  }
  (*BackFace)->plane = F->plane;

  (*FrontFace)->pts = (vec3_t*)Q_malloc(sizeof(vec3_t) * numnewfront);
  if ((*FrontFace)->pts == NULL)
  {
    Q_free((*BackFace));
    Q_free((*FrontFace));
    return -3;
  }
  (*FrontFace)->numpts = numnewfront;

  (*BackFace)->pts = (vec3_t*)Q_malloc(sizeof(vec3_t) * numnewback);
  if ((*BackFace)->pts == NULL)
  {
    Q_free((*FrontFace)->pts);
    Q_free((*BackFace));
    Q_free((*FrontFace));
    return -3;
  }
  (*BackFace)->numpts = numnewback;

  for (i = 0; i < numnewfront; i++)
    (*FrontFace)->pts[i] = newfront[i];
  for (i = 0; i < numnewback; i++)
    (*BackFace)->pts[i] = newback[i];

  return 0;
}

static bsp_node_t*
NewNode(void)
{
  bsp_node_t* newnode;

  newnode = (bsp_node_t*)Q_malloc(sizeof(bsp_node_t));
  if (newnode == NULL)
  {
    bsp_error = ERR_NOMEM;
    return NULL;
  }
  memset(newnode, 0, sizeof(bsp_node_t));
  newnode->contents = CONTENTS_EMPTY;

  return newnode;
}

static int curbrush;

static int
AddToTree(bsp_face_t* F, bsp_node_t* Root, int onplane)
{
  bsp_face_t* Front;
  bsp_face_t* Back;

  if (Root->contents != CONTENTS_SPLIT)
  {
    if ((Root->contents < curbrush) && (Root->contents >= CONTENTS_SOLID))
    {
      Q_free(F->pts);
      Q_free(F);
      return 1;
    }
    if (onplane)
    {
      Root->contents = curbrush;
    }
    else
    {
      Root->FrontNode = NewNode();
      Root->BackNode = NewNode();
      Root->plane = F->plane;

      Root->FrontNode->contents = CONTENTS_EMPTY;
      Root->BackNode->contents = curbrush;

      Root->contents = CONTENTS_SPLIT;
    }

    Q_free(F->pts);
    Q_free(F);
    return 1;
  }

  switch (BSPFaceSplit(F, &Root->plane, &Front, &Back))
  {
    case -4:
      /*      printf(" face (%g %g %g) %g on plane (%g %g %g) %g, ",
               F->plane.normal.x,
               F->plane.normal.y,
               F->plane.normal.z,
               F->plane.dist,
               Root->plane.normal.x,
               Root->plane.normal.y,
               Root->plane.normal.z,
               Root->plane.dist);*/

      if ((fabs(Root->plane.normal.x - F->plane.normal.x) < 0.1) &&
          (fabs(Root->plane.normal.y - F->plane.normal.y) < 0.1) &&
          (fabs(Root->plane.normal.z - F->plane.normal.z) < 0.1))
      {
        //         printf("AddToTree(back)\n");
        return AddToTree(F, Root->BackNode, 1);
      }
      else
      {
        //         printf("AddToTree(front)\n");
        return AddToTree(F, Root->FrontNode, 1);
      }
      break;

    case -1:
      return AddToTree(F, Root->BackNode, onplane);
      break;

    case -2:
      return AddToTree(F, Root->FrontNode, onplane);
      break;

    case 0:
      Q_free(F->pts);
      Q_free(F);

      if (!AddToTree(Back, Root->BackNode, onplane))
        return FALSE;

      if (!AddToTree(Front, Root->FrontNode, onplane))
        return FALSE;

      return TRUE;
      break;

    case -3:
      bsp_error = ERR_BSP;
      return FALSE;
      break;
  }
  return FALSE;
}

static bsp_face_t*
MakeBSPFace(brush_t* b, vec3_t* verts, plane_t* p)
{
  bsp_face_t* F;
  int i;

  F = (bsp_face_t*)Q_malloc(sizeof(bsp_face_t));
  if (!F)
  {
    bsp_error = ERR_NOMEM;
    return NULL;
  }
  memset(F, 0, sizeof(bsp_face_t));

  F->numpts = p->num_verts;

  F->pts = (vec3_t*)Q_malloc(sizeof(vec3_t) * (F->numpts));
  if (!F->pts)
  {
    bsp_error = ERR_NOMEM;
    Q_free(F);
    return NULL;
  }
  F->plane.normal = p->normal;
  F->plane.dist = p->dist;

  for (i = 0; i < F->numpts; i++)
  {
    F->pts[i] = verts[p->verts[i]];

    if (F->pts[i].x > max[0])
      max[0] = F->pts[i].x;
    if (F->pts[i].x < min[0])
      min[0] = F->pts[i].x;

    if (F->pts[i].y > max[1])
      max[1] = F->pts[i].y;
    if (F->pts[i].y < min[1])
      min[1] = F->pts[i].y;

    if (F->pts[i].z > max[2])
      max[2] = F->pts[i].z;
    if (F->pts[i].z < min[2])
      min[2] = F->pts[i].z;
  }

  return F;
}

static int firstface;

static void
BSPAddBrush(brush_t* b)
{
  int i;

  bsp_face_t* F;
  plane_t* p;

  int flags;

  if (b->Group->flags & 2)
    return;

  if (b->bt->type != BR_NORMAL)
    return;

  if (b->EntityRef != M.WorldSpawn)
    return;

  for (i = 0; i < b->num_planes; i++)
  {
    p = &b->plane[i];

    if (Game.tex.gettexbspflags)
      flags = Game.tex.gettexbspflags(&p->tex);
    else
      flags = 0;

    if (flags & TEX_NONSOLID)
      continue;

    F = MakeBSPFace(b, b->verts, p);
    if (F == NULL)
      break;
    if (firstface)
    {
      BSPHead = NewNode();
      if (!BSPHead)
        break;
      firstface = FALSE;
    }

    if (!AddToTree(F, BSPHead, 0))
      break;
  }
}

/*static void FixTemp_r(bsp_node_t *n)
{
   if (n->contents==CONTENTS_TEMP)
   {
      n->contents=CONTENTS_SOLID;
      return;
   }
   if (n->contents==CONTENTS_SPLIT)
   {
      FixTemp_r(n->FrontNode);
      FixTemp_r(n->BackNode);
   }
}*/

/* Main Tree Functions */

static int
BuildBSPTree(void)
{
  float start_time, end_time;

  brush_t* b;

  start_time = GetTime();
  firstface = TRUE;

  bsp_error = ERR_NO;

  curbrush = 1;
  for (b = M.BrushHead; (b && (bsp_error == ERR_NO)); b = b->Next)
  {
    BSPAddBrush(b);
    curbrush++;
  }

  end_time = GetTime();

  if (bsp_error)
  {
    DeleteBSPTree();
    HandleError("BSP Build", "%s", bsp_err_msg[bsp_error]);
    return 0;
  }

  NewMessage("BSP built in %g seconds", end_time - start_time);

  return 1;
}

#if 0
static void PBT(bsp_node_t *n,int level)
{
   if (n->FrontNode)
   {
      printf("%*c split (%g %g %g) %g\n",level,' ',
         n->plane.normal.x,
         n->plane.normal.y,
         n->plane.normal.z,
         n->plane.dist);
 
      if (n->BackNode)
      {
         PBT(n->FrontNode,level+1);
         PBT(n->BackNode,level+1);
      }
      else
      {
         printf("%*c odd, 1 child\n",level,' ');
         PBT(n->FrontNode,level+1);
      }
   }
   else
   if (n->BackNode)
   {
      printf("%*c split (%g %g %g) %g\n",level,' ',
         n->plane.normal.x,
         n->plane.normal.y,
         n->plane.normal.z,
         n->plane.dist);

      printf("%*c odd, 1 child\n",level,' ');
      PBT(n->BackNode,level+1);
   }
   else
   {
      printf("%*c leaf %i\n",level,' ',n->contents);
   }
}
#endif

static int
MakeFaceOnPlane(bsp_face_t* f, bsp_plane_t* p)
{
  face_my_t F;
  int i;

  memset(&F, 0, sizeof(F));
  F.normal = p->normal;
  F.dist = p->dist;

  MakeBoxOnPlane(&F);

  f->numpts = F.numpts;
  f->pts = Q_malloc(f->numpts * sizeof(vec3_t));
  if (!f->pts)
  {
    bsp_error = ERR_NOMEM;
    return 0;
    //      Abort("MakeFaceOnPlane","Out of memory!");
  }

  for (i = 0; i < f->numpts; i++)
    f->pts[i] = F.pts[i];

  f->plane = *p;

  return 1;
}

static void
AddPortalToNodes(portal_t* p, bsp_node_t* n1, bsp_node_t* n2)
{
  p->node[0] = n1;
  p->node[1] = n2;

  p->next[0] = n1->portals;
  n1->portals = p;

  p->next[1] = n2->portals;
  n2->portals = p;
}

static void
RemovePortal(portal_t* p, bsp_node_t* n)
{
  portal_t* p1;
  int s, s2;

  s2 = (p->node[1] == n);
  if (p == n->portals)
  {
    n->portals = p->next[s2];
    p->next[s2] = NULL;
    return;
  }

  for (p1 = n->portals; p1; p1 = p1->next[s])
  {
    s = (p1->node[1] == n);
    if (p1->next[s] == p)
    {
      p1->next[s] = p->next[s2];
      p->next[s2] = NULL;
      return;
    }
  }

  NewMessage("Warning: RemovePortal error!");
  //   Abort("RemovePortal","???");
}

static int
MakeHeadPortals(void)
{
  portal_t* hp[6];
  int i, j, k;
  int val;
  portal_t* p;

  bsp_plane_t pl[6];
  bsp_face_t *front, *back;

  k = 0;
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < 3; j++)
    {
      pl[k].normal.x = pl[k].normal.y = pl[k].normal.z = 0;
      if (i)
      {
        val = -1;
        pl[k].dist = -min[j];
      }
      else
      {
        val = 1;
        pl[k].dist = max[j];
      }

      switch (j)
      {
        case 0:
          pl[k].normal.x = val;
          break;
        case 1:
          pl[k].normal.y = val;
          break;
        case 2:
          pl[k].normal.z = val;
          break;
      }

      hp[k] = p = Q_malloc(sizeof(portal_t));
      if (!hp[k])
      {
        bsp_error = ERR_NOMEM;
        return 0;
        //            Abort("HeadNode","Out of memory!");
      }
      memset(p, 0, sizeof(portal_t));

      if (!MakeFaceOnPlane(&hp[k]->face, &pl[k]))
        return 0;

      k++;
    }
  }

  for (i = 0; i < 6; i++)
  {
    for (j = 0; j < 6; j++)
    {
      if (i == j)
        continue;

      k = BSPFaceSplit(&hp[i]->face, &pl[j], &front, &back);
      switch (k)
      {
        case 0:
          Q_free(front->pts);
          Q_free(front);
          Q_free(hp[i]->face.pts);
          hp[i]->face = *back;
          Q_free(back);
          break;
        case -3:
        case -4:
          bsp_error = ERR_PORTAL;
          return 0;
          //            Abort("HeadNode","???");
          break;
      }
    }
    AddPortalToNodes(hp[i], &outside, BSPHead);
  }

  return 1;
}

static int
MakePortals_r(bsp_node_t* n)
{
  portal_t* p;
  portal_t* p2;
  int s;
  int i;

  bsp_face_t *front, *back;

  bsp_node_t* other;
  portal_t* next;

  if (n->contents != CONTENTS_SPLIT)
    return 1;

  /*   printf("new node\n");
     for (p2=n->portals;p2;p2=p2->next[s])
     {
        s=(p2->node[1]==n);
        printf("  portal:\n");
        for (i=0;i<p2->face.numpts;i++)
        {
           printf("   %3i: %g %g %g\n",i,
              p2->face.pts[i].x,
              p2->face.pts[i].y,
              p2->face.pts[i].z);
        }
     }*/

  // create a new portal between the children and clip it to the other portals
  p = Q_malloc(sizeof(portal_t));
  if (!p)
  {
    bsp_error = ERR_NOMEM;
    return 0;
    //      Abort("MakePortals","Out of memory!");
  }
  memset(p, 0, sizeof(portal_t));
  p->face.plane = n->plane;

  if (!MakeFaceOnPlane(&p->face, &n->plane))
    return 0;

  //   printf("new portal\n");
  for (p2 = n->portals; p2; p2 = p2->next[s])
  {
    s = (p2->node[1] == n);

    /*      if ((p2->face.plane.dist==192) && (p2->face.plane.normal.x==-1))
          {
             printf(" before clip:\n");
             for (i=0;i<p->face.numpts;i++)
             {
                printf("   %3i: %g %g %g\n",i,
                   p->face.pts[i].x,
                   p->face.pts[i].y,
                   p->face.pts[i].z);
             }
             printf("  (%g %g %g) %g\n",
                p2->face.plane.normal.x,
                p2->face.plane.normal.y,
                p2->face.plane.normal.z,
                p2->face.plane.dist);
          }*/

    i = BSPFaceSplit(&p->face, &p2->face.plane, &front, &back);
    //      printf("  i=%i\n",i);
    switch (i)
    {
      case 0:
        if (s)
        {
          Q_free(front->pts);
          Q_free(front);
          Q_free(p->face.pts);
          p->face = *back;
          Q_free(back);
        }
        else
        {
          Q_free(back->pts);
          Q_free(back);
          Q_free(p->face.pts);
          p->face = *front;
          Q_free(front);
        }
        break;
      case -1:
        if (!s)
        {
          /*            printf("  (%g %g %g) %g\n",
                         p2->face.plane.normal.x,
                         p2->face.plane.normal.y,
                         p2->face.plane.normal.z,
                         p2->face.plane.dist);*/
          NewMessage("portal clipped away!");
          Q_free(p->face.pts);
          Q_free(p);
          p = NULL;
        }
        break;
      case -2:
        if (s)
        {
          /*            printf("  (%g %g %g) %g\n",
                         p2->face.plane.normal.x,
                         p2->face.plane.normal.y,
                         p2->face.plane.normal.z,
                         p2->face.plane.dist);*/
          NewMessage("portal clipped away!\n");
          Q_free(p->face.pts);
          Q_free(p);
          p = NULL;
        }
        break;
      case -3:
      case -4:
        //         Abort("MakePortals","???");
        Q_free(p->face.pts);
        Q_free(p);
        p = NULL;
        break;
    }
    if (!p)
      break;
    /*      if ((p2->face.plane.dist==192) && (p2->face.plane.normal.x==-1))
          {
             printf(" after clip:\n");
             for (i=0;i<p->face.numpts;i++)
             {
                printf("   %3i: %g %g %g\n",i,
                   p->face.pts[i].x,
                   p->face.pts[i].y,
                   p->face.pts[i].z);
             }
          }*/
  }
  if (p)
    AddPortalToNodes(p, n->FrontNode, n->BackNode);

  // split the node's portals and move them to the children
  for (p2 = n->portals; p2; p2 = next)
  {
    s = (p2->node[1] == n);
    next = p2->next[s];

    other = p2->node[!s];

    RemovePortal(p2, n);
    RemovePortal(p2, other);

    i = BSPFaceSplit(&p2->face, &n->plane, &front, &back);
    switch (i)
    {
      case -1:
        if (s)
          AddPortalToNodes(p2, other, n->BackNode);
        else
          AddPortalToNodes(p2, n->BackNode, other);
        break;
      case -2:
        if (s)
          AddPortalToNodes(p2, other, n->FrontNode);
        else
          AddPortalToNodes(p2, n->FrontNode, other);
        break;
      case -3:
      case -4:
        //         Abort("MakePortals","???");
        Q_free(p2->face.pts);
        Q_free(p2);
        break;
      case 0:
        p = Q_malloc(sizeof(portal_t));
        if (!p)
        {
          bsp_error = ERR_NOMEM;
          return 0;
          //            Abort("MakePortals","Out of memory!");
        }
        memset(p, 0, sizeof(portal_t));

        Q_free(p2->face.pts);
        p2->face = *front;
        Q_free(front);
        p->face = *back;
        Q_free(back);

        if (s)
          AddPortalToNodes(p2, other, n->FrontNode);
        else
          AddPortalToNodes(p2, n->FrontNode, other);

        if (s)
          AddPortalToNodes(p, other, n->BackNode);
        else
          AddPortalToNodes(p, n->BackNode, other);

        break;
    }
  }

  if (!MakePortals_r(n->FrontNode))
    return 0;
  if (!MakePortals_r(n->BackNode))
    return 0;
  return 1;
}

static void
DeleteBSPNode(bsp_node_t* Root)
{
  portal_t *p, *next;
  int s;

  for (p = Root->portals; p; p = next)
  {
    s = (p->node[1] == Root);
    next = p->next[s];

    RemovePortal(p, p->node[0]);
    RemovePortal(p, p->node[1]);
    Q_free(p->face.pts);
    Q_free(p);
  }

  if (Root->FrontNode != NULL)
    DeleteBSPNode(Root->FrontNode);
  if (Root->BackNode != NULL)
    DeleteBSPNode(Root->BackNode);

  Q_free(Root);
}

static void
FillDist_r(bsp_node_t* n, int dist)
{
  int s;
  portal_t* p;

  n->dist = dist;

  for (p = n->portals; p; p = p->next[s])
  {
    s = (p->node[1] == n);

    if ((p->node[0]->contents >= CONTENTS_SOLID) ||
        (p->node[1]->contents >= CONTENTS_SOLID))
      continue;

    if (p->node[!s]->dist)
      continue;

    FillDist_r(p->node[!s], dist + 1);
  }
}

static void
PlaceEntity(entity_t* e)
{
  vec3_t v;
  bsp_node_t* n;
  float t;

  if (!GetKeyValue(e, "origin"))
    return;

  sscanf(GetKeyValue(e, "origin"), "%f %f %f", &v.x, &v.y, &v.z);
  v.z++;

  n = BSPHead;
  while (n->contents == CONTENTS_SPLIT)
  {
    MY_DotProd(v, n->plane.normal, t);
    t -= n->plane.dist;
    /*      printf("(%3g %3g %3g) %4g  %i\n",
             n->plane.normal.x,
             n->plane.normal.y,
             n->plane.normal.z,
             n->plane.dist,t>0);*/
    if (t > 0)
      n = n->FrontNode;
    else
      n = n->BackNode;
  }

  /*   printf("'%s' at '%s' in leaf %i\n",
        GetKeyValue(e,"classname"),
        GetKeyValue(e,"origin"),n->contents);*/

  if (n->contents == CONTENTS_EMPTY)
  {
    n->ent = e;
    FillDist_r(n, 1);
  }
}

void
FindLeak(void)
{
  entity_t* e;

  min[0] = min[1] = min[2] = 10000;
  max[0] = max[1] = max[2] = -10000;

  if (!BuildBSPTree())
  {
    HandleError("FindLeak", "Couldn't build BSP tree!");
    return;
  }

  //   PBT(BSPHead,1);

  min[0] -= 16;
  min[1] -= 16;
  min[2] -= 16;
  max[0] += 16;
  max[1] += 16;
  max[2] += 16;

  /*   printf("min (%g %g %g)  max (%g %g %g)\n",
        min[0],min[1],min[2],
        max[0],max[1],max[2]);*/

  memset(&outside, 0, sizeof(outside));
  outside.contents = CONTENTS_EMPTY;

  if (!MakeHeadPortals())
  {
    DeleteBSPTree();
    HandleError("FindLeak", "Portalizing failed: %s", bsp_err_msg[bsp_error]);
    return;
  }
  if (!MakePortals_r(BSPHead))
  {
    DeleteBSPTree();
    HandleError("FindLeak", "Portalizing failed: %s", bsp_err_msg[bsp_error]);
    return;
  }

  for (e = M.EntityHead; e; e = e->Next)
  {
    if (!(e->Group->flags & 2))
      PlaceEntity(e);
  }

  if (M.npts)
  {
    Q_free(M.pts);
    Q_free(M.tpts);
    M.npts = 0;
    M.pts = NULL;
  }

  if (!outside.dist)
  {
    HandleError("FindLeak", "No leak found!");
  }
  else
  {
    portal_t* p;
    bsp_node_t* n;
    int next;
    int s;
    bsp_node_t* nextn;
    bsp_face_t* nextf;
    vec3_t v;
    int i;

    n = &outside;
    while (n->dist > 1)
    {
      //         printf("node contents=%i dist=%i\n",n->contents,n->dist);

      next = n->dist;
      nextn = NULL;
      nextf = NULL;
      for (p = n->portals; p; p = p->next[s])
      {
        s = (p->node[1] == n);
        if ((p->node[!s]->dist < next) && (p->node[!s]->dist))
        {
          next = p->node[!s]->dist;
          nextn = p->node[!s];
          nextf = &p->face;
        }
      }
      //         printf("best=%i\n",next);
      n = nextn;
      v.x = v.y = v.z = 0;
      for (i = 0; i < nextf->numpts; i++)
      {
        v.x += nextf->pts[i].x;
        v.y += nextf->pts[i].y;
        v.z += nextf->pts[i].z;
        /*            printf("  (%g %g %g)\n",
                       nextf->pts[i].x,
                       nextf->pts[i].y,
                       nextf->pts[i].z);*/
      }
      v.x /= i;
      v.y /= i;
      v.z /= i;
      //         printf("(%g %g %g) %i %p\n",v.x,v.y,v.z,i,nextf);

      M.pts = Q_realloc(M.pts, (M.npts + 1) * sizeof(vec3_t));
      if (!M.pts)
      {
        HandleError("FindLeak", "Out of memory!");
        DeleteBSPTree();
        M.npts = 0;
        status.draw_pts = 0;
        return;
      }
      M.pts[M.npts++] = v;
    }

    sscanf(GetKeyValue(n->ent, "origin"), "%f %f %f", &v.x, &v.y, &v.z);
    M.pts = Q_realloc(M.pts, (M.npts + 1) * sizeof(vec3_t));
    if (!M.pts)
    {
      HandleError("FindLeak", "Out of memory!");
      DeleteBSPTree();
      M.npts = 0;
      status.draw_pts = 0;
      return;
    }
    M.pts[M.npts++] = v;
    M.tpts = Q_malloc(sizeof(vec3_t) * M.npts);
    if (!M.tpts)
    {
      HandleError("FindLeak", "Out of memory!");
      DeleteBSPTree();
      Q_free(M.pts);
      M.npts = 0;
      status.draw_pts = 0;
      return;
    }

    status.draw_pts = 1;

    //      printf("n->dist=%i n->ent=%p\n",n->dist,n->ent);
  }

  DeleteBSPTree();
}

static int
Trace_r(bsp_node_t* n, vec3_t v1, vec3_t v2)
{
  vec3_t v;
  float t1, t2;

start:
  if (n->contents != CONTENTS_SPLIT)
  {
    if (n->contents == CONTENTS_EMPTY)
      return 1;
    else
      return 0;
  }

  t1 = DotProd(v1, n->plane.normal) - n->plane.dist;
  t2 = DotProd(v2, n->plane.normal) - n->plane.dist;

  if ((t1 > -0.01) && (t2 > -0.01))
  {
    n = n->FrontNode;
    goto start;
  }
  if ((t1 < 0.01) && (t2 < 0.01))
  {
    n = n->BackNode;
    goto start;
  }

  t1 = t1 / (t1 - t2);

  v.x = v1.x + (v2.x - v1.x) * t1;
  v.y = v1.y + (v2.y - v1.y) * t1;
  v.z = v1.z + (v2.z - v1.z) * t1;

  if (t2 > 0)
  {
    if (!Trace_r(n->FrontNode, v2, v))
      return 0;

    n = n->BackNode;
    v2 = v;
    goto start;
  }
  else
  {
    if (!Trace_r(n->FrontNode, v1, v))
      return 0;

    n = n->BackNode;
    v1 = v2;
    v2 = v;
    goto start;
  }
}

int
Trace(vec3_t from, vec3_t to)
{
  return Trace_r(BSPHead, from, to);
}

int
TraceBSPInit(void)
{
  if (!BuildBSPTree())
    return 0;
  return 1;
}

void
TraceBSPDone(void)
{
  DeleteBSPTree();
}
