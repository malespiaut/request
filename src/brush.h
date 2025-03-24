#ifndef BRUSH_H
#define BRUSH_H

typedef struct
{
  int type;
/* Brushes defining map geometry */
#define BR_NORMAL 1   /* a normal brush */
#define BR_SUBTRACT 2 /* a subtractive brush */
#define BR_POLYGON 3  /* a lone polygon */

#define BR_Q3_CURVE 4 /* a Quake 3 curve, array of bezier patches */

/* Brushes for internal use */
#define BR_CLIP 5 /* a clipping plane */

  const char* name;

  /* Flags for this type of brush */
  int flags;

/* The brush shouldn't be saved */
#define BR_F_NOSAVE 0x01

/* The brush is 'solid', it can be used in CSG operations */
#define BR_F_CSG 0x02

/* num_edges and edges are valid, the brush is drawn by drawing the
   edges */
#define BR_F_EDGES 0x10

/* The brush has its own texdef_t that should be set. If this isn't set,
   each plane has its own texdef_t */
#define BR_F_BTEXDEF 0x20

  int (*b_new)(struct brush_s* b);
  int (*b_copy)(struct brush_s* dst, const struct brush_s* src);
  void (*b_free)(struct brush_s* b);
} brush_type_t;

typedef struct plane_s
{
  float dist;    /* distance to the plane */
  vec3_t normal; /* Normal to plane */
  vec3_t center;
  vec3_t tcenter;
  svec_t scenter;
  int* verts; /* indices in the brush->verts array */
  int num_verts;

  texdef_t tex;
} plane_t;

/* Extra information for BR_Q3_CURVE */
typedef struct
{
  /* Size of bezier patch vertex array, number of patches is (dim-1)/2 */
  int sizex, sizey;
  /* Texture alignment information, the s and t coordinates at vertex x,y
     are in s[x+y*sizex] and t[x+y*sizey] , i.e. the alignment information
     for vertex verts[i] is in s[i] and t[i] */
  float *s, *t;
} brush_q3_curve_t;

typedef struct brush_s
{
  plane_t* plane;
  int num_planes;

  vec3_t* verts; /* Array of vertices */
  vec3_t* tverts;
  svec_t* sverts;
  int num_verts;

  edge_t* edges; /* Array of edges */
  int num_edges; /* (in the local verts array) */

  vec3_t center;
  vec3_t tcenter;
  svec_t scenter;

  texdef_t tex;

  /* Extra stuff used by some types of brushes */
  union
  {
    brush_q3_curve_t q3c;
  } x;

  /* used by 3d.c */
  int drawn;
  int hidden;

  brush_type_t* bt;

  struct entity_s* EntityRef;
  struct group_s* Group;

  unsigned int uid; /* Identification number, MUST be unique */

  struct brush_s* Next;
  struct brush_s* Last;
} brush_t;

brush_t* B_New(int type);
void B_Free(brush_t* b);

/* free only the contents of b, not b itself */
void B_FreeC(brush_t* b);

void B_Link(brush_t* b);
void B_Unlink(brush_t* b);

void B_ChangeType(brush_t* b, int ntype);

/*
Duplicates:
  num_planes
  plane
  plane.verts
  plane.num_verts
  plane.tex
  plane.normal

  num_verts
  verts

  num_edges
  edges

  tex

  EntityRef
  Group

  bt

If full==1, sets uid and allocates memory for:
  tverts
  sverts

*/
brush_t* B_Duplicate(brush_t* dest, brush_t* source, int full);

#endif
