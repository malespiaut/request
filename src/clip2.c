#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "clip2.h"

#include "3d.h"
#include "brush.h"
#include "check.h"
#include "clip.h"
#include "display.h"
#include "edbrush.h"
#include "edvert.h"
#include "error.h"
#include "geom.h"
#include "keyboard.h"
#include "keyhit.h"
#include "map.h"
#include "memory.h"
#include "message.h"
#include "mouse.h"
#include "newgroup.h"
#include "quest.h"
#include "status.h"
#include "undo.h"

int
AddBrushes(brush_t* b1, brush_t* b2)
{
  int i, j;
  brush_my_t A;
  brush_my_t B;
  brush_my_t C;
  brush_t* b;
  entity_t* EntityRef;

  if (!(b1->bt->flags & b2->bt->flags & BR_F_CSG))
  {
    HandleError("AddBrushes", "Can only use normal brushes for CSG operations");
    return 0;
  }

  EntityRef = b1->EntityRef;

  ClearSelVerts();

  A.numfaces = b1->num_planes;
  A.faces = (face_my_t*)Q_malloc(sizeof(face_my_t) * (A.numfaces));

  B.numfaces = b2->num_planes;
  B.faces = (face_my_t*)Q_malloc(sizeof(face_my_t) * (B.numfaces));

  C.numfaces = b1->num_planes + b2->num_planes;
  C.faces = (face_my_t*)Q_malloc(sizeof(face_my_t) * (C.numfaces));

  if (!A.faces || !B.faces || !C.faces)
  {
    HandleError("AddBrushes", "Out of memory!");
    return FALSE;
  }

  for (i = 0; i < b1->num_planes; i++)
  {
    A.faces[i].numpts = b1->plane[i].num_verts;
    for (j = 0; j < b1->plane[i].num_verts; j++)
    {
      A.faces[i].pts[j] = b1->verts[b1->plane[i].verts[j]];
    }

    A.faces[i].tex = b1->plane[i].tex;

    A.faces[i].misc = 0;
    A.faces[i].normal = b1->plane[i].normal;
    A.faces[i].dist = DotProd(A.faces[i].normal, b1->verts[b1->plane[i].verts[0]]);

    C.faces[i].tex = b1->plane[i].tex;

    C.faces[i].misc = 0;
    C.faces[i].normal = b1->plane[i].normal;
    C.faces[i].dist = DotProd(C.faces[i].normal, b1->verts[b1->plane[i].verts[0]]);
  }

  for (i = 0; i < b2->num_planes; i++)
  {
    B.faces[i].numpts = b2->plane[i].num_verts;
    for (j = 0; j < b2->plane[i].num_verts; j++)
    {
      B.faces[i].pts[j] = b2->verts[b2->plane[i].verts[j]];
    }

    B.faces[i].tex = b2->plane[i].tex;

    B.faces[i].misc = 0;
    B.faces[i].normal = b2->plane[i].normal;
    B.faces[i].dist = DotProd(B.faces[i].normal, b2->verts[b2->plane[i].verts[0]]);

    C.faces[i + b1->num_planes].tex = b2->plane[i].tex;

    C.faces[i + b1->num_planes].misc = 0;
    C.faces[i + b1->num_planes].normal = b2->plane[i].normal;
    C.faces[i + b1->num_planes].dist = DotProd(C.faces[i + b1->num_planes].normal, b2->verts[b2->plane[i].verts[0]]);
  }

  if (!BrushesIntersect(&A, &B))
  {
    Q_free(A.faces);
    Q_free(B.faces);
    Q_free(C.faces);
    HandleError("Brush Intersection", "Brushes do not intersect!");
    return FALSE;
  }

  Q_free(A.faces);
  Q_free(B.faces);

  if (!CreateBrushFaces(&C))
  {
    return FALSE;
  }

  ClearSelBrushes();

  b = MyBrushIntoMap(&C, EntityRef, b1->Group);
  if (b)
  {
    SUndo(UNDO_NONE, UNDO_NONE);
    AddDBrush(b);

    AddSelBrush(b, 0);
  }
  UpdateAllViewports();
  return TRUE;
}

