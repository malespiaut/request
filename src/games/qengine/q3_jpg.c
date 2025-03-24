#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "q3_jpg.h"

#include "color.h"
#include "error.h"
#include "memory.h"
#include "pakdir.h"
#include "tex.h"
#include "tex_all.h"

#include <jpeglib.h>

texture_t*
Q3_LoadJPG(tex_name_t* real, tex_name_t* tn)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  FILE* f;
  int bo, cf;

  int i;

  unsigned char palette[768];
  JSAMPROW pal[3];
  JSAMPROW row;

  texture_t* t;

  //   printf("Q3_LoadJPG: '%s'  '%s'\n",real->filename,tn->name);

  f = PD_Load(real, &bo, &cf);
  if (!f)
  {
    HandleError("Q3_LoadJPG", "Unable to open '%s'!", real->filename);
    return NULL;
  }
  fseek(f, bo, SEEK_SET);

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpeg_stdio_src(&cinfo, f);
  jpeg_read_header(&cinfo, TRUE);

  for (i = 0; i < 3; i++)
    if (cinfo.image_width >> i <= 192 && cinfo.image_height >> i <= 192)
      break;
  cinfo.scale_num = 1;
  cinfo.scale_denom = 1 << i;

  cinfo.quantize_colors = TRUE;
  cinfo.dither_mode = JDITHER_FS;

  for (i = 0; i < 256; i++)
  {
    palette[i + 0] = texture_pal[i * 3 + 0] << 2;
    palette[i + 256] = texture_pal[i * 3 + 1] << 2;
    palette[i + 512] = texture_pal[i * 3 + 2] << 2;
  }
  pal[0] = &palette[0];
  pal[1] = &palette[256];
  pal[2] = &palette[512];

  cinfo.actual_number_of_colors = 256;
  cinfo.colormap = pal;

  /* Make things faster */
  cinfo.dct_method = JDCT_FLOAT;
  cinfo.do_fancy_upsampling = FALSE;

  jpeg_start_decompress(&cinfo);

  /*   printf("%3ix%3i %i -> %3ix%3i %i  %i\n",
        cinfo.image_width,cinfo.image_height,cinfo.num_components,
        cinfo.output_width,cinfo.output_height,cinfo.output_components,
        cinfo.rec_outbuf_height);*/

  t = Q_malloc(sizeof(texture_t));
  if (!t)
  {
    jpeg_destroy_decompress(&cinfo);
    fclose(f);
    HandleError("Q3_LoadJPG", "Out of memory!");
    return NULL;
  }
  memset(t, 0, sizeof(texture_t));
  strcpy(t->name, tn->name);

  t->rsx = cinfo.image_width;
  t->rsy = cinfo.image_height;
  t->dsx = cinfo.output_width;
  t->dsy = cinfo.output_height;
  t->color = -1;

  t->data = Q_malloc(t->dsx * t->dsy);
  if (!t->data)
  {
    jpeg_destroy_decompress(&cinfo);
    fclose(f);
    HandleError("Q3_LoadJPG", "Out of memory!");
    return NULL;
  }

  while (cinfo.output_scanline < cinfo.output_height)
  {
    row = &t->data[cinfo.output_scanline * t->dsx];
    jpeg_read_scanlines(&cinfo, &row, 1);
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  fclose(f);

  return t;
}
