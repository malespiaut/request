#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "quake3.h"

#include "q3_jpg.h"

#include "game.h"
#include "tex.h"
#include "tex_all.h"
#include "texcat.h"

#include "color.h"
#include "error.h"
#include "filedir.h"
#include "memory.h"
#include "pakdir.h"
#include "q2_texi2.h"
#include "qmap.h"
#include "status.h"
#include "token.h"

static int
Q3_LoadTexInfo(texdef_t* tex)
{
  int i;

  i = QMap_LoadTexInfo(tex);
  if (i)
    return i;

  if (TokenAvailable(0))
  {
    GETTOKEN(0, T_NUMBER);
    tex->g.q2.contents = atoi(token);

    GETTOKEN(0, T_NUMBER);
    tex->g.q2.flags = atoi(token);

    GETTOKEN(0, T_NUMBER);
    tex->g.q2.value = atoi(token);
  }
  else
  {
    tex->g.q2.contents = 0;
    tex->g.q2.flags = 0;
    tex->g.q2.value = 0;
  }

  return ERROR_NO;
}

static void
Q3_SaveTexInfo(texdef_t* tex, FILE* fp)
{
  QMap_SaveTexInfo(tex, fp);

  if (tex->g.q2.contents ||
      tex->g.q2.flags ||
      tex->g.q2.value)
  {
    fprintf(fp, " %i %i %i", tex->g.q2.contents, tex->g.q2.flags, tex->g.q2.value);
  }
}

static void
Q3_ModifyFlags(void)
{
  static const char* q3_contents[32] =
    {
      NULL, NULL, NULL, NULL, // 00000001
      NULL,
      NULL,
      NULL,
      NULL, // 00000010
      NULL,
      NULL,
      NULL,
      NULL, // 00000100
      NULL,
      NULL,
      NULL,
      NULL, // 00001000
      NULL,
      NULL,
      NULL,
      NULL, // 00010000
      NULL,
      NULL,
      NULL,
      NULL, // 00100000
      NULL,
      NULL,
      NULL,
      "Detail", // 01000000
      NULL,
      NULL,
      NULL,
      NULL, // 10000000
    };
  static const char* q3_flags[32] =
    {
      NULL, NULL, NULL, NULL, // 00000001
      NULL,
      NULL,
      NULL,
      NULL, // 00000010
      NULL,
      NULL,
      NULL,
      NULL, // 00000100
      NULL,
      NULL,
      NULL,
      NULL, // 00001000
      NULL,
      NULL,
      NULL,
      NULL, // 00010000
      NULL,
      NULL,
      NULL,
      NULL, // 00100000
      NULL,
      NULL,
      NULL,
      NULL, // 01000000
      NULL,
      NULL,
      NULL,
      NULL, // 10000000
    };

  Quake2Flags_ModifyFlags(q3_contents, q3_flags);
}

static void
Q3_FlagsDefault(texdef_t* tex)
{
  tex->g.q2.contents = tex->g.q2.flags = tex->g.q2.value = 0;
}

static int
Q3_TexSort(const char* t1, const char* t2)
{
  return strcmp(t1, t2);
}

static int er, eg, eb;

static int
Dith(int r, int g, int b)
{
  int best;
  //   unsigned char *c;

  r += er;
  g += eg;
  b += eb;

  if (r < 0)
    r = 0;
  if (g < 0)
    g = 0;
  if (b < 0)
    b = 0;
  if (r > 63)
    r = 63;
  if (g > 63)
    g = 63;
  if (b > 63)
    b = 63;

  best = ((r >> 3) << 2) +
         ((g >> 3) << 5) +
         ((b >> 4) << 0);

  //   c=&texture_pal[best*3];
  //   er=r-c[0];
  //   eg=g-c[1];
  //   eb=b-c[2];

  er = r & 7;
  eg = g & 7;
  eb = b & 15;

  return best;
}

static void
AddTName(char* name, int nl, int ofs)
{
  tex_name_t* t;
  int i;

  for (i = 0; i < n_tnames; i++)
  {
    if (!strcmp(tnames[i].filename, name))
      return;
  }

  tnames = Q_realloc(tnames, sizeof(tex_name_t) * (n_tnames + 1));
  if (!tnames)
    Abort("Q3_Init", "Out of memory!");
  t = &tnames[n_tnames];
  memset(t, 0, sizeof(tex_name_t));
  n_tnames++;

  strcpy(t->filename, name);
  strcpy(t->name, &name[strlen("textures/")]);
  t->name[strlen(t->name) - 4] = 0;
  //   printf("t->name='%s' t->filename='%s'\n",t->name,t->filename);

  t->location = nl;
  t->ofs = ofs;
}

