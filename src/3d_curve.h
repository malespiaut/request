#ifndef _3D_CURVE_H
#define _3D_CURVE_H

struct brush_s;
struct plane_s;

void DrawBrush_Q3Curve(int vport,struct brush_s *b,int col,int selected);

void CurveGetPoints(struct brush_s *b,vec3_t *bverts,struct plane_s *p,
   vec3_t *dpoints,vec3_t *tpoints,int splits);

#endif

