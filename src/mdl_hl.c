#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "mdl.h"
#include "mdl_all.h"

#include "mdl_hl.h"

#include "error.h"
#include "memory.h"


typedef struct 
{
	int					id;
	int					version;

	char				name[64];
	int					length;

	vec3_t				eyeposition;	// ideal eye position
	vec3_t				min;			// ideal movement hull size
	vec3_t				max;			

	vec3_t				bbmin;			// clipping bounding box
	vec3_t				bbmax;		

	int					flags;

	int					numbones;			// bones
	int					boneindex;

	int					numbonecontrollers;		// bone controllers
	int					bonecontrollerindex;

	int					numhitboxes;			// complex bounding boxes
	int					hitboxindex;			
	
	int					numseq;				// animation sequences
	int					seqindex;

	int					numseqgroups;		// demand loaded sequences
	int					seqgroupindex;

	int					numtextures;		// raw textures
	int					textureindex;
	int					texturedataindex;

	int					numskinref;			// replaceable textures
	int					numskinfamilies;
	int					skinindex;

	int					numbodyparts;		
	int					bodypartindex;

	int					numattachments;		// queryable attachable points
	int					attachmentindex;

	int					soundtable;
	int					soundindex;
	int					soundgroups;
	int					soundgroupindex;

	int					numtransitions;		// animation node to animation node transition graph
	int					transitionindex;
} studiohdr_t;

// header for demand loaded sequence group data
typedef struct 
{
	int					id;
	int					version;

	char				name[64];
	int					length;
} studioseqhdr_t;

// bones
typedef struct 
{
	char				name[32];	// bone name for symbolic links
	int		 			parent;		// parent bone
	int					flags;		// ??
	int					bonecontroller[6];	// bone controller index, -1 == none
	float				value[6];	// default DoF values
	float				scale[6];   // scale for delta DoF values
} mstudiobone_t;


// bone controllers
typedef struct 
{
	int					bone;	// -1 == 0
	int					type;	// X, Y, Z, XR, YR, ZR, M
	float				start;
	float				end;
	int					rest;	// byte index value at rest
	int					index;	// 0-3 user set controller, 4 mouth
} mstudiobonecontroller_t;

// intersection boxes
typedef struct
{
	int					bone;
	int					group;			// intersection group
	vec3_t				bbmin;		// bounding box
	vec3_t				bbmax;		
} mstudiobbox_t;

typedef void *cache_user_t;

// demand loaded sequence groups
typedef struct
{
	char				label[32];	// textual name
	char				name[64];	// file name
	cache_user_t		cache;		// cache index pointer
	int					data;		// hack for group 0
} mstudioseqgroup_t;

// sequence descriptions
typedef struct
{
	char				label[32];	// sequence label

	float				fps;		// frames per second	
	int					flags;		// looping/non-looping flags

	int					activity;
	int					actweight;

	int					numevents;
	int					eventindex;

	int					numframes;	// number of frames per sequence

	int					numpivots;	// number of foot pivots
	int					pivotindex;

	int					motiontype;	
	int					motionbone;
	vec3_t				linearmovement;
	int					automoveposindex;
	int					automoveangleindex;

	vec3_t				bbmin;		// per sequence bounding box
	vec3_t				bbmax;		

	int					numblends;
	int					animindex;		// mstudioanim_t pointer relative to start of sequence group data
										// [blend][bone][X, Y, Z, XR, YR, ZR]

	int					blendtype[2];	// X, Y, Z, XR, YR, ZR
	float				blendstart[2];	// starting value
	float				blendend[2];	// ending value
	int					blendparent;

	int					seqgroup;		// sequence group for demand loading

	int					entrynode;		// transition node at entry
	int					exitnode;		// transition node at exit
	int					nodeflags;		// transition rules
	
	int					nextseq;		// auto advancing sequences
} mstudioseqdesc_t;

// events
typedef struct 
{
	int 				frame;
	int					event;
	int					type;
	char				options[64];
} mstudioevent_t;


// pivots
typedef struct 
{
	vec3_t				org;	// pivot point
	int					start;
	int					end;
} mstudiopivot_t;

// attachment
typedef struct 
{
	char				name[32];
	int					type;
	int					bone;
	vec3_t				org;	// attachment point
	vec3_t				vectors[3];
} mstudioattachment_t;

typedef struct
{
	unsigned short	offset[6];
} mstudioanim_t;