/*
This array will contain one entry for each actual .tga and .jpg file.
The tnames structure the rest of the code will use has entries for
shaders instead. They will also have pointers to this array so this
code knows where to find the actual texture.
*/
static tex_name_t* real_tn;
static int n_real_tn;

/* Used internally to keep track of shader information (not yet, though). */
typedef struct
{
  int light; /* 0 if no light */
  int flags;
#define SHADER_SKY 1
#define SHADER_LAVA 2
#define SHADER_WATER 4
#define SHADER_SLIME 8
#define SHADER_TRANS 16
#define SHADER_NODRAW 32
#define SHADER_NOTSOLID 64
#define SHADER_FOG 128
} q3_tn_int_t;

static int
Q3_TexBSPFlags(texdef_t* t)
{
  int i;
  tex_name_t* tn;
  q3_tn_int_t* sh;

  for (i = n_tnames, tn = tnames; i; i--, tn++)
    if (!stricmp(t->name, tn->name))
      break;

  if (!i)
    return 0;
  if (!tn->x)
    return 0;
  sh = (q3_tn_int_t*)tn->x;

  if (sh->flags & SHADER_SKY)
    return TEX_FULLBRIGHT;

  i = 0;
  if (sh->flags & SHADER_NODRAW)
    i |= TEX_NODRAW;
  if (sh->flags &
      (SHADER_WATER | SHADER_SLIME | SHADER_LAVA | SHADER_FOG | SHADER_NOTSOLID | SHADER_TRANS))
    i |= TEX_FULLBRIGHT | TEX_NONSOLID;

  return i;
}

static void
Q3_TextureDesc(char* dest, const texture_t* t)
{
  int i;
  tex_name_t* tn;
  q3_tn_int_t* sh;

  for (i = n_tnames, tn = tnames; i; i--, tn++)
    if (!stricmp(t->name, tn->name))
      break;
  if (!tn)
  {
    sprintf(dest, "unknown");
    return;
  }
  if (!tn->x)
  {
    sprintf(dest, "raw image");
    return;
  }
  sh = (q3_tn_int_t*)tn->x;

  i = sprintf(dest, "shader ");

  if (sh->light)
    i += sprintf(&dest[i], "light=%i ", sh->light);
  if (sh->flags & SHADER_SKY)
    i += sprintf(&dest[i], "sky ");
  if (sh->flags & SHADER_WATER)
    i += sprintf(&dest[i], "water ");
  if (sh->flags & SHADER_LAVA)
    i += sprintf(&dest[i], "lava ");
  if (sh->flags & SHADER_SLIME)
    i += sprintf(&dest[i], "slime ");
  if (sh->flags & SHADER_FOG)
    i += sprintf(&dest[i], "fog ");
  if (sh->flags & SHADER_TRANS)
    i += sprintf(&dest[i], "trans ");
  if (sh->flags & SHADER_NODRAW)
    i += sprintf(&dest[i], "nodraw ");
  if (sh->flags & SHADER_NOTSOLID)
    i += sprintf(&dest[i], "notsolid ");

  if (dest[0])
    dest[strlen(dest) - 1] = 0;
}

typedef struct
{
  unsigned char id_len __attribute__((packed));
  unsigned char cm_type __attribute__((packed));
  unsigned char image_type __attribute__((packed));

  unsigned short cm_start __attribute__((packed));
  unsigned short cm_len __attribute__((packed));
  unsigned char cm_size __attribute__((packed));

  unsigned short org_x __attribute__((packed));
  unsigned short org_y __attribute__((packed));
  unsigned short width __attribute__((packed));
  unsigned short height __attribute__((packed));
  unsigned char bpp __attribute__((packed));

  unsigned char flags __attribute__((packed));
} tga_header_t __attribute__((packed));

