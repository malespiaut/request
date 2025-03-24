/*
undo.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "undo.h"

#include "3d.h"
#include "brush.h"
#include "bsp.h"
#include "display.h"
#include "edbrush.h"
#include "edent.h"
#include "edface.h"
#include "edvert.h"
#include "entity.h"
#include "error.h"
#include "map.h"
#include "memory.h"
#include "message.h"
#include "quest.h"
#include "status.h"
#include "texlock.h"

typedef struct
{
  entity_t orig; /* copy of the original entity */
  entity_t* mod; // pointer to the modified one
} uent_t;

typedef struct
{
  brush_t orig;
  unsigned int m_uid; /* uid of original brush */
} ubrush_t;

/*
   The fields used in uent_t.orig are:
      numkeys
      key
      value
      Group

   The fields used in ubrush_t.orig are:
      num_planes
      plane
      num_verts
      verts
      num_edges
      edges
      EntityRef
      Group
      bt
      uid

   The fields used in the ubrush_t.orig.plane are:
      num_verts
      verts
      tex

   Lots of wasted space. Maybe I should fix this, but it isn't that bad.
*/

typedef struct undo_info_s
{
  int etype;
  int btype;

  uent_t* ents;
  int nents;

  entity_t** dents;
  int ndents;

  ubrush_t* brush;
  int nbrush;

  unsigned int* dbrush;
  int ndbrush;
} undo_info_t;

// Frees the all fields and u itself.
static void
FreeUndoInfo(undo_info_t* u)
{
  int i, j;
  entity_t* e;
  brush_t* b;

  for (i = 0; i < u->nents; i++)
  {
    e = &u->ents[i].orig;
    for (j = 0; j < e->numkeys; j++)
    {
      Q_free(e->key[j]);
      Q_free(e->value[j]);
    }
    Q_free(e->key);
    Q_free(e->value);
  }
  if (u->nents)
    Q_free(u->ents);

  for (i = 0; i < u->nbrush; i++)
  {
    b = &u->brush[i].orig;
    B_FreeC(b);
  }
  if (u->nbrush)
    Q_free(u->brush);

  if (u->ndents)
    Q_free(u->dents);

  if (u->ndbrush)
    Q_free(u->dbrush);

  Q_free(u);
}

// Get a new undo_info_t to place undo info in. Place it in undo[0].
static void
NewUndoInfo(void)
{
  int i;

  if (M.n_undo == MAX_UNDO)
  {
    FreeUndoInfo(M.undo[MAX_UNDO - 1]);
    M.n_undo--;
  }

  for (i = M.n_undo - 1; i >= 0; i--)
  {
    M.undo[i + 1] = M.undo[i];
  }

  M.undo[0] = Q_malloc(sizeof(undo_info_t));
  if (!M.undo[0])
    Abort("NewUndoInfo", "Out of memory!");
  memset(M.undo[0], 0, sizeof(undo_info_t));
  M.n_undo++;
}

static void
FinishUndoInfo(void)
{
  undo_info_t* u;
  int i, j;

  u = M.undo[0];

  for (i = 0; i < u->nbrush; i++)
  {
    for (j = 0; j < u->nents; j++)
    {
      if (u->brush[i].orig.EntityRef == u->ents[j].mod)
      {
        u->brush[i].orig.EntityRef = &u->ents[j].orig;
        break;
      }
    }
  }
}