// animation frames
typedef union 
{
	struct {
		unsigned char valid;
		unsigned char total;
	} num;
	short		value;
} mstudioanimvalue_t;



// body part index
typedef struct
{
	char				name[64];
	int					nummodels;
	int					base;
	int					modelindex; // index into models array
} mstudiobodyparts_t;



// skin info
typedef struct
{
	char					name[64];
	int						flags;
	int						width;
	int						height;
	int						index;
} mstudiotexture_t;


// skin families
// short	index[skinfamilies][skinref]

// studio models
typedef struct
{
	char				name[64];

	int					type;

	float				boundingradius;

	int					nummesh;
	int					meshindex;

	int					numverts;		// number of unique vertices
	int					vertinfoindex;	// vertex bone info
	int					vertindex;		// vertex vec3_t
	int					numnorms;		// number of unique surface normals
	int					norminfoindex;	// normal bone info
	int					normindex;		// normal vec3_t

	int					numgroups;		// deformation groups
	int					groupindex;
} mstudiomodel_t;


// vec3_t	boundingbox[model][bone][2];	// complex intersection info


// meshes
typedef struct 
{
	int					numtris;
	int					triindex;
	int					skinref;
	int					numnorms;		// per mesh normals
	int					normindex;		// normal vec3_t
} mstudiomesh_t;

// triangles
#if 0
typedef struct 
{
	short				vertindex;		// index into vertex array
	short				normindex;		// index into normal array
	short				s,t;			// s,t position on skin
} mstudiotrivert_t;
#endif

// lighting options
#define STUDIO_NF_FLATSHADE		0x0001
#define STUDIO_NF_CHROME		0x0002
#define STUDIO_NF_FULLBRIGHT	0x0004

// motion flags
#define STUDIO_X		0x0001
#define STUDIO_Y		0x0002	
#define STUDIO_Z		0x0004
#define STUDIO_XR		0x0008
#define STUDIO_YR		0x0010
#define STUDIO_ZR		0x0020
#define STUDIO_LX		0x0040
#define STUDIO_LY		0x0080
#define STUDIO_LZ		0x0100
#define STUDIO_AX		0x0200
#define STUDIO_AY		0x0400
#define STUDIO_AZ		0x0800
#define STUDIO_AXR		0x1000
#define STUDIO_AYR		0x2000
#define STUDIO_AZR		0x4000
#define STUDIO_TYPES	0x7FFF
#define STUDIO_RLOOP	0x8000	// controller that wraps shortest distance

// sequence flags
#define STUDIO_LOOPING	0x0001

// bone flags
#define STUDIO_HAS_NORMALS	0x0001
#define STUDIO_HAS_VERTICES 0x0002
#define STUDIO_HAS_BBOX		0x0004
#define STUDIO_HAS_CHROME	0x0008	// if any of the textures have chrome on them


static studiohdr_t hdr;
static mstudiobodyparts_t bp;
static mstudiomodel_t mdl;
static mstudiomesh_t mesh;


