//
// CQ2Render.h
//

#ifndef CQ2_MESH_RENDERING_HEADER_FILE_H_
#define CQ2_MESH_RENDERING_HEADER_FILE_H_

struct RenderArch;
struct IMeshObj;

bool ArchToMeshObj( ARCHETYPE_INDEX archID, RenderArch** renderArch, IMeshObj** );

#endif