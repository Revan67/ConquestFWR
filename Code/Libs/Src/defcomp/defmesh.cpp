//
//
//
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define MATERIAL_H_INCLUDE_LOAD_CODE 1
#include "FileSys_Utility.h"
#include "material.h"
#undef MATERIAL_H_INCLUDE_LOAD_CODE 

#include "fdump.h"
#include "IDeformable.h"
#include "Deform.h"
//#include "ITXMLib.h"
#include "ITextureLibrary.h"
#include "eng.h"

#include "TsmartPointer.h"
#include "tempstr.h"

//

using namespace Deform;

//

struct DefTriangle
{
	S32 verts[3];
	S32 uvs[3];
	S32 material;
	S32 texture_ID;
	S32 normal;
	SINGLE d_coefficient;
	FACE_PROPERTY properties;

	DefTriangle (void)
	{
		memset (this, -1, sizeof (*this));
	}
};

//

bool LoadChild(void ** buffer, IFileSystem * parent, const char * child_name);
bool ReadChild(void * buffer, IFileSystem * parent, const char * child_name);

//

void GetFullName(IFileSystem * fs, char * full_name)
{
	if(fs != NULL && full_name != NULL)
	{
		full_name[0] = 0;
		
		COMPTR<IFileSystem> parent;
		if(GR_OK == fs->GetParentSystem( parent ) && parent != NULL)
		{
			GetFullName(parent, full_name);
		
			char tmp_name[256];
			fs->GetFileName(tmp_name, 256);
			strcat(full_name, tmp_name);
		}
	}
}