static int
SizeUndoInfo(undo_info_t* u)
{
  int size;
  int i, j;
  brush_t* b;

  size = sizeof(undo_info_t);

  for (i = 0; i < u->nents; i++)
  {
    size += sizeof(uent_t);

    for (j = 0; j < u->ents[i].orig.numkeys; j++)
    {
      size += strlen(u->ents[i].orig.key[j]) + 1;
      size += strlen(u->ents[i].orig.value[j]) + 1;
    }
    size += u->ents[i].orig.numkeys * 2 * sizeof(char*);
  }

  for (i = 0; i < u->nbrush; i++)
  {
    size += sizeof(ubrush_t);

    b = &u->brush[i].orig;
    for (j = 0; j < b->num_planes; j++)
    {
      size += sizeof(plane_t);
      size += sizeof(int) * b->plane[j].num_verts;
    }
    size += sizeof(vec3_t) * b->num_verts;
    size += sizeof(edge_t) * b->num_edges;
  }

  size += sizeof(entity_t*) * u->ndents;
  size += sizeof(unsigned int) * u->ndbrush;

  /*   NewMessage("%5i bytes in %i entities and %i brushes.",
        size,u->nents,u->nbrush);*/

  return size;
}

static void
AddUEnt(entity_t* e)
{
  undo_info_t* u;
  entity_t* e2;
  int i;

  u = M.undo[0];

  for (i = 0; i < u->nents; i++)
  {
    if (u->ents[i].mod == e)
      return;
  }

  u->ents = Q_realloc(u->ents, sizeof(uent_t) * (u->nents + 1));
  if (!u->ents)
    Abort("AddUEnt", "Out of memory!");
  memset(&u->ents[u->nents], 0, sizeof(uent_t));

  u->ents[u->nents].mod = e;
  e2 = &u->ents[u->nents].orig;

  e2->numkeys = e->numkeys;
  e2->Group = e->Group;
  e2->key = Q_malloc(sizeof(char*) * e2->numkeys);
  e2->value = Q_malloc(sizeof(char*) * e2->numkeys);

  for (i = 0; i < e2->numkeys; i++)
  {
    e2->key[i] = Q_strdup(e->key[i]);
    e2->value[i] = Q_strdup(e->value[i]);
  }

  u->nents++;
}

static void
AddUBrush(brush_t* b)
{
  undo_info_t* u;
  brush_t* b2;
  int i;

  u = M.undo[0];

  for (i = 0; i < u->nbrush; i++)
  {
    if (u->brush[i].m_uid == b->uid)
      return;
  }

  u->brush = Q_realloc(u->brush, sizeof(ubrush_t) * (u->nbrush + 1));
  if (!u->brush)
    Abort("AddUBrush", "Out of memory!");

  u->brush[u->nbrush].m_uid = b->uid;
  b2 = &u->brush[u->nbrush].orig;

  if (!B_Duplicate(b2, b, 0))
  {
    Abort("AddUBrush", "Out of memory!");
  }

  b2->uid = b->uid;

  u->nbrush++;
}

void
SUndo(int etype, int btype)
{
  entityref_t* er;
  brushref_t* br;
  brush_t* b;
  fsel_t* fs;

  NewUndoInfo();

  M.undo[0]->etype = etype;
  M.undo[0]->btype = btype;
  M.modified = 1;

  if (etype != UNDO_NONE)
  {
    for (er = M.display.esel; er; er = er->Next)
    {
      AddUEnt(er->Entity);

      if (btype != UNDO_NONE)
      {
        for (b = M.BrushHead; b; b = b->Next)
        {
          if (b->Group)
            if (b->Group->flags & 0x02)
              continue;

          if (b->EntityRef == er->Entity)
            AddUBrush(b);
        }
      }
    }

    if (etype == UNDO_DELETE)
    {
      int i, j, k;
      undo_info_t *u, *u0;

      u0 = M.undo[0];
      for (i = 1; i < M.n_undo; i++)
      {
        u = M.undo[i];
        for (k = 0; k < u0->nents; k++)
        {
          for (j = 0; j < u->nents; j++)
          {
            if (u->ents[j].mod == u0->ents[k].mod)
              u->ents[j].mod = &u0->ents[k].orig;
          }

          for (j = 0; j < u->ndents; j++)
          {
            if (u->dents[j] == u0->ents[k].mod)
              u->dents[j] = &u0->ents[k].orig;
          }
        }
      }
    }
  }

  if (btype != UNDO_NONE)
  {
    for (br = M.display.bsel; br; br = br->Next)
    {
      AddUBrush(br->Brush);
    }

    for (fs = M.display.fsel; fs; fs = fs->Next)
    {
      AddUBrush(fs->Brush);
    }
  }

  FinishUndoInfo();
  //   SizeUndoInfo(M.undo[0]);
}