static void
CSG_MakeHollow(brush_t* b)
{
  plane_t* p;
  int i, j;
  brush_t* temp;
  brush_my_t t;

  for (i = 0; i < b->num_planes; i++)
  {
    t.numfaces = b->num_planes + 1;
    t.faces = Q_malloc(sizeof(face_my_t) * t.numfaces);
    if (!t.faces)
    {
      HandleError("CSG_MakeHollow", "Out of memory!");
      return;
    }
    for (j = 0; j < b->num_planes; j++)
    {
      t.faces[j].misc = 0;
      t.faces[j].numpts = 0;
      t.faces[j].normal = b->plane[j].normal;
      t.faces[j].dist = b->plane[j].dist;
      t.faces[j].tex = b->plane[j].tex;
    }
    p = &b->plane[i];

    // new plane
    t.faces[j].misc = 0;
    t.faces[j].numpts = 0;

    t.faces[j].normal.x = -p->normal.x;
    t.faces[j].normal.y = -p->normal.y;
    t.faces[j].normal.z = -p->normal.z;
    t.faces[j].dist = -p->dist + status.snap_size;
    t.faces[j].tex = p->tex;

    temp = B_New(BR_NORMAL);
    if (!temp)
    {
      Q_free(t.faces);
      HandleError("CSG_MakeHollow", "Out of memory!");
      return;
    }
    if (!BuildBrush(&t, temp))
    {
      HandleError("CSG_MakeHollow", "Error building brush!");
      return;
    }

    AddDBrush(temp);

    temp->EntityRef = M.WorldSpawn;
    temp->Group = FindVisGroup(M.WorldGroup);
    B_Link(temp);
  }
}

void
MakeHollow(void)
{
  brushref_t* b;
  if (!M.display.num_bselected)
  {
    HandleError("MakeHollow", "No brushes selected!");
    return;
  }
  ClearSelVerts();

  SUndo(UNDO_NONE, UNDO_DELETE);

  for (b = M.display.bsel; b; b = b->Next)
  {
    if (!(b->Brush->bt->flags & BR_F_CSG))
    {
      HandleError("MakeHollow", "Can only use normal brushes");
    }
    else if (!CheckBrush(b->Brush, FALSE))
    {
      HandleError("MakeHollow", "Brushes have errors!");
    }
    else
    {
      CSG_MakeHollow(b->Brush);
      DeleteABrush(b->Brush);
    }
  }

  ClearSelBrushes();
}

static void
CSG_MakeRoom(brush_t* b)
{
  plane_t* p;
  int i, j;
  brush_t* temp;
  brush_my_t t;

  for (i = 0; i < b->num_planes; i++)
  {
    t.numfaces = b->num_planes * 2;
    t.faces = Q_malloc(sizeof(face_my_t) * t.numfaces);
    if (!t.faces)
    {
      HandleError("CSG_MakeRoom", "Out of memory!");
      return;
    }
    for (j = 0; j < b->num_planes; j++)
    {
      t.faces[j].misc = 0;
      t.faces[j].numpts = 0;
      t.faces[j].normal = b->plane[j].normal;
      t.faces[j].dist = b->plane[j].dist;
      t.faces[j].tex = b->plane[j].tex;
    }
    // add new planes
    for (; j < t.numfaces; j++)
    {
      p = &b->plane[j - b->num_planes];

      t.faces[j].misc = 0;
      t.faces[j].numpts = 0;

      t.faces[j].normal.x = p->normal.x;
      t.faces[j].normal.y = p->normal.y;
      t.faces[j].normal.z = p->normal.z;
      t.faces[j].dist = p->dist - status.snap_size;
      if (j - b->num_planes == i)
      {
        t.faces[j].normal.x = -t.faces[j].normal.x;
        t.faces[j].normal.y = -t.faces[j].normal.y;
        t.faces[j].normal.z = -t.faces[j].normal.z;
        t.faces[j].dist = -t.faces[j].dist;
      }
      t.faces[j].tex = p->tex;
    }

    // create final brush
    temp = B_New(BR_NORMAL);
    if (!temp)
    {
      Q_free(t.faces);
      HandleError("CSG_MakeRoom", "Out of memory!");
      return;
    }
    if (!BuildBrush(&t, temp))
    {
      HandleError("CSG_MakeRoom", "Error building brush!");
      return;
    }

    AddDBrush(temp);
    temp->EntityRef = M.WorldSpawn;
    temp->Group = FindVisGroup(M.WorldGroup);
    B_Link(temp);
  }
}