static texture_t*
Q3_LoadTGA(tex_name_t* real, tex_name_t* tn)
{
  FILE* f;
  int closefile, baseofs;
  unsigned char *data, *d, *d2;

  int i, j, k;
  int x, y;
  int c[3];

  int scale;

  tga_header_t h;

  texture_t* t;

  f = PD_Load(real, &baseofs, &closefile);
  if (!f)
  {
    HandleError("Q3_LoadTGA", "Can't open '%s'!", real->filename);
    return NULL;
  }

  fseek(f, baseofs, SEEK_SET);
  fread(&h, 1, sizeof(tga_header_t), f);

  t = Q_malloc(sizeof(texture_t));
  if (!t)
  {
    HandleError("Q3_LoadTGA", "Out of memory!");
    return NULL;
  }
  memset(t, 0, sizeof(texture_t));

  strcpy(t->name, tn->name);
  t->rsx = t->dsx = h.width;
  t->rsy = t->dsy = h.height;
  t->color = -1;

  scale = 0;
  if (t->dsx > 32 && t->dsy > 32)
  {
    t->dsx /= 2;
    t->dsy /= 2;
    scale++;
  }

  while (t->dsx > 192 || t->dsy > 192)
  {
    t->dsx /= 2;
    t->dsy /= 2;
    scale++;
  }
  //   fprintf(stderr,"scale=%i  %ix%i  %ix%i\n",scale,t->rsx,t->rsy,t->dsx,t->dsy);

  fseek(f, baseofs + sizeof(tga_header_t) + h.id_len, SEEK_SET);

  data = Q_malloc(t->dsx * t->dsy * 3);
  t->data = Q_malloc(t->dsx * t->dsy);
  if (!data || !t->data)
  {
    HandleError("Q3_LoadTGA", "Out of memory!");
    return NULL;
  }
  memset(data, 0, t->dsx * t->dsy * 3);

  switch (h.image_type)
  {
    case 2:
      if (h.bpp != 32 && h.bpp != 24)
      {
        HandleError("Q3_LoadTGA", "type=%i and bpp=%i", h.image_type, h.bpp);
        return NULL;
      }

      for (i = 0; i < h.width * h.height; i++)
      {
        y = i / h.width;
        x = i - y * h.width;

        x >>= scale;
        y >>= scale;

        c[2] = fgetc(f);
        c[1] = fgetc(f);
        c[0] = fgetc(f);

        if (h.bpp == 32)
          fgetc(f);

        if (x < 0 || x >= t->dsx || y < 0 || y >= t->dsy)
        {
          //            fprintf(stderr,"%8i -> %4i x %4i (%4ix%4i  %4ix%4i)\n",i,x,y,t->dsx,t->dsy,t->rsx,t->rsy);
          //            abort();
          continue;
        }

        d = &data[3 * (x + t->dsx * y)];

        *d++ += c[0] >> (2 * scale);
        *d++ += c[1] >> (2 * scale);
        *d++ += c[2] >> (2 * scale);
      }
      break;

    case 10:
      if (h.bpp != 32 && h.bpp != 24)
      {
        HandleError("Q3_LoadTGA", "type=%i and bpp=%i", h.image_type, h.bpp);
        return NULL;
      }

      for (i = 0; i < h.width * h.height;)
      {
        j = ((k = fgetc(f)) & 127) + 1;
        if (k & 128)
        {
          c[2] = fgetc(f) >> (2 * scale);
          c[1] = fgetc(f) >> (2 * scale);
          c[0] = fgetc(f) >> (2 * scale);
          if (h.bpp == 32)
            fgetc(f);

          while (j--)
          {
            y = i / h.width;
            x = i - y * h.width;

            x >>= scale;
            y >>= scale;

            i++;

            if (x < 0 || x >= t->dsx || y < 0 || y >= t->dsy)
              continue;

            d = &data[3 * (x + t->dsx * y)];

            *d++ += c[0];
            *d++ += c[1];
            *d++ += c[2];
          }
        }
        else
        {
          while (j--)
          {
            c[2] = fgetc(f) >> (2 * scale);
            c[1] = fgetc(f) >> (2 * scale);
            c[0] = fgetc(f) >> (2 * scale);
            if (h.bpp == 32)
              fgetc(f);

            y = i / h.width;
            x = i - y * h.width;

            x >>= scale;
            y >>= scale;

            i++;
            if (x < 0 || x >= t->dsx || y < 0 || y >= t->dsy)
              continue;
            d = &data[3 * (x + t->dsx * y)];

            *d++ += c[0];
            *d++ += c[1];
            *d++ += c[2];
          }
        }
      }
      break;

    default:
      HandleError("Q3_LoadTGA", ".tga type %i", h.image_type);
      return NULL;
  }

  /*   fprintf(stderr,"Loaded %s  %ix%i (%ix%i)  %i %ibpp\n",
        t->name,
        t->rsx,t->rsy,
        t->dsx,t->dsy,
        h.image_type,h.bpp);*/

  er = eg = eb = 0;
  for (d2 = t->data, y = 0; y < t->dsy; y++)
  {
    for (x = 0; x < t->dsx; x++)
    {
      d = &data[3 * (x + (t->dsy - y - 1) * t->dsx)];

      *d2++ = Dith(d[0] / 4, d[1] / 4, d[2] / 4);
    }
  }

  Q_free(data);

  if (closefile)
    fclose(f);

  return t;
}