void
AddDBrush(brush_t* b)
{
  undo_info_t* u;

  u = M.undo[0];

  u->dbrush = Q_realloc(u->dbrush, sizeof(unsigned int) * (u->ndbrush + 1));
  u->dbrush[u->ndbrush] = b->uid;
  u->ndbrush++;
}

void
AddDEntity(entity_t* e)
{
  undo_info_t* u;

  u = M.undo[0];

  u->dents = Q_realloc(u->dents, sizeof(entity_t*) * (u->ndents + 1));
  u->dents[u->ndents] = e;
  u->ndents++;
}

void
Undo(void)
{
  undo_info_t* u;
  int i, j;
  entity_t* e;
  entity_t* olde;

  brush_t* b;
  brush_t* oldb;

  int type;

  if (!M.n_undo)
  {
    HandleError("Undo", "Nothing to undo!");
    return;
  }
  u = M.undo[0];

  if (u->etype != UNDO_NONE)
  {
    type = u->etype;
    for (i = 0; i < u->nents; i++)
    {
      olde = &u->ents[i].orig;
      switch (type)
      {
        case UNDO_CHANGE: // find the modified entity and reuse it
          for (e = M.EntityHead; e; e = e->Next)
          {
            if (e == u->ents[i].mod)
              break;
          }
          if (!e)
          {
            HandleError("Undo", "Stale undo info! How did you manage this?");
            goto e_cont;
          }

          for (j = 0; j < e->numkeys; j++)
          {
            Q_free(e->key[j]);
            Q_free(e->value[j]);
          }
          Q_free(e->key);
          Q_free(e->value);
          e->numkeys = 0;
          e->key = e->value = NULL;

          break;

        case UNDO_DELETE: // just create a new entity
          e = Q_malloc(sizeof(entity_t));
          if (!e)
            Abort("Undo", "Out of memory!");
          memset(e, 0, sizeof(entity_t));

          InitEntity(e);
          if (M.EntityHead != NULL)
            M.EntityHead->Last = e;
          e->Next = M.EntityHead;
          e->Last = NULL;

          M.EntityHead = e;
          M.num_entities++;

          break;

        default:
          Abort("Undo", "Internal error!");
          break;
      }

      e->Group = olde->Group;

      for (j = 0; j < olde->numkeys; j++)
        SetKeyValue(e, olde->key[j], olde->value[j]);

      u->ents[i].mod = e;

e_cont:
    }

    if (type == UNDO_DELETE)
    {
      int i, j, k;
      undo_info_t* un;

      for (i = 1; i < M.n_undo; i++)
      {
        un = M.undo[i];

        for (k = 0; k < u->nents; k++)
        {
          for (j = 0; j < un->nents; j++)
          {
            if (un->ents[j].mod == &u->ents[k].orig)
              un->ents[j].mod = u->ents[k].mod;
          }

          for (j = 0; j < un->ndents; j++)
          {
            if (un->dents[j] == &u->ents[k].orig)
              un->dents[j] = u->ents[k].mod;
          }
        }
      }
    }
  }

  if (u->btype != UNDO_NONE)
  {
    type = u->btype;
    for (i = 0; i < u->nbrush; i++)
    {
      oldb = &u->brush[i].orig;
      switch (type)
      {
        case UNDO_CHANGE: /* find the modified brush and delete it */
          for (b = M.BrushHead; b; b = b->Next)
          {
            if (b->uid == u->brush[i].m_uid)
              break;
          }
          if (!b)
          {
            HandleError("Undo", "Stale undo info! Can't find modified brush! How did you manage this?");
          }
          else
          {
            B_Unlink(b);
            B_Free(b);
          }
          break;

        case UNDO_DELETE:
          break;

        default:
          Abort("Undo", "Internal error! Unknown undo type %i!", type);
          break;
      }

      b = B_Duplicate(NULL, oldb, 1);
      b->uid = oldb->uid;

      for (j = 0; j < u->nents; j++)
      {
        if (b->EntityRef == &u->ents[j].orig)
        {
          b->EntityRef = u->ents[j].mod;
          break;
        }
      }

      B_Link(b);
    }
  }

  for (i = 0; i < u->ndents; i++)
  {
    for (e = M.EntityHead; e; e = e->Next)
    {
      if (e == u->dents[i])
        break;
    }
    if (!e)
    {
      HandleError("Undo", "Stale undo info! How did you manage this?");
    }
    else
    {
      DeleteEntity(e);
    }
  }

  for (i = 0; i < u->ndbrush; i++)
  {
    for (b = M.BrushHead; b; b = b->Next)
    {
      if (b->uid == u->dbrush[i])
        break;
    }
    if (!b)
    {
      HandleError("Undo", "Stale undo info! How did you manage this?");
    }
    else
    {
      DeleteABrush(b);
    }
  }

  /*   NewMessage("Undo: Restored %i/%i brushes and %i/%i entities",
        u->nbrush,u->ndbrush,u->nents,u->ndents);*/

  FreeUndoInfo(u);

  for (i = 0; i < M.n_undo - 1; i++)
    M.undo[i] = M.undo[i + 1];

  M.n_undo--;

  ClearSelVerts();
  ClearSelFaces();
  ClearSelBrushes();
  ClearSelEnts();
}