void
MakeRoom(void)
{
  brushref_t* b;
  if (!M.display.num_bselected)
  {
    HandleError("MakeRoom", "No brushes selected!");
    return;
  }
  ClearSelVerts();

  SUndo(UNDO_NONE, UNDO_DELETE);

  for (b = M.display.bsel; b; b = b->Next)
  {
    if (!(b->Brush->bt->flags & BR_F_CSG))
    {
      HandleError("MakeRoom", "Can only use normal brushes");
    }
    else if (!CheckBrush(b->Brush, FALSE))
    {
      HandleError("MakeRoom", "Brushes have errors!");
    }
    else
    {
      CSG_MakeRoom(b->Brush);
      DeleteABrush(b->Brush);
    }
  }

  ClearSelBrushes();
}

static int
ClipBrush(brush_t* b, plane_t* p, int flip)
{
  int j;
  brush_t* temp;
  brush_my_t t;

  t.numfaces = b->num_planes + 1;
  t.faces = Q_malloc(sizeof(face_my_t) * t.numfaces);
  if (!t.faces)
  {
    HandleError("ClipBrush", "Out of memory!");
    return 0;
  }
  for (j = 0; j < b->num_planes; j++)
  {
    t.faces[j].misc = 0;
    t.faces[j].numpts = 0;
    t.faces[j].normal = b->plane[j].normal;
    t.faces[j].dist = b->plane[j].dist;
    t.faces[j].tex = b->plane[j].tex;
  }
  // add new plane
  t.faces[j].misc = 0;
  t.faces[j].numpts = 0;

  if (flip)
  {
    t.faces[j].normal.x = -p->normal.x;
    t.faces[j].normal.y = -p->normal.y;
    t.faces[j].normal.z = -p->normal.z;
    t.faces[j].dist = -p->dist;
  }
  else
  {
    t.faces[j].normal.x = p->normal.x;
    t.faces[j].normal.y = p->normal.y;
    t.faces[j].normal.z = p->normal.z;
    t.faces[j].dist = p->dist;
  }
  t.faces[j].tex = b->plane[0].tex;

  // create final brush
  temp = B_New(BR_NORMAL);
  if (!temp)
  {
    Q_free(t.faces);
    HandleError("ClipBrush", "Out of memory!");
    return 0;
  }

  if (!BuildBrush(&t, temp))
  {
    //      HandleError("ClipBrush","Error building brush!");
    return 0;
  }

  temp->EntityRef = M.WorldSpawn;
  temp->Group = FindVisGroup(M.WorldGroup);
  B_Link(temp);

  AddDBrush(temp);

  return 1;
}

void
PlaneSplit(void)
{
  plane_t* p;
  brushref_t* br1;
  brushref_t* br;
  int split, error;

  br1 = M.display.bsel;

  M.display.bsel = NULL;
  M.display.num_bselected = 0;
  UpdateAllViewports();
  NewMessage("Select the clipping plane. Right click when finished.");
  do
  {
    CheckMoveKeys();
    UpdateMap();
    UpdateMouse();
    //		ClearKeys();
  } while (mouse.button != 2);

  while (mouse.button == 2)
    UpdateMouse();

  if (M.display.num_bselected != 1)
  {
    NewMessage("Exactly one clipping plane must be selected!");
    M.display.bsel = br1;
    ClearSelBrushes();
    return;
  }
  if (M.display.bsel->Brush->bt->type != BR_CLIP)
    NewMessage("Warning! Brush isn't a clipping plane!");

  p = &M.display.bsel->Brush->plane[0];
  ClearSelBrushes();

  if ((fabs(p->normal.x) < 0.01) &&
      (fabs(p->normal.y) < 0.01) &&
      (fabs(p->normal.z) < 0.01))
  {
    HandleError("PlaneSplit", "Invalid clipping plane!");
    M.display.bsel = br1;
    ClearSelBrushes();
    return;
  }

  M.display.bsel = br1;
  SUndo(UNDO_NONE, UNDO_DELETE);

  split = error = 0;
  for (br = br1; br; br = br->Next)
  {
    if (!br->Brush->bt->flags & BR_F_CSG)
    {
      HandleError("PlaneSplit", "Can only split normal brushes");
      continue;
    }

    if (!ClipBrush(br->Brush, p, 0))
      error++;
    if (!ClipBrush(br->Brush, p, 1))
      error++;
    DeleteABrush(br->Brush);
    split += 2;
  }

  NewMessage("%i brushes split, %i not split.", split - error, error);

  M.display.bsel = br1;
  ClearSelBrushes();
}