static int
Q3_LoadTexture(tex_name_t* tn)
{
  tex_name_t* real;
  texture_t* t;

  if (tn->location == -1)
  {
    //      HandleError("Q3_LoadTexture","Can't find .tga file for shader '%s'!",tn->name);
    return 0;
  }

  real = &real_tn[tn->location];
  if (real->tex)
  {
    tn->tex = Q_malloc(sizeof(texture_t));
    if (!tn->tex)
    {
      HandleError("Q3_LoadTexture", "Out of memory!");
      return 0;
    }
    *(tn->tex) = *(real->tex);
    strcpy(tn->tex->name, tn->name);
    return 1;
  }

  if (!stricmp(&real->filename[strlen(real->filename) - 4], ".jpg"))
    t = Q3_LoadJPG(real, tn);
  else
    t = Q3_LoadTGA(real, tn);

  if (!t)
    return 0;

  //   fprintf(stderr,"Loaded %s  %ix%i  %ix%i\n",t->name,t->rsx,t->rsy,t->dsx,t->dsy);

  tn->tex = real->tex = t;

  return 1;
}

static void
AddShader(char* name, int ref, q3_tn_int_t* sh)
{
  tex_name_t* t;
  int i;

  strlwr(name);
  for (i = 0; i < n_tnames; i++)
  {
    if (!stricmp(tnames[i].name, name))
      return;
  }

  tnames = Q_realloc(tnames, sizeof(tex_name_t) * (n_tnames + 1));
  if (!tnames)
    Abort("Q3_AddShader", "Out of memory!");
  t = &tnames[n_tnames];
  memset(t, 0, sizeof(tex_name_t));
  n_tnames++;

  strcpy(t->name, name);
  t->ofs = -1;
  t->location = ref;

  if (sh)
  {
    t->x = Q_malloc(sizeof(q3_tn_int_t));
    if (!t->x)
      Abort("Q3_AddShader", "Out of memory!");
    memcpy(t->x, sh, sizeof(q3_tn_int_t));
  }
}

static int
LookupImage(const char* iname)
{
  const char* in;
  int len;
  int i;
  tex_name_t* r;

  if (strncmp(iname, "textures/", 9))
    return -1;

  in = iname + 9;
  len = strlen(in);
  if (!stricmp(&in[len - 4], ".tga"))
    len -= 4;

  for (i = 0, r = real_tn; i < n_real_tn; i++, r++)
    if (!r->name[len] && !strnicmp(r->name, in, len))
      return i;

  return -1;
}