mdl_t *HL_LoadMDL(FILE *f,int baseofs)
{
   int i,j;
   mdl_t *tm;

   printf("Load_HL_MDL %p %i\n",f,baseofs);

   fseek(f,baseofs,SEEK_SET);
   fread(&hdr,1,sizeof(studiohdr_t),f);

/*   printf(
      "id=%04x ver=%i name='%s' length=%i\n"
      "eye position=(%g %g %g)  min=(%g %g %g)  max=(%g %g %g)\n"
      "bbmin=(%g %g %g)  bbmax=(%g %g %g)\n"
      "flags=%i\n"
      "bones=%i %i\n"
	   "seq=%i %i\n"
      "seqgroups=%i %i\n"
      "bodyparts=%i %i\n"
      ,
      hdr.id,
      hdr.version,hdr.name,hdr.length,
      hdr.eyeposition.x,hdr.eyeposition.y,hdr.eyeposition.z,
      hdr.min.x,hdr.min.y,hdr.min.z,
      hdr.max.x,hdr.max.y,hdr.max.z,
      hdr.bbmin.x,hdr.bbmin.y,hdr.bbmin.z,
      hdr.bbmax.x,hdr.bbmax.y,hdr.bbmax.z,
      hdr.flags,

      hdr.numbones,hdr.boneindex,
      hdr.numseq,hdr.seqindex,
      hdr.numseqgroups,hdr.seqgroupindex,
      hdr.numbodyparts,hdr.bodypartindex
      );


   for (i=0;i<hdr.numbodyparts;i++)
   {
      fseek(f,baseofs+hdr.bodypartindex+sizeof(mstudiobodyparts_t)*i,SEEK_SET);
      fread(&bp,1,sizeof(mstudiobodyparts_t),f);
   
      printf("bp %i "
         "name='%s' "
         "nummodels=%i base=%i modelindex=%i\n"
         ,
         i,
         bp.name,bp.nummodels,bp.base,bp.modelindex);

      for (j=0;j<bp.nummodels;j++)
      {
         fseek(f,baseofs+bp.modelindex+sizeof(mstudiomodel_t)*j,SEEK_SET);
         fread(&mdl,1,sizeof(mstudiomodel_t),f);
         printf(
            "  %i name='%s' "
            " type=%i radius=%g nummesh=%i numverts=%i\n"
            ,j,
            mdl.name,
            mdl.type,mdl.boundingradius,mdl.nummesh,mdl.numverts
            );
      }
   }*/

   tm=Q_malloc(sizeof(mdl_t));
   if (!tm)
      Abort("HL_LoadMDL","Out of memory!");
   memset(tm,0,sizeof(mdl_t));

   for (i=0;i<hdr.numbodyparts;i++)
   { // try loading one bodypart, one mesh
      int oldverts;

      oldverts=tm->numvertices;

      fseek(f,baseofs+hdr.bodypartindex+i*sizeof(mstudiobodyparts_t),SEEK_SET);
      fread(&bp,1,sizeof(mstudiobodyparts_t),f);

      fseek(f,baseofs+bp.modelindex,SEEK_SET);
      fread(&mdl,1,sizeof(mstudiomodel_t),f);

      if (!mdl.numverts)
         continue;

      tm->numvertices+=mdl.numverts;
      tm->vertlist=Q_realloc(tm->vertlist,sizeof(vec3_t)*tm->numvertices);
      if (!tm->vertlist)
         Abort("HL_LoadMDL","Out of memory!");

      fseek(f,baseofs+mdl.vertindex,SEEK_SET);
      fread(&tm->vertlist[oldverts],1,sizeof(vec3_t)*mdl.numverts,f);

      printf("%i meshes\n",mdl.nummesh);
      for (j=0;j<mdl.nummesh;j++)
      {
         fseek(f,baseofs+mdl.meshindex+j*sizeof(mstudiomesh_t),SEEK_SET);
         fread(&mesh,1,sizeof(mstudiomesh_t),f);
   
         {
            short cmd;
            short a,b,c;
            int misc[4];
   
            fseek(f,baseofs+mesh.triindex,SEEK_SET);
            fread(&cmd,1,2,f);
            while (cmd)
            {
               if (cmd<0)
               {
                  cmd=-cmd;
                  fread(&a,1,2,f);
                  fread(&misc,1,6,f);
   
                  fread(&b,1,2,f);
                  fread(&misc,1,6,f);
                  for (cmd-=2;cmd>0;cmd--)
                  {
                     fread(&c,1,2,f);
                     fread(&misc,1,6,f);
   
                     AddEdge(tm,oldverts+a,oldverts+b);
                     AddEdge(tm,oldverts+b,oldverts+c);
                     b=c;
                  }
                  AddEdge(tm,oldverts+a,oldverts+b);
               }
               else
               {
                  fread(&a,1,2,f);
                  fread(&misc,1,6,f);
   //               printf("a %i %i\n",a,misc[0]);
                  fread(&b,1,2,f);
                  fread(&misc,1,6,f);
   //               printf("b %i %i\n",b,misc[0]);
                  for (cmd-=2;cmd>0;cmd--)
                  {
                     fread(&c,1,2,f);
                     fread(&misc,1,6,f);
   //               printf("c %i %i\n",c,misc[0]);
   
                     AddEdge(tm,oldverts+a,oldverts+b);
                     AddEdge(tm,oldverts+b,oldverts+c);
                     AddEdge(tm,oldverts+c,oldverts+a);
   
                     a=b;
                     b=c;
                  }
               }
               fread(&cmd,1,2,f);
            }
         }
      }
   }

   printf("%i edges, %i vertices\n",tm->numedges,tm->numvertices);
   return tm;
//   Abort("Load_HL_MDL","Unimplemented!");

   printf("\n");

/*   {
   static int time=0;
   if (time) exit(0);
   time++;
   }*/
   return NULL;
}