//----------------------------------------------
// This function takes two brushes and attempts to create a single
// convex brush that is the union of the two.
//----------------------------------------------
// originally by Gyro Gearloose
int
JoinBrushes(brush_t* b1, brush_t* b2)
{
  int i, j, k, l;
  brush_my_t A;
  brush_my_t B;
  brush_my_t C;
  entity_t* EntityRef;
  face_my_t Face1, Face2;
  int totalback;
  brush_t* b;

  if (!(b1->bt->flags & b2->bt->flags & BR_F_CSG))
  {
    HandleError("JoinBrushes", "Can only join normal brushes");
    return 0;
  }

  EntityRef = b1->EntityRef;

  ClearSelVerts();

  A.numfaces = b1->num_planes;
  A.faces = (face_my_t*)Q_malloc(sizeof(face_my_t) * (A.numfaces));

  B.numfaces = b2->num_planes;
  B.faces = (face_my_t*)Q_malloc(sizeof(face_my_t) * (B.numfaces));

  C.numfaces = 0;
  C.faces = NULL;

  for (i = 0; i < b1->num_planes; i++)
  {
    A.faces[i].numpts = b1->plane[i].num_verts;
    for (j = 0; j < b1->plane[i].num_verts; j++)
    {
      A.faces[i].pts[j] = b1->verts[b1->plane[i].verts[j]];
    }
    A.faces[i].tex = b1->plane[i].tex;
    A.faces[i].misc = 0;
    A.faces[i].normal = b1->plane[i].normal;
    A.faces[i].dist = DotProd(A.faces[i].normal, b1->verts[b1->plane[i].verts[0]]);
  }
  for (i = 0; i < b2->num_planes; i++)
  {
    B.faces[i].numpts = b2->plane[i].num_verts;
    for (j = 0; j < b2->plane[i].num_verts; j++)
    {
      B.faces[i].pts[j] = b2->verts[b2->plane[i].verts[j]];
    }
    B.faces[i].tex = b2->plane[i].tex;
    B.faces[i].misc = 0;
    B.faces[i].normal = b2->plane[i].normal;
    B.faces[i].dist = DotProd(B.faces[i].normal, b2->verts[b2->plane[i].verts[0]]);
  }

  // For each face of b1 and b2, we must ask whether or not to make it
  // a face of C.  One way that might work could be to treat each face
  // of a brush as a plane and attempt to split the faces of the other
  // brush with it.  Only if all the other faces are coplanar or entirely
  // behind the plane should we add it to C.  Coplanars of one should be
  // kept, and coplanars of the other should be tossed.

  for (i = 0; i < A.numfaces; i++)
  {
    totalback = 0;
    for (j = 0; j < B.numfaces; j++)
    {
      k = DivideFaceByPlane(B.faces[j], A.faces[i], &Face1, &Face2, FALSE);
      // Count face if it's coplanar or behind this plane of b1
      if ((k != -1) && (k != -4))
        break;
    }
    if (j == B.numfaces)
    {
      // Add this plane of A to C
      l = C.numfaces;
      C.numfaces++;
      C.faces = (face_my_t*)Q_realloc(C.faces, sizeof(face_my_t) * C.numfaces);

      C.faces[l].tex = b1->plane[i].tex;
      C.faces[l].misc = 0;
      C.faces[l].normal = b1->plane[i].normal;
      C.faces[l].dist = DotProd(C.faces[l].normal, b1->verts[b1->plane[i].verts[0]]);
    }
  }

  for (i = 0; i < B.numfaces; i++)
  {
    totalback = 0;
    for (j = 0; j < A.numfaces; j++)
    {
      k = DivideFaceByPlane(A.faces[j], B.faces[i], &Face1, &Face2, FALSE);
      // Count face if it's coplanar or behind this plane of b2
      if ((k != -1) && (k = -4))
        break;
    }
    if (j == A.numfaces)
    {
      // Add this plane of B to C
      l = C.numfaces;
      C.numfaces++;
      C.faces = (face_my_t*)Q_realloc(C.faces, sizeof(face_my_t) * C.numfaces);

      C.faces[l].tex = b2->plane[i].tex;
      C.faces[l].misc = 0;
      C.faces[l].normal = b2->plane[i].normal;
      C.faces[l].dist = DotProd(C.faces[l].normal, b2->verts[b2->plane[i].verts[0]]);
    }
  }

  if (!CreateBrushFaces(&C))
  {
    return FALSE;
  }

  ClearSelBrushes();
  b = MyBrushIntoMap(&C, EntityRef, b1->Group);

  SUndo(UNDO_NONE, UNDO_NONE);
  AddDBrush(b);

  AddSelBrush(b, 0);

  UpdateAllViewports();
  return TRUE;
}