static void
LoadShaderFile(char* fname)
{
  char name[256];
  int brace_level;
  int i;
  int i_image, i_map;

  q3_tn_int_t sh;

  if (!TokenFile(fname, T_C | T_ALLNAME, NULL))
  {
    HandleError("Q3_LoadShader", "Can't open '%s'!", fname);
    return;
  }

  while (TokenGet(1, -1))
  {
    strcpy(name, token);
    i_image = i_map = -1;

    TokenGet(1, -1);
    if (strcmp(token, "{"))
      Abort("Q3_LoadShader", "Expected '{', got '%s'!", token);

    brace_level = 1;
    memset(&sh, 0, sizeof(sh));
    do
    {
      TokenGet(1, -1);
      if (!strcmp(token, "{"))
        brace_level++;
      else if (!strcmp(token, "}"))
        brace_level--;

      /* general shader information */
      else if (!strcmp(token, "surfaceparm"))
      {
        TokenGet(1, -1);
        if (!strcmp(token, "fog"))
          sh.flags |= SHADER_FOG;
        else if (!strcmp(token, "nodraw"))
          sh.flags |= SHADER_NODRAW;
        else if (!strcmp(token, "nonsolid"))
          sh.flags |= SHADER_NOTSOLID;
        else if (!strcmp(token, "trans"))
          sh.flags |= SHADER_TRANS;

        else if (!strcmp(token, "lava"))
          sh.flags |= SHADER_LAVA;
        else if (!strcmp(token, "slime"))
          sh.flags |= SHADER_SLIME;
        else if (!strcmp(token, "water"))
          sh.flags |= SHADER_WATER;
        else if (!strcmp(token, "sky"))
          sh.flags |= SHADER_SKY;
      }
      else if (!strcmp(token, "q3map_surfacelight"))
      {
        TokenGet(1, -1);
        sh.light = atof(token);
      }

      else if (!strcmp(token, "qer_editorimage"))
      {
        TokenGet(1, -1);
        i_image = LookupImage(token);
      }

      else if (!strcmp(token, "map"))
      {
        TokenGet(1, -1);
        if (token[0] != '$')
        {
          i = LookupImage(token);
          if (i_map == -1 && i != -1)
            i_map = i;
        }
      }

      else
      {
        while (TokenAvailable(0))
          TokenGet(0, T_ALLNAME);
      }

    } while (brace_level);

    if (i_image == -1)
    {
      i_image = LookupImage(name);
      if (i_image == -1)
        i_image = i_map;
    }

    if (!strnicmp(name, "textures/", 9))
      AddShader(&name[9], i_image, &sh);
    else
      AddShader(name, i_image, &sh);
  }

  TokenDone();
}

static void
LoadShaders(void)
{
  struct directory_s* d;
  filedir_t f;
  char name[256];
  int len;

  strcpy(name, status.tex_str);
  if (name[0] && name[strlen(name) - 1] != '/')
    strcat(name, "/");
  strcat(name, "scripts/");
  len = strlen(name);
  strcat(name, "*.shader");

  d = DirOpen(name, FILE_NORMAL);
  if (!d)
  {
    HandleError("Q3_LoadShaders", "Can't find shaders!");
    return;
  }

  while (DirRead(d, &f))
  {
    strcpy(&name[len], f.name);
    LoadShaderFile(name);
  }

  DirClose(d);
}

static void
MakeShaders(void)
{
  int i, j;
  int old_num;
  tex_name_t* rt;

  old_num = n_tnames;
  for (i = 0, rt = real_tn; i < n_real_tn; i++, rt++)
  {
    for (j = 0; j < old_num; j++)
    {
      if (!stricmp(tnames[j].name, rt->name))
        break;
    }
    if (j < old_num)
      continue;

    AddShader(rt->name, rt - real_tn, NULL);
  }
}

static int
Q3_GetTexCat(char* name)
{
  char* c;
  int i, j;
  category_t* t;

  c = strchr(name, '/');
  if (!c)
    return -1;
  j = c - name + 1;

  for (i = 0, t = tcategories; i < n_tcategories; i++, t++)
  {
    if (!strnicmp(&t->name[2], name, j))
      return i;
  }

  HandleError("Q3_GetTexCat", "!!c but can't find category");
  return -1;
}

static game_t Game_Quake3 =
  {
    "Quake3",
    {1,
     0,

     Q3_TexSort,
     NULL,
     Q3_LoadTexture,
     Q3_GetTexCat,
     Q3_FlagsDefault,
     Q3_ModifyFlags,
     Q3_TexBSPFlags,
     Q3_TextureDesc},
    {QMap_Load,
     QMap_Save,
     QMap_SaveVisible,
     QMap_LoadGroup,
     QMap_SaveGroup,
     QMap_Profile},
    {L_QUAKE2,
     1,
     0},
    {1},
    ".lin"};