bool DeformablePartArchetype::load_mesh(IFileSystem * file)
{
	bool result = false;
	if (file)
	{
		char full_name[256];
		GetFullName( file, full_name );

		if (file->SetCurrentDirectory("openFLAME 3D N-mesh"))
		{
			result = true;

		// FACE GROUPS.
			if (file->SetCurrentDirectory("Face groups"))
			{
				ReadChild(&face_group_cnt, file, "Count");
				face_groups = new FaceGroup[face_group_cnt];

				int vertex_ref_cnt = 0;

				char group_name[10];
				FaceGroup * group = face_groups;
				for (int i = 0; i < face_group_cnt; i++, group++)
				{
					sprintf(group_name, "Group%d", i);

					if (file->SetCurrentDirectory(group_name))
					{
						ReadChild(&group->material, file, "Material");
						ReadChild(&group->face_cnt, file, "Face count");

						int gvrc = group->face_cnt * 3;
						vertex_ref_cnt += gvrc;

						group->face_vertex_chain = new int[gvrc];
						ReadChild(group->face_vertex_chain,	file, "Face vertex chain");

						group->face_properties = new FACE_PROPERTY[group->face_cnt];
						ReadChild(group->face_properties,	file, "Face property");

						face_cnt += group->face_cnt;
						file->SetCurrentDirectory("..");
					}
				}

				face_group_lookup = new int[face_cnt];
				face_group_index_lookup = new int[face_cnt];
				
				int tmp_face_cnt = 0;

				group = face_groups;
				for (i = 0; i < face_group_cnt; i++, group++)
				{
					int group_start = tmp_face_cnt;

					for (int j = 0; j < group->face_cnt; j++, tmp_face_cnt++)
					{
						face_group_lookup[tmp_face_cnt] = i;
						face_group_index_lookup[tmp_face_cnt] = tmp_face_cnt - group_start;
					}
				}

				file->SetCurrentDirectory("..");
			}

		// VERTICES.
			if (file->SetCurrentDirectory("Vertices"))
			{
				ReadChild(&vertex_batch_cnt,	file, "Vertex batch count");

				vertex_batch_list = new int[vertex_batch_cnt];
				ReadChild(vertex_batch_list,	file, "Vertex batch list");

				texture_batch_list = new int[vertex_batch_cnt];
				ReadChild(texture_batch_list,	file, "Texture batch list");

				texture_batch_list2 = new int[vertex_batch_cnt];
				if( !ReadChild(texture_batch_list2,	file, "Texture batch list2") )
				{
					delete [] texture_batch_list2;
					texture_batch_list2 = NULL;
				}

				ReadChild(&object_vertex_cnt,	file, "Object vertex count");

				ReadChild(&texture_vertex_cnt,	file, "Texture vertex count");

				texture_vertex_list = new TexCoord[texture_vertex_cnt];
				ReadChild(texture_vertex_list,	file, "Texture vertex list");

				vertex_bone_cnt = new int[object_vertex_cnt];
				ReadChild(vertex_bone_cnt, file, "Vertex bone count");

				int bone_array_length = 0;
				int * cnt = vertex_bone_cnt;
				for (int i = 0; i < object_vertex_cnt; i++, cnt++)
				{
					bone_array_length += *cnt;
				}

				vertex_bone_index = new int[object_vertex_cnt];
				ReadChild(vertex_bone_index, file, "First vertex");

				bone_id_list = new int[bone_array_length];
				ReadChild(bone_id_list, file, "Bone id list");

				bone_weight_list = new float[bone_array_length];
				ReadChild(bone_weight_list, file, "Bone weight list");

				bone_vertex_list = new Vector[bone_array_length];
				ReadChild(bone_vertex_list, file, "Bone vertex list");

				bone_normal_list = new Vector[bone_array_length];
				ReadChild(bone_normal_list, file, "Bone normal list");

				// UV bone stuff
				ReadChild(&uv_bone_count,	file, "UV bone count");
				if(uv_bone_count > 0)
				{
					uv_bone_id = new int[uv_bone_count];
					ReadChild(uv_bone_id,			file, "UV bone id");

					uv_vertex_count = new int[uv_bone_count];
					ReadChild(uv_vertex_count,		file, "UV vertex count");

					uv_plane_distance = new float[uv_bone_count];
					ReadChild(uv_plane_distance,	file, "UV plane distance");

					x_to_u_scale = new float[uv_bone_count];
					ReadChild(x_to_u_scale,			file, "Bone X to U scale");
					y_to_v_scale = new float[uv_bone_count];
					ReadChild(y_to_v_scale,			file, "Bone Y to V scale");

					min_du = new float[uv_bone_count];
					ReadChild(min_du,				file, "Min du");
					max_du = new float[uv_bone_count];
					ReadChild(max_du,				file, "Max du");
					min_dv = new float[uv_bone_count];
					ReadChild(min_dv,				file, "Min dv");
					max_dv = new float[uv_bone_count];
					ReadChild(max_dv,				file, "Max dv");

				//
				// ALERT: Freelancer head has data min_du > max_du, so I added code to swap them if
				// necessary. -bb
				//
					for (i = 0; i < uv_bone_count; i++)
					{
						if (min_du[i] > max_du[i])
						{
							float temp = min_du[i];
							min_du[i] = max_du[i];
							max_du[i] = temp;
							GENERAL_WARNING("Bad UV bone imits!\n");
						}
						if (min_dv[i] > max_dv[i])
						{
							float temp = min_dv[i];
							min_dv[i] = max_dv[i];
							max_dv[i] = temp;
							GENERAL_WARNING("Bad UV bone imits!\n");
						}
					}

					ReadChild(&uv_list_length,		file, "UV list length");
					uv_vertex_id = new int[uv_list_length];
					ReadChild(uv_vertex_id,			file, "UV vertex id");
					uv_default_list = new TexCoord[uv_list_length];
					ReadChild(uv_default_list,		file, "UV default list");
				}

				file->SetCurrentDirectory("..");
			}
		}
		else if (file->SetCurrentDirectory("Bezier Patch object"))
		{
			result = true;

			ReadChild(&patch_cnt, file, "Patch count");

			//  patch groups
			if (file->SetCurrentDirectory("Patch groups"))
			{
				ReadChild(&patch_group_cnt, file, "Count");
				patch_groups = new PatchGroup[patch_group_cnt];

				tri_patch_cnt = 0;
				char group_name[10];
				for (int i = 0; i < patch_group_cnt; i++)
				{
					PatchGroup * group = patch_groups + i;
					sprintf(group_name, "Group%d", i);

					if (file->SetCurrentDirectory(group_name))
					{
						ReadChild(&group->mtl_id, file, "Material");
						ReadChild(&group->patch_cnt, file, "Patch count");

						group->patch_list = new BezierPatch[group->patch_cnt];
						ReadChild(group->patch_list, file, "Patch list");
						file->SetCurrentDirectory("..");
					}

					for(int pid = 0; pid < group->patch_cnt; pid++)
					{
						if(group->patch_list[pid].type == 3)
						{
							tri_patch_cnt++;
						}
					}
				}
				file->SetCurrentDirectory("..");
			}

			// Geometry
			if (file->SetCurrentDirectory("Geometry"))
			{
				ReadChild(&object_vertex_cnt,	file, "Vertex count");

				vertex_bone_index = new int[object_vertex_cnt];
				ReadChild(vertex_bone_index,	file, "Vertex bone first");

				vertex_bone_cnt = new int[object_vertex_cnt];
				ReadChild(vertex_bone_cnt,		file, "Vertex bone count");

				int bone_array_length = 0;
				for (int i = 0; i < object_vertex_cnt; i++)
				{
					bone_array_length += vertex_bone_cnt[i];
				}

				bone_id_list = new int[bone_array_length];
				ReadChild(bone_id_list,			file, "Vertex bone id");

				bone_weight_list = new float[bone_array_length];
				ReadChild(bone_weight_list,		file, "Vertex bone weight");

				bone_vertex_list = new Vector[bone_array_length];
				ReadChild(bone_vertex_list,		file, "Vertex bone point");
		
				// UV's
				ReadChild(&texture_vertex_cnt,	file, "UV count");
				texture_vertex_list = new TexCoord[texture_vertex_cnt];
				ReadChild(texture_vertex_list,	file, "UV list");

				file->SetCurrentDirectory("..");
			}

			// Edges
			if (file->SetCurrentDirectory("Edges"))
			{
				ReadChild(&edge_cnt,	file, "Edge count");

				edges = new BezierEdge[edge_cnt];
				ReadChild(edges,	file, "Edge list");

				file->SetCurrentDirectory("..");
			}
		}

		if(result == true) // same for meshes and patches
		{
			TXMLIB->load_library(file, NULL);
			if (file->SetCurrentDirectory("Texture library"))
			{
				WIN32_FIND_DATA fd;
				char filter[] = "*";

				HANDLE search = file->FindFirstFile(filter, &fd);
				if (search != INVALID_HANDLE_VALUE)
				{
					//texture_id = TXMLIB->get_texture_id(fd.cFileName);

					file->FindClose(search);
				}

				file->SetCurrentDirectory("..");
			}

			if (file->SetCurrentDirectory("Material library"))
			{
				ReadChild(&material_cnt, file, "Material count");

				material_list = new Material[material_cnt];

				HANDLE          search;
				WIN32_FIND_DATA found;

				search = file->FindFirstFile("*.*", &found);

				if (search != INVALID_HANDLE_VALUE)
				{
					do
					{
					//
					// If this is a valid material directory (not "." or ".."), 
					// enter it and get the material's attributes
					//

						if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
						{
							continue;
						}

						if ((!strcmp(found.cFileName,".")) || 
							(!strcmp(found.cFileName,"..")))
						{
							continue;
						}

						if( file->SetCurrentDirectory( found.cFileName ) ) {

							int material_index;

							ReadChild( &material_index, file, "Material identifier" );

							ASSERT( material_index < material_cnt );

							Material *material = &material_list[material_index];
							material->initialize();
							material->set_name( found.cFileName );
							material->set_texture_library( TXMLIB );
							material->load_from_filesystem( file );

							file->SetCurrentDirectory( ".." );
						}
					}
					while (file->FindNextFile(search, &found));
				}

				file->FindClose(search);
				file->SetCurrentDirectory("..");
			}

			file->SetCurrentDirectory(".."); // get out of openFLAME or PATCH object
		}
	}

	return result;
}

