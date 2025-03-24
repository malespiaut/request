#ifndef MAP_H
#define MAP_H

float SnapPointToGrid(float pt);

int VertsAreEqual(vec3_t one, vec3_t two);
int EdgesAreEqual(edge_t one, edge_t two);

void CalcBrushCenter(struct brush_s* b);

void RecalcNormals(struct brush_s* b);
void RecalcAllNormals(void);

void UnloadMap(void);

#endif