void
Q3_Init(void)
{
  Game = Game_Quake3;
  qmap_loadtexinfo = Q3_LoadTexInfo;
  qmap_savetexinfo = Q3_SaveTexInfo;

  if (!PD_Init())
  {
    PD_Search("textures/", ".tga", AddTName);
    PD_Search("textures/", ".jpg", AddTName);
    PD_Write();
  }

  real_tn = tnames;
  n_real_tn = n_tnames;

  tnames = NULL;
  n_tnames = 0;

  LoadShaders();

  MakeShaders();

  {
    int i;
    int r, g, b;
    unsigned char tex_pal[768];

    for (i = 0; i < 256; i++)
    {
      r = ((i >> 2) & 7) * 8;
      g = ((i >> 5) & 7) * 8;
      b = ((i >> 0) & 3) * 16;

      tex_pal[i * 3 + 0] = r;
      tex_pal[i * 3 + 1] = g;
      tex_pal[i * 3 + 2] = b;
    }
    SetTexturePal(tex_pal);
  }

  /*   {
        int i;
        tex_name_t *t;

  #define PRINT printf("  l/o=(%3i %3i) %08x  %s\n",t->location,t->ofs,(int)t->x,t->name);

        printf("Real:\n");
        for (i=0,t=real_tn;i<n_real_tn;i++,t++) PRINT
        printf("Shaders:\n");
        for (i=0,t=tnames;i<n_tnames;i++,t++) PRINT
     }*/
  //   Abort("Foo","Zot");

  tcategories = NULL;
  n_tcategories = 0;
  {
    int i, j, k;
    tex_name_t* t;
    category_t* ct;
    char* c;

    for (i = 0, t = tnames; i < n_tnames; i++, t++)
    {
      c = strchr(t->name, '/');
      if (!c)
        continue;
      k = c - t->name + 1;
      for (j = 0, ct = tcategories; j < n_tcategories; j++, ct++)
        if (!strnicmp(&ct->name[2], t->name, k))
          break;
      if (j < n_tcategories)
        continue;
      tcategories = Q_realloc(tcategories, sizeof(category_t) * (j + 1));
      if (!tcategories)
        Abort("Q3_Init", "Out of memory!");
      ct = &tcategories[j++];
      strcpy(ct->name, "- ");
      strcat(ct->name, t->name);
      *(strchr(ct->name, '/') + 1) = 0;
      n_tcategories++;
    }
  }
}

/* TODO */

/*


(a,b,c) -> (d,e,f)

d=a+vx*t
e=b+vy*t
f=c+vz*t-400*t^2

vz-800*t=0
vz=800*t

f=c+800*t*t-400*t*t=c+400*t^2
t^2=(f-c)/400
t=(f-c)^0.5/20

*/
#include <math.h>

#include "3d.h"
#include "entity.h"
#include "quest.h"

#define NUMPTS 32
#define STEP 0.1
#define GRAVITY 800.0
void
Quake3_JumpPad(void)
{
  vec3_t to;
  const char *c1, *c2;
  entity_t *e1, *e2;

  vec3_t pos, vel;
  float t;

  int i;

  if (M.display.num_eselected != 2)
    return;

  FreePts();
  M.pts = Q_malloc(sizeof(vec3_t) * NUMPTS);
  M.tpts = Q_malloc(sizeof(vec3_t) * NUMPTS);
  if (!M.pts || !M.tpts)
  {
    Q_free(M.pts);
    Q_free(M.tpts);
    M.pts = M.tpts = NULL;
    return;
  }
  status.draw_pts = 1;
  M.npts = NUMPTS;

  c1 = GetKeyValue(M.display.esel->Entity, "classname");
  c2 = GetKeyValue(M.display.esel->Next->Entity, "classname");

  if (!strcmp(c1, "target_position"))
    e1 = M.display.esel->Entity, e2 = M.display.esel->Next->Entity;
  else
    e2 = M.display.esel->Entity, e1 = M.display.esel->Next->Entity;

  c1 = GetKeyValue(e1, "origin");
  sscanf(c1, "%f %f %f", &to.x, &to.y, &to.z);
  pos = e2->center;

  /*   printf("(%g %g %g) -> (%g %g %g)\n",
        pos.x,pos.y,pos.z,to.x,to.y,to.z);*/

  t = to.z - pos.z;
  if (t < 0)
    t = 0;

  t = sqrt(t) / 20; /* 20=sqrt(gravity/2) */

  vel.z = t * GRAVITY;
  vel.x = (to.x - pos.x) / t;
  vel.y = (to.y - pos.y) / t;

  //   printf("t=%g vel=(%g %g %g)\n",t,vel.x,vel.y,vel.z);

  for (i = 0; i < NUMPTS; i++)
  {
    M.pts[i] = pos;
    pos.x += vel.x * STEP;
    pos.y += vel.y * STEP;
    pos.z += vel.z * STEP;

    vel.z -= GRAVITY * STEP;
  }
}