void
ClearUndo(void)
{
  int i;

  for (i = 0; i < M.n_undo; i++)
  {
    FreeUndoInfo(M.undo[i]);
  }
  M.n_undo = 0;
}

int
MapSize(void)
{
  brush_t* b;
  entity_t* e;
  plane_t* p;
  int i;
  int size;

  size = 0;

  for (b = M.BrushHead; b; b = b->Next)
  {
    size += sizeof(*b);
    size += sizeof(vec3_t) * b->num_verts * 2;
    size += sizeof(svec_t) * b->num_verts;
    size += sizeof(edge_t) * b->num_edges;
    size += sizeof(plane_t) * b->num_planes;

    for (i = 0, p = b->plane; i < b->num_planes; i++, p++)
    {
      size += sizeof(int) * p->num_verts;
    }
  }

  for (e = M.EntityHead; e; e = e->Next)
  {
    size += sizeof(*e);
    size += sizeof(char**) * 2 * e->numkeys;

    for (i = 0; i < e->numkeys; i++)
    {
      size += strlen(e->key[i]) + 1;
      size += strlen(e->value[i]) + 1;
    }
  }

  return size;
}

int
UndoSize(void)
{
  int total = 0;
  int i;

  for (i = 0; i < M.n_undo; i++)
    total += SizeUndoInfo(M.undo[i]);

  return total;
}

void
UndoDone(void)
{
  undo_info_t* u;
  int i;
  brush_t* b;

  if (M.n_undo < 1)
    return;
  u = M.undo[0];

  if (status.texlock) // Do the texture locking.
  {
    for (i = 0; i < u->nbrush; i++)
    {
      RecalcNormals(&u->brush[i].orig);

      for (b = M.BrushHead; b; b = b->Next)
        if (b->uid == u->brush[i].m_uid)
          break;

      if (b)
        TexLock(b, &u->brush[i].orig);
    }
  }
  /* Currently disabled! Very dangerous! Eats huge amount of memory and
     reduces performance a lot.

     if (M.BSPHead) // Update the BSP tree.
     {
        for (i=0;i<u->nbrush;i++)
        {
           BSPDeleteBrush(u->brush[i].mod);
           BSPAddBrush(u->brush[i].mod);
        }
     }*/
}
