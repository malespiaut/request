/*
quest.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef QUEST_H
#define QUEST_H

// number of maps loaded at the same time
#define MAX_MAPS 5

#define MAX_UNDO 5

typedef struct
{
	struct brush_s *brushes;
	vec3_t          b_base;

	entity_t       *entities;
   struct brush_s *ent_br;
	vec3_t          e_base;
} clipboard_t;

typedef struct
{
	int vid_mode;
} options_t;

//extern float start_time;

extern float mouse_sens_x;
extern float mouse_sens_y;


/*
This used to be in display.h, but multiple maps stuff requires it to be
here.
*/
typedef struct viewport_s
{
/* 1 if this viewport is 'active'. Basically, if it's 0, it's completely
ignored. Note that this isn't the same as a viewport being outside the
M.display.num_vports range, a viewport inside the range should have valid
values even if used is 0. */
   int     used;

	int     xmin;
	int     ymin;
	int     xmax;
	int     ymax;
	float   zoom_amt;

   float   f_xmin,f_xmax; // Values from 0 to 1, xmin et al are calculated
   float   f_ymin,f_ymax; // from these based on the current area available
                          // for viewports.

	int     mode;
	int     fullbright;
   int     textured;
	int     grid_type;

	lvec3_t camera_pos;
	int     camera_dir;

   int     axis_aligned;
   int     rot_x;
   int     rot_y;
   int     rot_z;

	float  *zbuffer;
} viewport_t;

typedef struct
{
	/* Viewports */
	viewport_t vport[MAX_NUM_VIEWPORTS];
	int        num_vports;
	int        active_vport;

/* If 1, the active viewport should cover the entire map space. */
   int        vp_full;

	/* Vertex selection */
	int          num_vselected;
	vsel_t      *vsel;
	/* Face selection */
	int          num_fselected;
	fsel_t      *fsel;
	/* Brush selection */
	int          num_bselected;
	brushref_t  *bsel;
	/* Entity selection */
	int          num_eselected;
	entityref_t *esel;
} display_t;


typedef struct
{
   struct brush_s *BrushHead;  /* head of linked list of brushes */
   entity_t       *EntityHead; /* head of linked list of entities */
   entity_t       *WorldSpawn; /* points to the worldspawn entity */
   group_t        *GroupHead;  /* head of linked list of groups */
   group_t        *CurGroup;   /* points to group that is to be saved */
   group_t        *WorldGroup; /* group containing all brushes NOT in any */
                               /* user specified group (aka the default group) */


/* This maps texture cache. */
   struct texture_s *Cache;
   int num_textures;


/*
  self explanatory counter variables
 number of brushes, entities, groups, unique vertices, unique edges, and
 targets

 will these always hold correct values???
*/
   int num_brushes;
   int num_entities;
   int num_groups;
   int num_vertices;
   int num_edges;

   entity_t   *cur_entity;      /* points to the "current" entity. used */
                                /* in updating the status area */
   struct brush_s *cur_brush;   /* points to the "current" brush. used in */
                                /* updating the status area */
   fsel_t     cur_face;         /* same as cur_entity and cur_brush */
   char       cur_texname[128]; /* same */

   char mapfilename[256];

   display_t display;

   struct undo_info_s *undo[MAX_UNDO];
   int n_undo;


// bsp stuff
   struct bsp_node_s *BSPHead;
   struct bsp_map_face_s **BSPmf;
   int num_mf;

   vec3_t *BSPverts;
   struct vec3_vs *TBSPverts;
   int *BSPhash;      // hash stuff for finding unique vertices
   int *BSPnextvec;

   int totalverts;
   int uniqueverts;

   int startnumber;
   int totalfaces;


// pts/lin stuff
   vec3_t *pts;
   vec3_t *tpts;
   int npts;


// map specific settings
   int showsel;
   int modified;
} map_t;

extern map_t maps[MAX_MAPS];
extern map_t M;
extern int cur_map;

void SwitchMap(int newmap,int verbose);

extern clipboard_t Clipboard;

extern char texturename[256];

extern char profile[256];

extern options_t options;

extern int take_screenshot;

extern float gammaval;

extern int MenuShowing;

extern char **argv;
extern int argc;

void TakeScreenShot(void);

void dprint(const char *format, ...) __attribute__ ((format(printf,1,2)));

#endif

