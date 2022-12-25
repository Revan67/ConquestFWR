//************************************************************************** 
//* Export.cpp	- Ascii File Exporter
//* 
//* By Christer Janson
//* Kinetix Development
//*
//* January 20, 1997 CCJ Initial coding
//*
//* This module contains the main export functions.
//*
//* Copyright (c) 1997, All Rights Reserved. 
//***************************************************************************



#include "asciiexp.h"
//#include "mapping.h"	// UVW

#ifdef _DEBUG
#pragma warning( disable : 4189 )		// local variable is initialized but not referenced
#pragma warning( disable : 4100 )		// unreferenced formal parameter
#endif
#pragma warning( error : 4701 )   // variable may be used without having been initialized
#pragma warning( error : 4700 )   // local variable used without having been initialized
#pragma warning( disable : 4100 ) // unreferenced formal parameter
#pragma warning( disable : 4189)  //  local variable is initialized but not referenced
#pragma warning( push, 4 )

/****************************************************************************

  GeomObject output
  
****************************************************************************/

#include "icq2mtl.h"


void GetNotes(INode *node, int *num_notes, Note **note_list, DefNoteTrack *nt)
{
	*num_notes = 0;
	*note_list = NULL;

/*
	while(node && !(node->IsRootNode()))
	{
		if(node->HasNoteTracks())
		{
			DefNoteTrack *nt = (DefNoteTrack*)node->GetNoteTrack(0);
*/
			int keys =  nt->NumKeys();
			for(int i=0; i < keys; i++)
			{
				TimeValue t = nt->GetKeyTime(i) / GetTicksPerFrame();

				//t -= GetStaticFrame();

				bool duplicate = false;
				for(int j=0; j < *num_notes; j++)
				{
					if((*note_list)[j].t == t)
					{
						duplicate = true;
						break;
					}
				}

				if(!duplicate)
				{
					(*num_notes)++;
					(*note_list) = (Note*)Realloc((*note_list), (*num_notes) * sizeof(Note));

					((*note_list)[(*num_notes) - 1]).t = t;
					((*note_list)[(*num_notes) - 1]).name = (char*)(nt->keys[i]->note);
					
					// strip out CR
					char *tmp_name = ((*note_list)[(*num_notes) - 1]).name;
					for(unsigned int c_id=0; c_id < strlen(tmp_name); c_id++)
					{
						if(tmp_name[c_id] == 13 || tmp_name[c_id] == 10)
						{
							tmp_name[c_id] = 0;
						}
					}
				}
			}
/*
		}
		node = GetMyObjParent(node); //node->GetParentNode();
	}
*/

	// sort by time
	qsort((*note_list), (*num_notes), sizeof(Note), CompareNotes);
}

// lowest to highest
int CompareNotes(const void *pt1, const void *pt2)
{
  if( ((Note*)pt1)->t < ((Note*)pt2)->t )
  {
    return -1;
  }
  else
  if( ((Note*)pt1)->t > ((Note*)pt2)->t )
  {
    return 1;
  }

  return 0;
}  

DefNoteTrack* GetNoteTrack(INode *node)
{
	while(node && !(node->IsRootNode()))
	{
		if(node->HasNoteTracks())
		{
			int num_notetracks = node->NumNoteTracks();
			if(num_notetracks > 0)
			{
				if(num_notetracks > 1)
				{
					Winprint("Error: only one note track per node (%s) supported\n!", node->GetName());
				}

				return (DefNoteTrack*)node->GetNoteTrack(0);
			}
		}
		else
		{
			node = GetMyObjParent(node); //node->GetParentNode();
		}
	}

	return NULL;
}

BOOL AsciiExp::Export3DB(INode* node, object *obj, const object_type mesh_type, INode *to_node,
						 const TimeValue t)
{
	if(!node) return FALSE;

	const char *name = node->GetName();

	ObjectState os ( node->EvalWorldState(t) );
	if (!os.obj)
	{
		return FALSE;
	}
	
	const SClass_ID sc_id = os.obj->SuperClassID();
	if (sc_id != GEOMOBJECT_CLASS_ID)
	{
		return FALSE; // Safety net. This shouldn't happen.
	}

	// Targets are actually geomobjects, but we will export them
	// from the camera and light objects, so we skip them here.
	const Class_ID c_id( os.obj->ClassID() );
	if (c_id == Class_ID(TARGET_CLASS_ID, 0))
	{
		return FALSE;
	}

	obj->da_mesh.type = obj->type;

	// Bezier patch object
	//ShadeContext
	//UVGen
	// interval
	// IsAnimated()
	// NumKeys()
	// GetKeyTime(int index)

	BOOL result = FALSE;

	if(c_id == Class_ID(PATCHOBJ_CLASS_ID, 0))
	{
		if(mesh_type == DEF_OBJ)
			obj->type = DEF_PATCH;
		else
			obj->type = FIXED_PATCH;

		ExportPatch(obj, node, t, to_node);

		if(obj->b_mesh.group_cnt > 0)
		{
			ExportHP(obj, node, t);
			ExportUserProperties(obj, node);

			result = TRUE;
		}
	}
	else
	if(FindNURBRefObject(node))
	{
		if(mesh_type == DEF_OBJ)
			obj->type = DEF_NURB;
		else
			obj->type = FIXED_NURB;

		ExportNURBS(obj, node, t, to_node);

		if(obj->nurb_count > 0)
		{
			ExportHP(obj, node, t);
			ExportUserProperties(obj, node);

			result = TRUE;
		}
	}
	else
	{
		if(mesh_type == DEF_OBJ)
			obj->type = DEF_MESH;
		else
			obj->type = FIXED_MESH;

		ExportMesh(obj, node, t, to_node);

		if(obj->v.object_count > 0 || obj->da_mesh.v_cnt > 0)
		{
			ExportHP(obj, node, t);
			ExportUserProperties(obj, node);
			
			result = TRUE;
		}
	}   

	if(result == FALSE)
	{
		Winprint("Warning: object %s not exported! Empty?\n", name);
		if(obj->face_count <= 0 || obj->v.object_count <= 0) // don't free objects that already had data from before
		{
			FreeObject(obj);
		}
	}

	return result;
}

char* GetStart(const char *string, const char *keys, const int key_cnt)
{
	const int len = strlen(string);
	for(int i = 0; i < len - 1; i++)
	{
		for(int j = 0; j < key_cnt; j++)
		{
			if(string[i] == keys[j])
			{
				return (char*)(string + i + 1);
			}
		}
	}

	return NULL;
}

char* GetAfterEqual(const char *string)
{
	const int len = strlen(string);
	const char *pt = string;
	for(int i = 0; i < len - 1; i++, pt++)
	{
		if(*pt == '=')
		{
			i++;
			pt++;
			while(i < len && (*pt == ' ' || *pt == '\t'))
			{
				pt++;
				i++;
			}
			break;
		}
	}

	if( i < len )
	{
		return (char*)pt;
	}
	else
	{
		return NULL;
	}
}

void AsciiExp::ExportUserProperties(object *obj, INode *node)
{
	TSTR buf;
	node->GetUserPropBuffer(buf);

	if(buf.Length() > 0)
	{
		char steps[2] = { 13, '\n'};

		char *pt = _strdup((char*)buf);

		char *token = strtok(pt, steps);

		while(token)
		{
			const int prop_id = obj->prop_hdr.propCount;
			obj->prop_hdr.Add();
			obj->prop_list = (PersistProperty2*)Realloc(obj->prop_list,
							  obj->prop_hdr.propCount * sizeof(*(obj->prop_list)));

			//const char *value_st = GetStart(token, " \t", 2);
			const char *value_st = GetAfterEqual(token);
			const char *value = NULL;
			
			if(value_st)
			{
				ReplaceBeforeEqual(token);
				//ReplaceFirstChar(token, ' ', 0);
				//ReplaceFirstChar(token, '\t', 0);

				obj->prop_list[prop_id].name = _strdup(token);
				obj->prop_list[prop_id].nameLen = strlen(obj->prop_list[prop_id].name) + 1;
				
				if( value_st[0] >= '0' && value_st[0] <= '9' && !_isnan(atof(value_st)))
				{
					if( strpbrk( value_st, "." ) )
					{
						float data_f = (float)atof(value_st);
						value = (char*)&data_f;
						obj->prop_list[prop_id].propType = PT_SINGLE;
						obj->prop_list[prop_id].data = (U8*)Malloc(sizeof(float));
						memcpy(obj->prop_list[prop_id].data, &data_f, sizeof(data_f));
						obj->prop_list[prop_id].dataLen = obj->prop_list[prop_id].nameLen + sizeof(data_f);
					}
					else
					{
						long data_l = atol(value_st);
						value = (char*)&data_l;
						obj->prop_list[prop_id].propType = PT_LONG;
						obj->prop_list[prop_id].data = (U8*)Malloc(sizeof(long));
						memcpy(obj->prop_list[prop_id].data, &data_l, sizeof(data_l));
						obj->prop_list[prop_id].dataLen = obj->prop_list[prop_id].nameLen + sizeof(data_l);
					}
				}
				else
				{
					obj->prop_list[prop_id].propType = PT_STRING;
					obj->prop_list[prop_id].data = (U8*)_strdup(value_st);
					obj->prop_list[prop_id].dataLen = obj->prop_list[prop_id].nameLen + strlen(value_st) + 1;
				}
			}
			else
			{
				obj->prop_list[prop_id].name = _strdup(token);
				obj->prop_list[prop_id].nameLen = strlen(obj->prop_list[prop_id].name) + 1;
				obj->prop_list[prop_id].propType = PT_VOID;
				obj->prop_list[prop_id].data = NULL;
				obj->prop_list[prop_id].dataLen = obj->prop_list[prop_id].nameLen;
			}

			token = strtok(NULL, steps);
		}
		Free(pt);
	}
}

void AsciiExp::ExportMesh(object *obj, INode* node, const TimeValue t, INode *to_node)
{

#ifdef _DEBUG
	const char *name = node->GetName();
	const char *to_name = to_node->GetName();
#endif

	if(node->GetCVertMode()) // vertex color flag is on
	{
		//Winprint("Please, mail this object %s to Mike S.\n", node->GetName());
	}

	ObjectState os ( node->EvalWorldState(t) );
	if (!os.obj || (os.obj->SuperClassID() != GEOMOBJECT_CLASS_ID))
	{
		return; // Sanity check. This shouldn't happen.
	}

	bool *uv_used_list = NULL;
	if(obj->type == FIXED_MESH)
	{
		CheckForUVAnim(node, &uv_used_list);
	}

	BOOL needDel;
	TriObject* tri = GetTriObjectFromNode(node, t, &needDel);
	if (!tri)
	{
		return;
	}

	Mesh* mesh = &(tri->mesh);
	const TVFace *f_uv0 = NULL, *f_uv1 = NULL;
	const Point3 *v_uv0 = NULL, *v_uv1 = NULL;
	poly p;	
	p.vertex_count = 3;
	p.api_node_id = (int)node;


	int mtl_id = -1;
	Mtl * mat = node->GetMtl();
	if(mat)
	{
		if (mat->ClassID() == CQ2Material_CLASS_ID)  // same mtl for all these faces
		{	
			mtl_id = ExportCQ2Material(mat, obj, t, node);
			GetUVChannels(mesh, &f_uv0, &f_uv1, &v_uv0, &v_uv1);	
		}
		else if (mat->ClassID() == Class_ID(DMTL_CLASS_ID, 0))  // same mtl for all these faces
		{
			Winprint("Error: OLD material type.\n");
			assert(0);
			//mtl_id = ExportMAXMaterial(mat, obj, t, node);
			//assert(mtl_id >= 0);
			//GetMtlProperty((StdMat *)mat, &(p.property));
			//GetUVChannels(mesh, &f_uv0, &f_uv1, &v_uv0, &v_uv1);
		}
		else
		if(mat->ClassID() == Class_ID(MULTI_CLASS_ID, 0))  // assign later per face
		{
			mtl_id=-1;
		}
		else // sanity check
		{
			Winprint("Error: unknown material type.\n");
			exit(1);
		}
	}
	else
	{	
		Winprint("Error: MISSING material type.\n");
		assert(0);
		//mtl_id = ExportMAXMaterial(NULL, obj, t, node);
		//assert(mtl_id >= 0);
		//p.property = p.property & ~TWO_SIDED;
	}

	const int numVx = mesh->getNumVerts();
	const int numTVx = mesh->getNumTVerts();
	const int num_colVx = mesh->getNumVertCol();

	// this fails for NURBS and Bezier patches
	if( !needDel )
	{
		assert(numVx == os.obj->NumPoints());// || os.obj->NumPoints() == 0);
	}

	
	if(num_colVx != numVx && num_colVx != 0 )//&& uv_src[0] != 2 && uv_src[1] != 2)
	{
		Winprint("Warning: Object %s number of vertices %d does not match number of COLORED vertices %d.\n",
			node->GetName(), numVx, num_colVx);
	}

	// set up transform to bone's local coord system
	Matrix3 tm ( GetMyObjTMAfterWSM(node, t) );

	if(to_node) // export in local coord (to_node and node can be the same)
	{
		Matrix3 ntm ( GetMyNodeTM(to_node, t) );
		CleanMatrix3(ntm); //ntm.NoScale(); NoShear(ntm); // this will make sure scale is applied to exported geometry

		tm *= Inverse(ntm); // same as tm = tm * Inverse(ntm); NOT tm = Inverse(ntm) * tm;
	}
	//else // export in world

	int vx1, vx2, vx3;
	if( tm.Parity() )  // negScale
	{
		vx1 = 2;
		vx2 = 1;
		vx3 = 0;
	}
	else
	{
		vx1 = 0;
		vx2 = 1;
		vx3 = 2;
	}

	bool uv_warn_flag = false;
	bool smg_warn = false;
	const int numFaces = mesh->getNumFaces();
	for(int f_id = 0; f_id < numFaces; f_id++)
	{
		p.api_face_id = f_id;

		const DWORD api_sm_group = mesh->faces[f_id].getSmGroup();
		DWORD sm_group = api_sm_group;

		// merge groups into one for better LOD results
//TODO: mark any split vert a immovable
		if(options.nsm_groups == 0 || options.nLodPercent < 100.0f)
		{
			if(sm_group > 1 && !smg_warn && options.nsm_groups == 1)
			{
				smg_warn = true;
				fprintf(stderr, "Warning: merging smoothing groups of %s into one because of LOD!\n",
					node->GetName());
			}

			p.api_smg_id = 0;
		}
		else
		{
			if(sm_group && (sm_group & (sm_group - 1)))
			{
				if(!smg_warn)
				{
					smg_warn = true;
					Winprint("Warning: at least one face of %s belongs to multiple smoothing groups!\n",
						node->GetName());
				}

				// use the lowest group
				DWORD mask = 0xffffffff >> 1;
				sm_group &= mask;
				while(sm_group && (sm_group & (sm_group - 1)))
				{
					mask >>= 1;
					sm_group &= mask;
				}
			}

			p.api_smg_id = sm_group;
		}

		if(sm_group)
		{
			p.property = p.property & ~FLAT_SHADED;
			p.property = p.property | SMOOTH_SHADED;
		}
		else
		{
			p.property = p.property & ~SMOOTH_SHADED;
			p.property = p.property | FLAT_SHADED;
		}

		if(obj->type == DEF_MESH)
		{
			if(api_sm_group == (1 << (RIGHT_EYE-1))) //23
			{
				p.flags[0] = p.flags[1] = p.flags[2] = RIGHT_EYE;
			}
			else if(api_sm_group == (1 << (LEFT_EYE-1)))//24
			{
				p.flags[0] = p.flags[1] = p.flags[2] = LEFT_EYE;
			}
			else
			{
				p.flags[0] = p.flags[1] = p.flags[2] = 0;
			}
		}
		else
		{
			p.flags[0] = p.flags[1] = p.flags[2] = 0;
		}


		// normal node
		int v_id1 = mesh->faces[f_id].v[vx1];
		Point3 p1 = tm * mesh->getVert(v_id1);//verts[v_id1];

		int v_id2 = mesh->faces[f_id].v[vx2];
		Point3 p2 = tm * mesh->getVert(v_id2);//verts[v_id2];
		
		int v_id3 = mesh->faces[f_id].v[vx3];
		Point3 p3 = tm * mesh->getVert(v_id3);//verts[v_id3];

		if(v_id1 == v_id2 || v_id2 == v_id3 || v_id3 == v_id1)
		{
			continue; // degenerate poly
		}

		// this is just so we have a copy that has the same indexing order as MAX
		if(obj->type == DEF_MESH)
		{
			p.api_xyz_id[0] = v_id1;
			p.api_xyz_id[1] = v_id2;
			p.api_xyz_id[2] = v_id3;
		}
		
		p.vertices[3*0]=p1.x;
		p.vertices[3*0+1]=p1.y;
		p.vertices[3*0+2]=p1.z;

		p.vertices[3*1]=p2.x;
		p.vertices[3*1+1]=p2.y;
		p.vertices[3*1+2]=p2.z;

		p.vertices[3*2]=p3.x;
		p.vertices[3*2+1]=p3.y;
		p.vertices[3*2+2]=p3.z;


		
		if(mtl_id!=-1) // standard mtl (for all these faces)
		{
			p.material_id = mtl_id;
		}
		else
		{			
			p.material_id = mesh->faces[f_id].getMatID();
			assert(p.material_id == mesh->getFaceMtlIndex(f_id));

			p.material_id = p.material_id % mat->NumSubMtls(); // to be safe, since our mtl id might not be same as max id	
			Mtl *sub = mat->GetSubMtl(p.material_id);

			if(sub == NULL) // this should not happen but rarely does ?!
			{
				Winprint("Error: material %s of object %s is missing sub material %d!\n",
						mat->GetName(), node->GetName(), mesh->faces[f_id].getMatID());
			}
			p.material_id = ExportCQ2Material(sub, obj, t, node);
			assert(p.material_id >= 0);
			if(sub)
			{
				//GetMtlProperty((StdMat *)sub, &(p.property));
				GetUVChannels(mesh, &f_uv0, &f_uv1, &v_uv0, &v_uv1);
			}
		}


							

		// use vertex color to mark vertices that should not move w/ LOD
		if(num_colVx > 0)//&& uv_src[0] != 2 && uv_src[1] != 2)
		{
			assert(mesh->vertCol);

			// hack for meshes that have less color vertices than regular vertices
			const int c_id1 = mesh->vcFace[f_id].t[vx1];
			assert( c_id1 < num_colVx );
			const int c_id2 = mesh->vcFace[f_id].t[vx2];
			assert( c_id2 < num_colVx );
			const int c_id3 = mesh->vcFace[f_id].t[vx3];
			assert( c_id3 < num_colVx );

			if(export_vertex_colors)
			{
				p.color[0] = (int)((mesh->vertCol[c_id1].x * 255.0f) + .4999f);
				p.color[1] = (int)((mesh->vertCol[c_id1].y * 255.0f) + .4999f);
				p.color[2] = (int)((mesh->vertCol[c_id1].z * 255.0f) + .4999f);

				p.color[3] = (int)((mesh->vertCol[c_id2].x * 255.0f) + .4999f);
				p.color[4] = (int)((mesh->vertCol[c_id2].y * 255.0f) + .4999f);
				p.color[5] = (int)((mesh->vertCol[c_id2].z * 255.0f) + .4999f);

				p.color[6] = (int)((mesh->vertCol[c_id3].x * 255.0f) + .4999f);
				p.color[7] = (int)((mesh->vertCol[c_id3].y * 255.0f) + .4999f);
				p.color[8] = (int)((mesh->vertCol[c_id3].z * 255.0f) + .4999f);
			}
			else // used by LOD
			{
				p.frozen[0] = 1.0f - mesh->vertCol[c_id1].x;
				p.frozen[1] = 1.0f - mesh->vertCol[c_id1].y;
				p.frozen[2] = 1.0f - mesh->vertCol[c_id1].z;

				p.frozen[3] = 1.0f - mesh->vertCol[c_id2].x;
				p.frozen[4] = 1.0f - mesh->vertCol[c_id2].y;
				p.frozen[5] = 1.0f - mesh->vertCol[c_id2].z;

				p.frozen[6] = 1.0f - mesh->vertCol[c_id3].x;
				p.frozen[7] = 1.0f - mesh->vertCol[c_id3].y;
				p.frozen[8] = 1.0f - mesh->vertCol[c_id3].z;
			}
			// else p.frozen[0-8] = 0.0f;
		}
		
		if( f_uv0 && v_uv0 )
		{
			GetPolyUV(f_uv0, v_uv0, f_id, vx1, vx2, vx3, uv_used_list, obj->type,
				p.uv, p.api_uv_id, (f_uv1) ? 1 : 0 );
		}
		else // non textured
		{
			p.zero_uv();
		}

		if( f_uv1 && v_uv1 )
		{
			GetPolyUV(f_uv1, v_uv1, f_id, vx1, vx2, vx3, uv_used_list, obj->type,
				p.uv2, p.api_uv_id2, 2);
		}
		else
		{
			p.zero_uv2();
		}

		//InsertPolyTriangles(obj, &p);
		InsertPoly(obj, &p);
	}

	if (needDel) 
	{
		tri->DeleteMe();
		tri = NULL;
		//delete tri;
	}

	if(uv_used_list)
	{
		ExportUVAnim(obj, node, to_node, uv_used_list);
		Free(uv_used_list);
	}
}

void AsciiExp::GetPolyUV(const TVFace *tv_face, const Point3 *tv_vert, const int f_id,
						 const int vx1, const int vx2, const int vx3,
						 const bool *uv_used_list, const object_type type, float *uv, int *api_uv_id,
						 const int channel_id)
{
	const int tv_id1 = tv_face[f_id].t[vx1];
	//assert( tv_id1 >= 0 && tv_id1 < t_count);
	uv[2*0] = tv_vert[tv_id1].x;
	uv[2*0+1] = tv_vert[tv_id1].y;

	if(type == DEF_MESH || channel_id)
	{
		api_uv_id[0] = tv_id1 | (channel_id << 24);
		//api_uv_id[0] = tv_id1;
	}
	else if(uv_used_list && uv_used_list[tv_id1])
	{
		// mark animated uv vertex as a unique uv & face combination
		// this is necessary for independent interpolation
		api_uv_id[0] = tv_id1 + (f_id << 16);
	}
	else
	{
		api_uv_id[0] = -1;
	}

	const int tv_id2 = tv_face[f_id].t[vx2];
	//assert( tv_id2 >= 0 && tv_id2 < t_count);
	uv[2*1]=tv_vert[tv_id2].x;
	uv[2*1+1]=tv_vert[tv_id2].y;

	if(type == DEF_MESH || channel_id)
	{
		api_uv_id[1] = tv_id2 | (channel_id << 24);
		//api_uv_id[1] = tv_id2;
	}
	else if(uv_used_list && uv_used_list[tv_id2])
	{
		api_uv_id[1] = tv_id2 + (f_id << 16);
	}
	else
	{
		api_uv_id[1] = -1;
	}

	const int tv_id3 = tv_face[f_id].t[vx3];
	//assert( tv_id3 >= 0 && tv_id3 < t_count);
	uv[2*2]=tv_vert[tv_id3].x;
	uv[2*2+1]=tv_vert[tv_id3].y;

	if(type == DEF_MESH || channel_id)
	{
		api_uv_id[2] = tv_id3 | (channel_id << 24);
		//api_uv_id[2] = tv_id3;
	}
	else if(uv_used_list && uv_used_list[tv_id3])
	{
		api_uv_id[2] = tv_id3 + (f_id << 16);
	}
	else
	{
		api_uv_id[2] = -1;
	}

	//assert(tv_id1 != tv_id2 && tv_id2 != tv_id3 && tv_id3 != tv_id1);
}


// test function from kinetix for a bug in GetCLPTextureSurfaceData()
/*
void GetChordLengthData(INode *node, Interface *mpIp)
{
	char name[256] = {0};
	strncpy(name, node->GetName(), 255);
	
	Object *obj = FindNURBRefObject(node);

	NURBSSet nset;

	GetNURBSSet(obj, mpIp->GetTime(), nset, FALSE);
    
	int numobj = nset.GetNumObjects();
	    
    for (int i=0;i<numobj;i++)
	{
		NURBSObject *Nobj1 = nset.GetNURBSObject (i);
		if (Nobj1->GetType() == kNCVSurface || Nobj1->GetType() == kNPointSurface) 
		{

			for (int q=0;q<2;q++)
			{
				int channel=q, degreeInU, degreeInV, numInU, numInV, numKnotsInU, numKnotsInV;
				NURBSCVTab cvs;
				NURBSKnotTab uKnots,vKnots;
				  

				TimeValue t=mpIp->GetTime();;

				NURBSSurface* surf = (NURBSSurface*) Nobj1;
			     
				if (surf->GetCLPTextureSurfaceData(t, channel,degreeInU,degreeInV,numInU,numInV,cvs,
					numKnotsInU, numKnotsInV, uKnots, vKnots) )
					{
					  DebugPrint ("Success Channel %i \n",channel);
					  for (int i=0;i<cvs.Count();i++)
					  {
							NURBSControlVertex *cv = (NURBSControlVertex *) cvs.Addr(i);
							Point3 pt = cv->GetPosition(mpIp->GetTime());
							DebugPrint ("%i: %g %g %g \n",i,pt.x,pt.y,pt.z);
					  }

					  
					  NURBSTextureSurface pTexSurf = surf->mTexSurface[channel];

					  if (! pTexSurf.IsCLP())
					  {
					    int numU = pTexSurf.GetNumUCVs();
					    int numV = pTexSurf.GetNumVCVs();
				    
					    DebugPrint ("now the cvs from the texture surface\n");
				        for (int u = 0; u < numU; u++)
					       for (int v = 0; v < numV; v++)
						   {	
						     NURBSControlVertex *cv = pTexSurf.GetCV((u,v),mpIp->GetTime());
							 double x,y,z;
							 cv->GetPosition (t,x,y,z);
							 //Point3 pt = cv->GetPosition(mpIp->GetTime());
							 Point3 pt = Point3 (x,y,z);
							 DebugPrint ("%i: %g %g %g \n",u,pt.x,pt.y,pt.z);
						   }
					  }
						
					}
				    else
					{
					    DebugPrint ("Error \n");
					}
				}
		  }
	}
}

*/

void FlipPatch( Bezier_patch *p, Vector *aux )
{
	switch(p->type)
	{
		case PATCH_QUAD:
		{
			Swap32( p->v + 0, p->v + 1 );
			Swap32( p->v + 2, p->v + 3 );

			Swap32( p->vec + 0, p->vec + 1 );
			Swap32( p->vec + 2, p->vec + 7 );
			Swap32( p->vec + 3, p->vec + 6 );
			Swap32( p->vec + 4, p->vec + 5 );

			Swap32( p->interior + 0, p->interior + 1 );
			Swap32( p->interior + 2, p->interior + 3 );

			Swap32( p->tv + 0, p->tv + 1 );
			Swap32( p->tv + 2, p->tv + 3 );
			break;
		}
		case PATCH_TRI:
		{
			Swap32( p->v + 1, p->v + 2 );

			Swap32( p->vec + 0, p->vec + 5 );
			Swap32( p->vec + 1, p->vec + 4 );
			Swap32( p->vec + 2, p->vec + 3 );

			Swap32( p->interior + 1, p->interior + 2 );

			Swap32( p->tv + 1, p->tv + 2 );

			MemSwap( aux + 0, aux + 8, sizeof(Vector) );
			MemSwap( aux + 1, aux + 7, sizeof(Vector) );
			MemSwap( aux + 2, aux + 6, sizeof(Vector) );
			MemSwap( aux + 3, aux + 5, sizeof(Vector) );
			break;
		}
		default:
			Winprint("Error: FlipPatch()\n");
	}
}

void FixPatchEdge(const Point3 & p1, const Point3 & p90, const Point3 & pdir, Point3 & dest)
{
	Point3 dir_v ( pdir - p1 );
	dir_v.Normalize();

	float length;// = (p90 - p1).Length();
	//if( length < .0001f )
	{
		length = .25f * (pdir - p1).Length();
	}

	dest = p1 + ( dir_v * length );

	assert( (dest - p1).LengthSquared() > .001f );
}

void AsciiExp::ExportPatch(object *obj, INode* node, TimeValue t, INode *to_node)
{
	const char *name = node->GetName();
	const char *to_name = ( to_node ) ? to_node->GetName() : NULL;

	Matrix3 tm ( GetMyObjTMAfterWSM(node, t) );

	if(to_node) // export in local coord (to_node and node can be the same)
	{
		Matrix3 ntm ( GetMyNodeTM(to_node, t) );
		CleanMatrix3(ntm); //ntm.NoScale(); NoShear(ntm); // this will make sure scale is applied to exported geometry

		tm *= Inverse(ntm); // same as tm = tm * Inverse(ntm); NOT tm = Inverse(ntm) * tm;
	}
	//else // export in world

	const BOOL flip_normals = tm.Parity();

	int deleteIt;
	PatchObject *po = GetPatchObjectFromNode(node, t, &deleteIt);

	//EnumModifiers( node, DumpParam2Block, NULL );
	//DumpParam2Block( po, NULL );

	PatchMesh & patch_mesh = po->patch;

#if 0 // check vectors are non zero length
// TODO: fix in 10% of the direction of the other handle on the same edge
	for(int ti = 0; ti < patch_mesh.numPatches; ti++)
	{
		Patch & patch = patch_mesh.patches[ti];
		float l0, l1, l2, l3, l4, l5, l6, l7;

		const float min_d = .0001f;
//p 90 dir dest
		if(patch.type == PATCH_QUAD)
		{
			l0 = ( patch_mesh.vecs[ patch.vec[7] ].p - patch_mesh.verts[ patch.v[0] ].p ).LengthSquared();
			if( l0 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[0] ].p,
							  patch_mesh.vecs[ patch.vec[0] ].p,
							  patch_mesh.verts[ patch.v[3] ].p,
							  patch_mesh.vecs[ patch.vec[7] ].p );
				
			l1 = ( patch_mesh.vecs[ patch.vec[0] ].p - patch_mesh.verts[ patch.v[0] ].p ).LengthSquared();
			if( l1 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[0] ].p,
							  patch_mesh.vecs[ patch.vec[7] ].p,
							  patch_mesh.verts[ patch.v[1] ].p,
							  patch_mesh.vecs[ patch.vec[0] ].p );

			l2 = ( patch_mesh.vecs[ patch.vec[1] ].p - patch_mesh.verts[ patch.v[1] ].p ).LengthSquared();
			if( l2 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[1] ].p,
							  patch_mesh.vecs[ patch.vec[2] ].p,
							  patch_mesh.verts[ patch.v[0] ].p,
							  patch_mesh.vecs[ patch.vec[1] ].p );

			l3 = ( patch_mesh.vecs[ patch.vec[2] ].p - patch_mesh.verts[ patch.v[1] ].p ).LengthSquared();
			if( l3 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[1] ].p,
							  patch_mesh.vecs[ patch.vec[1] ].p,
							  patch_mesh.verts[ patch.v[2] ].p,
							  patch_mesh.vecs[ patch.vec[2] ].p );

			l4 = ( patch_mesh.vecs[ patch.vec[3] ].p - patch_mesh.verts[ patch.v[2] ].p ).LengthSquared();
			if( l4 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[2] ].p,
							  patch_mesh.vecs[ patch.vec[4] ].p,
							  patch_mesh.verts[ patch.v[1] ].p,
							  patch_mesh.vecs[ patch.vec[3] ].p );

			l5 = ( patch_mesh.vecs[ patch.vec[4] ].p - patch_mesh.verts[ patch.v[2] ].p ).LengthSquared();
			if( l5 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[2] ].p,
							  patch_mesh.vecs[ patch.vec[3] ].p,
							  patch_mesh.verts[ patch.v[3] ].p,
							  patch_mesh.vecs[ patch.vec[4] ].p );

			l6 = ( patch_mesh.vecs[ patch.vec[5] ].p - patch_mesh.verts[ patch.v[3] ].p ).LengthSquared();
			if( l6 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[3] ].p,
							  patch_mesh.vecs[ patch.vec[6] ].p,
							  patch_mesh.verts[ patch.v[2] ].p,
							  patch_mesh.vecs[ patch.vec[5] ].p );

			l7 = ( patch_mesh.vecs[ patch.vec[6] ].p - patch_mesh.verts[ patch.v[3] ].p ).LengthSquared();
			if( l7 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[3] ].p,
							  patch_mesh.vecs[ patch.vec[5] ].p,
							  patch_mesh.verts[ patch.v[0] ].p,
							  patch_mesh.vecs[ patch.vec[6] ].p );
		}else
		if(patch.type == PATCH_TRI)
		{
			l0 = ( patch_mesh.vecs[ patch.vec[5] ].p - patch_mesh.verts[ patch.v[0] ].p ).LengthSquared();
			if( l0 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[0] ].p,
							  patch_mesh.vecs[ patch.vec[0] ].p,
							  patch_mesh.verts[ patch.v[2] ].p,
							  patch_mesh.vecs[ patch.vec[5] ].p );

			l1 = ( patch_mesh.vecs[ patch.vec[0] ].p - patch_mesh.verts[ patch.v[0] ].p ).LengthSquared();
			if( l1 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[0] ].p,
							  patch_mesh.vecs[ patch.vec[5] ].p,
							  patch_mesh.verts[ patch.v[1] ].p,
							  patch_mesh.vecs[ patch.vec[9] ].p );

			l2 = ( patch_mesh.vecs[ patch.vec[1] ].p - patch_mesh.verts[ patch.v[1] ].p ).LengthSquared();
			if( l2 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[1] ].p,
							  patch_mesh.vecs[ patch.vec[2] ].p,
							  patch_mesh.verts[ patch.v[0] ].p,
							  patch_mesh.vecs[ patch.vec[1] ].p );

			l3 = ( patch_mesh.vecs[ patch.vec[2] ].p - patch_mesh.verts[ patch.v[1] ].p ).LengthSquared();
			if( l3 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[1] ].p,
							  patch_mesh.vecs[ patch.vec[1] ].p,
							  patch_mesh.verts[ patch.v[2] ].p,
							  patch_mesh.vecs[ patch.vec[2] ].p );

			l4 = ( patch_mesh.vecs[ patch.vec[3] ].p - patch_mesh.verts[ patch.v[2] ].p ).LengthSquared();
			if( l4 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[2] ].p,
							  patch_mesh.vecs[ patch.vec[4] ].p,
							  patch_mesh.verts[ patch.v[1] ].p,
							  patch_mesh.vecs[ patch.vec[3] ].p );

			l5 = ( patch_mesh.vecs[ patch.vec[4] ].p - patch_mesh.verts[ patch.v[2] ].p ).LengthSquared();
			if( l5 < min_d )
				FixPatchEdge( patch_mesh.verts[ patch.v[2] ].p,
							  patch_mesh.vecs[ patch.vec[3] ].p,
							  patch_mesh.verts[ patch.v[0] ].p,
							  patch_mesh.vecs[ patch.vec[4] ].p );

			l6 = l7 = 1.0f;
		}
			
		if( l0 < min_d ||
			l1 < min_d ||
			l2 < min_d ||
			l3 < min_d ||
			l4 < min_d ||
			l5 < min_d ||
			l6 < min_d ||
			l7 < min_d )
		{
			patch_mesh.patchSel.Set(ti, 1);
		}
	}
	patch_mesh.computeAux();
#endif

	Bezier_mesh & b_mesh = obj->b_mesh;

	// keep track of the node this data comes from so to be resolved later against Physique
	b_mesh.api_node_count++;
	b_mesh.api_node_list = (int*)Realloc(b_mesh.api_node_list, b_mesh.api_node_count * sizeof(int));
	b_mesh.api_node_list[b_mesh.api_node_count-1] = (int)node;

	b_mesh.api_node_ver_offset = (int*)Realloc(b_mesh.api_node_ver_offset, b_mesh.api_node_count * sizeof(int));
	b_mesh.api_node_ver_offset[b_mesh.api_node_count-1] = b_mesh.vertex_cnt;

	b_mesh.api_node_vec_offset = (int*)Realloc(b_mesh.api_node_vec_offset, b_mesh.api_node_count * sizeof(int));
	b_mesh.api_node_vec_offset[b_mesh.api_node_count-1] = b_mesh.vector_cnt;

	b_mesh.api_node_aux_offset = (int*)Realloc(b_mesh.api_node_aux_offset, b_mesh.api_node_count * sizeof(int));
	b_mesh.api_node_aux_offset[b_mesh.api_node_count-1] = b_mesh.aux_cnt;

	// VERTICES
	const int old_ver_cnt = b_mesh.vertex_cnt;
	b_mesh.vertex_cnt += patch_mesh.numVerts;
	b_mesh.vertices = (Vector*)Realloc(b_mesh.vertices, b_mesh.vertex_cnt * sizeof(Vector));
	for(int i = 0; i < patch_mesh.numVerts; i++)
	{
		const Point3 p3 ( tm * patch_mesh.verts[i].p );
		b_mesh.vertices[old_ver_cnt+i].x = p3.x;
		b_mesh.vertices[old_ver_cnt+i].y = p3.y;
		b_mesh.vertices[old_ver_cnt+i].z = p3.z;
	}

	// VECTORS
	const int old_vec_cnt = b_mesh.vector_cnt;
	b_mesh.vector_cnt += patch_mesh.numVecs;
	b_mesh.vectors = (Vector*)Realloc(b_mesh.vectors, b_mesh.vector_cnt * sizeof(Vector));
	for(i = 0; i < patch_mesh.numVecs; i++)
	{
		const Point3 p3 ( tm * patch_mesh.vecs[i].p );
		b_mesh.vectors[old_vec_cnt+i].x = p3.x;
		b_mesh.vectors[old_vec_cnt+i].y = p3.y;
		b_mesh.vectors[old_vec_cnt+i].z = p3.z;
	}

	// UV's

	if( b_mesh.uv_cnt == 0 ) // give unmapped patches some data to index into
	{
		assert( b_mesh.uvs == NULL );
		b_mesh.uv_cnt = 1;
		b_mesh.uvs = (TexCoord*)Malloc(sizeof(TexCoord));
		b_mesh.uvs[0].u =
		b_mesh.uvs[0].v = .5f;
	}

	const int old_uv_cnt = b_mesh.uv_cnt;
	const int channel_id = 0;
	bool has_uv = (patch_mesh.getNumTVertsChannel(channel_id) > 0);
	if( has_uv )
	{
		b_mesh.uv_cnt += patch_mesh.getNumTVertsChannel(channel_id);
		b_mesh.uvs = (TexCoord*)Realloc(b_mesh.uvs, b_mesh.uv_cnt * sizeof(TexCoord));
		for(i = 0; i < b_mesh.uv_cnt - old_uv_cnt; i++)
		{
			const UVVert & uvv = patch_mesh.getTVertChannel(channel_id, i);
			b_mesh.uvs[old_uv_cnt+i].u = uvv.x;
			b_mesh.uvs[old_uv_cnt+i].v = uvv.y;
		}
	}

	// edges
	// don't know what group each patch will fall into
	// have to build edges manually at the end
#if 0
	const int old_edge_cnt = b_mesh.edge_cnt;
	b_mesh.edge_cnt += patch_mesh.numEdges;
	b_mesh.edges = (Bezier_edge*)Realloc(b_mesh.edges, b_mesh.edge_cnt * sizeof(Bezier_edge));
	for(i = 0; i < patch_mesh.numEdges; i++)
	{
		PatchEdge & p_edge = patch_mesh.edges[i];
		Bezier_edge & bm_edge = b_mesh.edges[old_edge_cnt + i];

		bm_edge.patch1 = b_mesh.patch_cnt + p_edge.patch1;
		bm_edge.patch2 = b_mesh.patch_cnt + p_edge.patch2;

		bm_edge.v1 = old_ver_cnt + p_edge.v1;
		bm_edge.v2 = old_ver_cnt + p_edge.v2;
	}
#endif

	Bezier_patch p;
	p.Init(); // -1

	//MtlID mtl_id = patch_mesh.getMtlIndex();

	int mtl_id;
	Mtl *mat = node->GetMtl();
	if(mat && mat->ClassID() == Class_ID(DMTL_CLASS_ID, 0))
	{
		mtl_id = -1;
		Winprint("Error: OLD material type.\n");
		assert(0);
		//mtl_id = ExportMAXMaterial(mat, obj, t, node);
		//assert(mtl_id >= 0);
		//GetMtlProperty((StdMat *)mat, &(p.property));
	}
	else
	{
		mtl_id = -1;
		assert(!mat || mat->ClassID() == Class_ID(MULTI_CLASS_ID, 0));
	}

	// PATCHES
	Vector aux9[9];
	for(i = 0; i < patch_mesh.numPatches; i++)
	{
		Patch & patch = patch_mesh.patches[i];
		TVPatch & tv_patch = patch_mesh.getTVPatchChannel(channel_id, i);
		//int tv_patch_cnt = patch_mesh.tvPatches.Count();
		Vector aux[9];

		switch(patch.type)
		{
			case PATCH_QUAD:
			{
				p.type = 4;
				for(int j = 0; j < 4; j++)
				{
					p.v[j] = patch.v[j] + old_ver_cnt;
				}

				for(j = 0; j < 8; j++)
				{
					p.vec[j] = patch.vec[j] + old_vec_cnt;
				}

				for(j = 0; j < 4; j++)
				{
					p.interior[j] = patch.interior[j] + old_vec_cnt;
				}
			
				//UV indices
				for(j = 0; j < 4; j++)
				{
					if( has_uv )
					{
						p.tv[j] = tv_patch.getTVert(j) + old_uv_cnt;
						assert( p.tv[j] >= old_uv_cnt && p.tv[j] < b_mesh.uv_cnt );
					}
					else
					{
						p.tv[j] = 0;
					}
				}
				break;
			}
			case PATCH_TRI:
			{
				p.type = 3;
				for(int j = 0; j < 3; j++)
				{
					p.v[j] = patch.v[j] + old_ver_cnt;
				}
				p.v[3] = -1;

				for(j = 0; j < 6; j++)
				{
					p.vec[j] = patch.vec[j] + old_vec_cnt;
				}
				p.vec[6] = p.vec[7] = -1;

				for(j = 0; j < 3; j++)
				{
					p.interior[j] = patch.interior[j] + old_vec_cnt;
				}
				p.interior[3] = -1;
			
				//UV indices
				for(j = 0; j < 3; j++)
				{
					if( has_uv )
					{
						p.tv[j] = tv_patch.getTVert(j) + old_uv_cnt;
						assert( p.tv[j] >= old_uv_cnt && p.tv[j] < b_mesh.uv_cnt );
					}
					else
					{
						p.tv[j] = 0;
					}
				}
				p.tv[3] = -1;
				
				
				for(j = 0; j < 9; j++)
				{
					const Point3 p3 ( tm * patch.aux[j] );
					aux[j].x = p3.x;
					aux[j].y = p3.y;
					aux[j].z = p3.z;
				}
				break;
			}
			default:
			{
				Winprint("Error: patch %d in %s is of unknown type!\n", i, name);
			}
		}

		int in_mtl;
		if(mtl_id == -1)
		{
			Mtl *sub_mtl = NULL;
			if(mat)
			{
				sub_mtl = mat->GetSubMtl( patch.getMatID() % mat->NumSubMtls() );
			}

			Winprint("Error: NO material type.\n");
			assert(0);
			//in_mtl = ExportMAXMaterial(sub_mtl, obj, t, node);
			in_mtl = -1;
		}
		else
		{
			in_mtl = mtl_id;
		}

		if( flip_normals )
		{
			FlipPatch( &p, aux );
		}
		b_mesh.InsertPatch( p, in_mtl, i, aux);
	}

	if(deleteIt)
	{
		delete po;
	}
}

// indices from 0 to n
void idx_remap(const int n1, const int n2, const int i1, int *i2, float *fraction)
{
	const float ii = (float)(i1 * n2) / (float)n1;

	*i2 = (int)floor(ii);
	*fraction = ii - *i2;
}

void AsciiExp::ExportNURBS(object *obj, INode* node, TimeValue t, INode *to_node)
{
	const char *name = node->GetName();
	const char *to_name = ( to_node ) ? to_node->GetName() : NULL;
	
	ObjectState os = node->EvalWorldState(t);
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID)
	{
		return; // Safety net. This shouldn't happen.
	}

	// set up transform to bone's local coord system
	Matrix3 tm ( GetMyObjTMAfterWSM(node, t) );
	if(to_node)
	{
		Matrix3 ntm ( GetMyNodeTM(to_node, t) );
		CleanMatrix3(ntm); //ntm.NoScale();  NoShear(ntm); // this will make sure scale is applied to exported geometry
		tm *= Inverse(ntm);
	}
	//else // world

	
	//if( tm.Parity() ) { // flip normals ?}
	
	int mtl_id;
	Mtl *mat = node->GetMtl();
	if(mat && mat->ClassID() == Class_ID(DMTL_CLASS_ID, 0))
	{
		mtl_id = -1;
		Winprint("Error: OLD material type.\n");
		assert(0);
		//mtl_id = ExportMAXMaterial(mat, obj, t, node);
		//assert(mtl_id >= 0);
		//GetMtlProperty((StdMat *)mat, &(p.property));
	}
	else
	{
		mtl_id = -1;
		assert(!mat || mat->ClassID() == Class_ID(MULTI_CLASS_ID, 0));
	}

	// Get the object reference of the node
	Object* ref_obj = FindNURBRefObject(node);


	// if this is true you get surfaces as blends, fillets, etc.
	// with it false, you always get base NURBSCVSurface's
	BOOL	relational;
	if(obj->type == DEF_NURB)
	{
		relational = TRUE;
	}
	else
	{
		relational = FALSE;
	}


	int node_cv_total = 0;
	int first_nurb_id = obj->nurb_count;

	NURBSSet NURBS_set;
	if( GetNURBSSet(ref_obj, t, NURBS_set, relational) )
	{
		const int num_objects = NURBS_set.GetNumObjects();

		for( int surf = 0; surf < num_objects; surf++ )
		{
			NURBSObject *nObj = NURBS_set.GetNURBSObject(surf);

			const char *surf_name = nObj->GetName();

			if(nObj->GetKind() == kNURBSCurve && nObj->GetType() == kNCVCurve)
			{
				NURBSCVCurve *curve = (NURBSCVCurve*)nObj;
				node_cv_total += curve->GetNumCVs();
				continue;
			}


			if(nObj->GetKind() != kNURBSSurface) // we don't want curves and points
			{
				continue;
			}

			if(nObj->GetType() != kNCVSurface) // kNPointSurface
			{
				Winprint("Error: can't export a non CV surface %s %s as a deformable NURB!\n",
					name, surf_name);

				continue;
			}
			
			obj->nurb_count++;
			obj->nurb_list = (nurb*)Realloc(obj->nurb_list, obj->nurb_count * sizeof(nurb));
			nurb *nr = obj->nurb_list + obj->nurb_count - 1;
			InitNurb(nr);

			nr->name = _strdup(surf_name);
	
			// NURBSPointSurface
			// we're always asking for NURBSCVSurface's, not relational surfaces
			NURBSCVSurface *surface = (NURBSCVSurface*)nObj; nObj = NULL;

			nr->api_st_offset = node_cv_total;
			node_cv_total += surface->GetNumUCVs() * surface->GetNumVCVs();

			if(mtl_id >= 0)
			{
				nr->mtl_id = mtl_id;
			}
			else
			{
				Mtl *sub_mtl = NULL;
				if(mat)
				{
					int num_submtls = mat->NumSubMtls();
					int	sub_material_ID = surface->MatID();

#if 1 //MAX_RELEASE == 2500
													 // this is hacked to fix a MAX bug
					sub_material_ID = (sub_material_ID + (num_submtls - 1)) % num_submtls; 
#else
					sub_material_ID = sub_material_ID % num_submtls;
#endif
					sub_mtl = mat->GetSubMtl(sub_material_ID);
				}

				Winprint("Error: OLD material type.\n");
				assert(0);
				//nr->mtl_id = ExportMAXMaterial(sub_mtl, obj, t, node);
				//GetMtlProperty(sub_mtl, &(p.property));
			}

			//Matrix3 surf_tm ( surface->GetTransformMatrix(t) );
			BOOL flip_normals = surface->FlipNormals();
			nr->s_closed = (surface->IsClosedInU() == TRUE) ? true : false;
			nr->t_closed = (surface->IsClosedInV() == TRUE) ? true : false;

			nr->s_order = surface->GetUOrder(); // order 4 = degree 3 = cubic
			nr->t_order = surface->GetVOrder(); // order 3 = degree2 = quadratic
			if(nr->s_order > 4 || nr->s_order < 2 ||
			   nr->t_order > 4 || nr->t_order < 2)
			{
				Winprint("Error: in %s %s order %dx%d NURBS not yet supported!\n",
					name, surf_name, nr->s_order, nr->t_order);
			}

			nr->s_knot_count = surface->GetNumUKnots(); // min 8 (00001111) (typical 10 (0000123333)
			nr->t_knot_count = surface->GetNumVKnots();
			assert(nr->s_knot_count >= 2 * nr->s_order);
			assert(nr->t_knot_count >= 2 * nr->t_order);
			
			nr->s_point_count = surface->GetNumUCVs(); // min == order
			nr->t_point_count = surface->GetNumVCVs();
			assert(nr->s_point_count >= nr->s_order);
			assert(nr->t_point_count >= nr->t_order);

			assert(nr->s_knot_count - nr->s_order == nr->s_point_count);
			assert(nr->t_knot_count - nr->t_order == nr->t_point_count);


			// NOTE: u_patch_count = (number_u_cvs - u_order + 1)

			nr->s_knot_list = (float*)Malloc(nr->s_knot_count * sizeof(float));
			nr->t_knot_list = (float*)Malloc(nr->t_knot_count * sizeof(float));
			nr->point_list = (float*)Malloc(3 * nr->s_point_count * nr->t_point_count * sizeof(float));
			nr->weight_list = (float*)Malloc(nr->s_point_count * nr->t_point_count * sizeof(float));
			nr->api_s_id = (int*)Malloc(nr->s_point_count * nr->t_point_count * sizeof(int));
			nr->api_t_id = (int*)Malloc(nr->s_point_count * nr->t_point_count * sizeof(int));
			nr->api_node_id = (int)node;

			
			// u knots
			nr->s_knot_list[0] = (float)surface->GetUKnot(0);
			for ( int knot = 1; knot < nr->s_knot_count; knot++ )
			{
				nr->s_knot_list[knot] = (float)surface->GetUKnot(knot);
				assert(nr->s_knot_list[knot-1] <= nr->s_knot_list[knot]);
			}

			// v knots
			if(flip_normals)
			{
				nr->t_knot_list[0] = -(float)surface->GetVKnot(nr->t_knot_count - 1);
				for ( knot = 1; knot < nr->t_knot_count; knot++ )
				{
					nr->t_knot_list[knot] = -(float)surface->GetVKnot(nr->t_knot_count - (knot+1));
					assert(nr->t_knot_list[knot-1] <= nr->t_knot_list[knot]);
				}
			}
			else
			{
				nr->t_knot_list[0] = (float)surface->GetVKnot(0);
				for ( knot = 1; knot < nr->t_knot_count; knot++ )
				{
					nr->t_knot_list[knot] = (float)surface->GetVKnot(knot);
					assert(nr->t_knot_list[knot-1] <= nr->t_knot_list[knot]);
				}
			}


			// clean knots
			for( knot = 1; knot < nr->s_knot_count; knot++ )
			{
				if( nr->s_knot_list[knot] - nr->s_knot_list[knot-1] < .0001)
				{
					nr->s_knot_list[knot] = nr->s_knot_list[knot-1];
				}
			}
			for( knot = 1; knot < nr->t_knot_count; knot++ )
			{
				if( nr->t_knot_list[knot] - nr->t_knot_list[knot-1] < .0001)
				{
					nr->t_knot_list[knot] = nr->t_knot_list[knot-1];
				}
			}

			nr->s_basis_cnt = nr->s_point_count - (nr->s_order - 1);
			nr->t_basis_cnt = nr->t_point_count - (nr->t_order - 1);
			// drop basis for each repeated knot
			for(int sk = nr->s_order-1; sk < nr->s_knot_count - nr->s_order; sk++)
			{
				if(nr->s_knot_list[sk] == nr->s_knot_list[sk+1])
				{
					nr->s_basis_cnt--;
				}
			}

			for(int tk = nr->t_order-1; tk < nr->t_knot_count - nr->t_order; tk++)
			{
				if(nr->t_knot_list[tk] == nr->t_knot_list[tk+1])
				{
					nr->t_basis_cnt--;
				}
			}
		

			// st ctrl points
			for ( int u = 0; u < nr->s_point_count; u++ )
			{
				for ( int v = 0; v < nr->t_point_count; v++ )
				{
					NURBSControlVertex*	cv;
					if(flip_normals)
					{
						cv = surface->GetCV(u, nr->t_point_count - (v+1));
						nr->api_s_id[u * nr->t_point_count + v] = u;
						nr->api_t_id[u * nr->t_point_count + v] = nr->t_point_count - (v+1);
					}
					else
					{
						cv = surface->GetCV(u, v);
						nr->api_s_id[u * nr->t_point_count + v] = u;
						nr->api_t_id[u * nr->t_point_count + v] = v;
					}

					double	w;
					cv->GetWeight(t, w);
					nr->weight_list[u * nr->t_point_count + v] = (float)w;

					Point3	pos = tm * cv->GetPosition(t);
					nr->point_list[3*(u * nr->t_point_count + v)  ] = pos.x;
					nr->point_list[3*(u * nr->t_point_count + v)+1] = pos.y;
					nr->point_list[3*(u * nr->t_point_count + v)+2] = pos.z;
				}
			}

			// toss weights if all constant
			bool const_w = true;
#if 0		// MAX currently ignores w so we do too
			for(int i=0; i < nr->s_point_count * nr->t_point_count; i++)
			{
				if( fabs(nr->weight_list[i] - nr->weight_list[0]) >= 0.0001f )
				{
					const_w = false;
					break;
				}
			}
#endif
			if(const_w )
			{
				/* constant weights other than 1.0
				if( fabs(nr->weight_list[0] - 1.0f) > 0.0001f )
				{
					Winprint("Error: this NURB %s %s has constant weights of %f !\n"
							 "Please contact Mike S.\n",
						name, surf_name, nr->weight_list[0]);

				}
				*/
				Free(nr->weight_list);
			}
			
#if MAX_RELEASE >= 3000
			
			const int num_ch = surface->NumChannels();
			if(num_ch > 1)
			{
				Winprint("Warning: NURBS %s %s w/ more than 1 UV channel are not yet supported!\n",
					name, surf_name);
			}

			//int ch_i = (num_ch) ? surface->GetChannelFromIndex(0) : -1;
			NURBSTextureSurface & t_surface = surface->GetTextureSurface( 0 );
			
			NURBSTexSurfType ts_type = t_surface.MapperType();

			if(ts_type != kNMapUserDefined)
			{
				Winprint("Error: %s %s has unsupported UV mapping.\n"
						 "Converting to UserDefined.\n", name, surf_name);

				ts_type = kNMapUserDefined;
				t_surface.SetMapperType(ts_type);
				assert(ts_type == t_surface.MapperType());
			}
			
			switch ( ts_type )
			{
				case kNMapUserDefined:
				{
					const int num_Upoints = t_surface.GetNumUPoints();
					const int num_Vpoints = t_surface.GetNumVPoints();

					nr->s_uv_count = nr->s_basis_cnt + 1;
					nr->t_uv_count = nr->t_basis_cnt + 1;

					nr->uv_list = (float*)Malloc(2 * nr->s_uv_count * nr->t_uv_count * sizeof(float));
					float uu[4], vv[4];
					NURBSTexturePoint *ntp;
					for(int u = 0; u < nr->s_uv_count; u++)
					{
						float u_fraction, om_u_fraction;
						int ss;
						idx_remap(nr->s_uv_count - 1, num_Upoints - 1,
								  u, &ss, &u_fraction);
						om_u_fraction = 1.0f - u_fraction;

						if(u == nr->s_uv_count - 1 )
						{
							assert( u_fraction == 0.0f );
						}
						
						for(int v = 0; v < nr->t_uv_count; v++)
						{
							int flip_v = (!flip_normals) ? v : nr->t_uv_count - (v + 1);
							float v_fraction, om_v_fraction;
							int tt;
							idx_remap(nr->t_uv_count - 1, num_Vpoints - 1,
									  flip_v, &tt, &v_fraction);

							if(!flip_normals)
							{
								om_v_fraction = 1.0f - v_fraction;
								if(v == nr->t_uv_count - 1 )
								{
									assert( v_fraction == 0.0f );
								}
							}
							else
							{
								om_v_fraction = v_fraction;
								v_fraction = 1.0f - om_v_fraction;
								if(v == nr->t_uv_count - 1 )
								{
									assert( v_fraction == 1.0f );
								}
							}

							ntp = t_surface.GetPoint(ss, tt);
							ntp->GetPosition(t, uu[0], vv[0]);

							ntp = t_surface.GetPoint(((ss+1) % num_Upoints), tt);
							ntp->GetPosition(t, uu[1], vv[1]);

							ntp = t_surface.GetPoint(ss, ((tt+1) % num_Vpoints));
							ntp->GetPosition(t, uu[2], vv[2]);

							ntp = t_surface.GetPoint(((ss+1) % num_Upoints), ((tt+1) % num_Vpoints));
							ntp->GetPosition(t, uu[3], vv[3]);

							nr->uv_list[2*(u * nr->t_uv_count + v)] =
								om_v_fraction * (om_u_fraction * uu[0] + u_fraction * uu[1]) +
								v_fraction * (om_u_fraction * uu[2] + u_fraction * uu[3]);

							nr->uv_list[2*(u * nr->t_uv_count + v)+1] =
								om_v_fraction * (om_u_fraction * vv[0] + u_fraction * vv[1]) +
								v_fraction * (om_u_fraction * vv[2] + u_fraction * vv[3]);
						}
					}		
				}
				break;
				case kNMapDefault:
				{
					int deg_u, deg_v, num_u, num_v, num_ku, num_kv;
					NURBSCVTab cvs; // typedef Tab<NURBSControlVertex> NURBSCVTab;
					//cvs.ZeroCount();
					NURBSKnotTab uKnots, vKnots; // typedef Tab<double> NURBSKnotTab;

					if( surface->GetCLPTextureSurfaceData
						(t, 0, deg_u, deg_v, num_u, num_v, cvs, num_ku, num_kv, uKnots, vKnots) )
					{
						assert(deg_u == nr->s_order - 1);
						assert(deg_v == nr->t_order - 1);
						assert(num_u == nr->s_point_count);
						assert(num_v == nr->t_point_count);
						assert(num_ku == nr->s_knot_count);
						assert(num_kv == nr->t_knot_count);
						assert(cvs.Count() == nr->s_point_count * nr->t_point_count);

						const int expected_u_cnt = (surface->GetNumUCVs() - surface->GetUOrder()) + 2;
						const int expected_v_cnt = (surface->GetNumVCVs() - surface->GetVOrder()) + 2;

						int cvs_cnt = cvs.Count();
						assert(expected_u_cnt == num_u);
						assert(expected_v_cnt == num_v);

						//cvs.Shrink();

						float tmpx, tmpy, tmpz;
						double dtmpx, dtmpy, dtmpz;

						NURBSControlVertex tmp0 ( cvs[0] );
						NURBSControlVertex *tmp0_pt = cvs.Addr(0);
						Point3 p0;// = tmp0_pt->GetPosition(t);
						cvs[0].GetPosition(t, dtmpx, dtmpy, dtmpz); // crashes
						tmp0.GetPosition(t, tmpx, tmpy, tmpz);
						tmp0.GetPosition(t, dtmpx, dtmpy, dtmpz);
						p0 = tmp0.GetPosition(t);

						//nr->uv_point_list = (float*)Malloc(2 * nr->s_point_count * nr->t_point_count * sizeof(float));
								
						for(int u = 0; u < nr->s_point_count; u++)
						{
							for(int v = 0; v < nr->t_point_count; v++)
							{
								Point3 point;
								double w;

								if(flip_normals)
								{
									point = cvs[u * nr->t_point_count + (nr->t_point_count - (v+1))].GetPosition(t);
									w = cvs[u * nr->t_point_count + (nr->t_point_count - (v+1))].GetWeight(t);
								}
								else
								{
									point = cvs[u * nr->t_point_count + v].GetPosition(t);
									w = cvs[u * nr->t_point_count + v].GetWeight(t);

									//nr->uv_point_list[2*(u*nr->t_point_count + v) + 0] = point.x;
										
									//nr->uv_point_list[2*(u*nr->t_point_count + v) + 1] = point.y;

									//printf("%.2f %.2f %.2f %.2f ", point.x, point.y, point.z, w);

									// use uniform knots or geometry knots ??
								}			
							}
							//printf("\n");
						}
					}
					else
					{
						Winprint("Error: %s %s did not get UV's.\n", name, surf_name);
					}
				}
				break;
				case kNMapSufaceMapper:
				{
					const int ts_parent = t_surface.GetParent();
					Winprint("Error: %s %s NURBSTextureSurface type kNMapSufaceMapper not yet supported.\n"
							 "See Mike S.\n", name, surf_name);
				}
				break;
				default:
				{
					Winprint("Error: %s %s has unknown NURBSTextureSurface type %d\n",
						name, surf_name, t_surface.MapperType());
				}
			}

#else //#elif MAX_RELEASE == 2500

			// NOTE: two channels are available
			// adjust uv's for texture cropping
		
			//const float mu = obj->ml->materials[nr->mtl_id]->diffuse.u; // offset
			//const float mv = obj->ml->materials[nr->mtl_id]->diffuse.v;
			//const float mw = obj->ml->materials[nr->mtl_id]->diffuse.w; // tiling
			//const float mh = obj->ml->materials[nr->mtl_id]->diffuse.h;
	
			float tile_u, tile_v;
			IParamBlock *pb = NULL;
			Modifier* uv_mod = GetUVWModifier(node);

			if(uv_mod)
			{
				pb = (IParamBlock*)uv_mod->GetReference(1);
			}

			if(pb)
			{
				tile_u = pb->GetFloat(1); // U Tile
				tile_v = pb->GetFloat(2); // V Tile
				//tile_w = pb->GetFloat(3); // W Tile
			}
			else
			{
				tile_u = 
				tile_v = 1.0f;
			}
			// MAX bug: the tiling is lost if the stack is collapsed

			//BOOL tmp = surface->GenerateUVs(0);
			//surface->SetGenerateUVs(!tmp, 0);
			//NURBSTextureSurface & ts = surface->mTexSurface[0];

			float mu, mv, mw, mh;
			
#if MAX_RELEASE == 2500
			surface->GetTileOffset(t, mw, mh, mu, mv, 0);
#elif MAX_RELEASE >= 3000
			float angle;
			surface->GetTileOffset(t, mw, mh, mu, mv, angle, 0);
#endif
			float u_basis[4];
			float v_basis[4];

			for(int corner = 0; corner < 4; corner++)
			{
				Point2 tmp_uv ( surface->GetTextureUVs( t, corner, 0 ) );

				u_basis[corner] = tile_u * tmp_uv.x;
				v_basis[corner] = tile_v * tmp_uv.y;
				TransformUV(&(u_basis[corner]), &(v_basis[corner]), mu, mv, mw, mh);
			}

			Swap32(&(u_basis[2]), &(u_basis[3]));
			Swap32(&(v_basis[2]), &(v_basis[3]));

			if(flip_normals) // flip top to bottom
			{
				Swap32(&(u_basis[0]), &(u_basis[3]));
				Swap32(&(v_basis[0]), &(v_basis[3]));
				Swap32(&(u_basis[1]), &(u_basis[2]));
				Swap32(&(v_basis[1]), &(v_basis[2]));
			}

			//printf("0 %f %f\n", u_basis[0], v_basis[0]);
			//printf("1 %f %f\n", u_basis[1], v_basis[1]);
			//printf("2 %f %f\n", u_basis[2], v_basis[2]);
			//printf("3 %f %f\n", u_basis[3], v_basis[3]);

			// interpolate UV's per sub patch
			{
				int s_basis_count = (nr->s_point_count - nr->s_order) + 1;
				int t_basis_count = (nr->t_point_count - nr->t_order) + 1;

				// drop basis for each repeated knot
				for(int sk = nr->s_order-1; sk < nr->s_knot_count - nr->s_order; sk++)
				{
					if(nr->s_knot_list[sk] == nr->s_knot_list[sk+1])
					{
						s_basis_count--;
					}
				}

				for(int tk = nr->t_order-1; tk < nr->t_knot_count - nr->t_order; tk++)
				{
					if(nr->t_knot_list[tk] == nr->t_knot_list[tk+1])
					{
						t_basis_count--;
					}
				}

				nr->s_uv_count = s_basis_count + 1;
				nr->t_uv_count = t_basis_count + 1;
			
				nr->uv_list = (float*)Malloc(2 * nr->s_uv_count * nr->t_uv_count * sizeof(float));
						
				for(int u = 0; u < nr->s_uv_count; u++)
				{
					const float u_fraction = (float)u / (float)(nr->s_uv_count - 1);
					const float u_remain = 1.0f - u_fraction;

					for(int v = 0; v < nr->t_uv_count; v++)
					{
						const float v_fraction = (float)v / (float)(nr->t_uv_count - 1);
						const float v_remain = 1.0f - v_fraction;

						float tmp1 = v_remain * u_basis[0] + v_fraction * u_basis[3];
						float tmp2 = v_remain * u_basis[1] + v_fraction * u_basis[2];

						nr->uv_list[2*(u*nr->t_uv_count + v) + 0] =
							u_remain * tmp1 + u_fraction * tmp2;

						tmp1 = v_remain * v_basis[0] + v_fraction * v_basis[3];
						tmp2 = v_remain * v_basis[1] + v_fraction * v_basis[2];

						nr->uv_list[2*(u*nr->t_uv_count + v) + 1] =
							u_remain * tmp1 + u_fraction * tmp2;

						//printf("%.1f %.1f ", nr->uv_list[2*(u*nr->t_uv_count + v) + 0],
							//nr->uv_list[2*(u*nr->t_uv_count + v) + 1]);
					}
					//printf("\n");
				}
			}
#endif // max 2500

#ifdef _DEBUG //text output
			char file_name[256] = {0};
			_snprintf(file_name, 255, "c:\\export\\%s%d.txt", node->GetName(), surf);
			FILE *pStream = fopen(file_name, "wb");

			fprintf(pStream, "%d\n", nr->s_closed ? 1 : 0 );
			fprintf(pStream, "%d\n", nr->t_closed ? 1 : 0 );
			fprintf(pStream, "%d\n", nr->s_order );
			fprintf(pStream, "%d\n", nr->t_order );
			fprintf(pStream, "%d\n", nr->s_knot_count );
			fprintf(pStream, "%d\n", nr->t_knot_count );
			fprintf(pStream, "%d\n", nr->s_point_count );
			fprintf(pStream, "%d\n", nr->t_point_count );

			// u knots
			for ( knot = 0; knot < nr->s_knot_count; knot++ )
			{
				fprintf(pStream, "%.16f ", nr->s_knot_list[knot]);
			}
			fprintf(pStream,"\n\n");

			// v knots
			for ( knot = 0; knot < nr->t_knot_count; knot++ )
			{
				fprintf(pStream, "%.16f ", nr->t_knot_list[knot]);
			}
			fprintf(pStream,"\n\n");

		
			// uv ctrl points
			for ( u = 0; u < nr->s_point_count; u++ )
			{
				for ( int v = 0; v < nr->t_point_count; v++ )
				{
					if(nr->weight_list)
					{
						fprintf(pStream, "%d %d   %.5f %.5f %.5f  %.3f\n",
							u, v,
							nr->point_list[3*(u*nr->t_point_count+v)],
							nr->point_list[3*(u*nr->t_point_count+v)+1],
							nr->point_list[3*(u*nr->t_point_count+v)+2],
							nr->weight_list[u*nr->t_point_count+v]);
					}
					else
					{
						fprintf(pStream, "%d %d   %.5f %.5f %.5f\n",
							u, v,
							nr->point_list[3*(u*nr->t_point_count+v)],
							nr->point_list[3*(u*nr->t_point_count+v)+1],
							nr->point_list[3*(u*nr->t_point_count+v)+2]);
					}
				}
			}

			// UV's
			for(u = 0; u < nr->s_uv_count; u++)
			{
				for(int v = 0; v < nr->t_uv_count; v++)
				{
					fprintf(pStream, "%d %d %.3f %.3f\n",
						u, v,
						nr->uv_list[2*(u*nr->t_uv_count + v) + 0],
						nr->uv_list[2*(u*nr->t_uv_count + v) + 1]);
				}
			}
			
			fclose(pStream);
			pStream = NULL;
#endif
		}
	}

	for(int i = first_nurb_id; i < obj->nurb_count; i++)
	{
		obj->nurb_list[i].api_node_cv_total = node_cv_total;
	}
}

bool AsciiExp::CheckForUVAnim(INode* node, bool **used_list)
{
	assert(*used_list == NULL);
	//assert(*used_face_list == NULL);

	if( !(node->IsAnimated()) )
		return false;

	TimeValue start = _MAX<TimeValue>(0, ip->GetAnimRange().Start());
	TimeValue end = ip->GetAnimRange().End();
	int delta = GetTicksPerFrame();
	int n_frames = 1 + (end - start) / delta;

	TimeValue t;
	int i,j;
	UVVert *uv_list=NULL;

	bool result = false;
	int current_used_count = 0;

	int first_numFaces;
	int first_numTVx;
	int first_numVx;
	{
		BOOL needDel;
		TriObject* tri = GetTriObjectFromNode(node, start, &needDel);
		if (!tri) return false;
			
		Mesh* mesh = &(tri->mesh);
		first_numFaces = mesh->getNumFaces();
		first_numTVx = mesh->getNumTVerts();
		first_numVx = mesh->getNumVerts();

		if (needDel)
		{
			tri->DeleteMe();
			tri = NULL;
		}
	}

	for (t=start, i=0; i < n_frames; t+=delta, i++)
	{
		BOOL needDel;
		TriObject* tri = GetTriObjectFromNode(node, t, &needDel);
		if (!tri) return false;
			
		Mesh* mesh = &(tri->mesh);
		int numFaces = mesh->getNumFaces();
		int numTVx = mesh->getNumTVerts();
		int numVx = mesh->getNumVerts();
		
		if( numFaces != first_numFaces ||
			numTVx != first_numTVx ||
			numVx != first_numVx
		  )
		{
			Winprint("Error: number of faces or vertices is changing w/ animation!!\n");
			if (needDel)
			{
				tri->DeleteMe();
				tri = NULL;
			}
			Free(uv_list);
			return false;
		}

		if(i==0)
		{
			if(numTVx < 1) return false;

			*used_list = (bool*)Malloc(numTVx * sizeof(bool));

			uv_list = (UVVert*)Malloc(numTVx * sizeof(UVVert));

			for(j=0; j<numTVx; j++)
			{
				uv_list[j] = mesh->getTVert(j);//tVerts[j];
				(*used_list)[j] = false;
			}
		}
		else
		{
			for(j=0; j < numTVx; j++)
			{
				// if this vertex is not marked yet
				if((*used_list)[j] == false)
				{
					UVVert uv = mesh->getTVert(j)/*tVerts[j]*/ - uv_list[j];
					if(fabs(uv.x) + fabs(uv.y) > .0001f)
					{
						(*used_list)[j] = true;
						result = true;
						current_used_count++;

						// if everybody is animating by now we can bail out early
						if(current_used_count == numTVx)
						{
							if (needDel) 
							{
								tri->DeleteMe();
								tri = NULL;
							}
							Free(uv_list);
							return true; //goto face_label;
						}
					}
				}
			}
		}
		if (needDel)
		{
			tri->DeleteMe();
			tri = NULL;
		}
	}

	Free(uv_list);

	if(result == false)
	{
		Free(*used_list);
	}

	return result;
}

void AsciiExp::ExportUVAnim(object *obj, INode* node, INode *to_node, const bool *uv_used_list)
{
	//Winprint("Error: UV animation is now broken! %s\n", node->GetName());
	//return;

	assert(uv_used_list);

	int used_batch_count = 0;
	{
		for(int i = 0; i < obj->v.batch_count; i++)
		{
			if( obj->v.api_uv_id[ obj->v.texture_batch_list[i] ] != -1 )
			//if(obj->v.api_uv_id_list[i] != -1)
			{
				used_batch_count++;
				assert(uv_used_list[ 0x0000ffff & obj->v.api_uv_id[ obj->v.texture_batch_list[i] ] ]);
				//assert(uv_used_list[ 0x0000ffff & obj->v.api_uv_id_list[i] ]);
			}
		}
	}

	UVChannel uvc;
	InitUVChannel(&uvc);
	uvc.name = (char*)Malloc((strlen(node->GetName())+1) * sizeof(char));
	strcpy(uvc.name, node->GetName());

	// create (used batch to batch) remap
	uvc.vertex_count = used_batch_count;
	uvc.vertex_lookup = (int*)Malloc(uvc.vertex_count * sizeof(int));

	{
		int index = 0;
		for(int i = 0; i < obj->v.batch_count; i++)
		{
			if( obj->v.api_uv_id[ obj->v.texture_batch_list[i] ] != -1)
			//if(obj->v.api_uv_id_list[i] != -1)
			{
				uvc.vertex_lookup[index] = i;
				index++;
			}
		}
		// sanity check
		assert(index == used_batch_count);
	}
	
	// determine vertex order (we have to be consistent w/ ExportMesh)
	Matrix3 tm ( GetMyObjTMAfterWSM(node, GetStaticFrame()) );
	
	if(to_node) // local
	{
		Matrix3 ntm ( GetMyNodeTM(to_node, GetStaticFrame()) );
		CleanMatrix3(ntm); //ntm.NoScale();  NoShear(ntm); // this will make sure scale is applied to exported geometry
		tm *= Inverse(ntm);
	}
	//else world
	
	int vx1, vx2, vx3;
	if( tm.Parity() )  // negScale
	{
		vx1 = 2;
		vx2 = 1;
		vx3 = 0;
	}
	else
	{
		vx1 = 0;
		vx2 = 1;
		vx3 = 2;
	}

	TimeValue start = _MAX<TimeValue>(0, ip->GetAnimRange().Start());
	TimeValue end = ip->GetAnimRange().End();
	int delta = GetTicksPerFrame();
	int n_frames = 1 + (end - start) / delta;

	uvc.fps = (float)GetFrameRate();
	uvc.frame_count = n_frames;
	uvc.uv_chain = (int*)Malloc(uvc.vertex_count * uvc.frame_count * sizeof(int));
	{
		for(int i=0; i < uvc.vertex_count * uvc.frame_count; i++)
		{
			uvc.uv_chain[i] = -1;
		}
	}
	

	int numFaces = -1;
	int numTVx = -1;

	TimeValue t;
	int frame_id;
	for (t=start, frame_id=0; frame_id < n_frames; t+=delta, frame_id++)
	{
		//ip->SetTime(t, TRUE);

		BOOL needDel;
		TriObject* tri = GetTriObjectFromNode(node, t, &needDel);
		if (!tri) return;

		Mesh* mesh = &(tri->mesh);
		if( numFaces == -1 )
		{
			numFaces = mesh->getNumFaces();
			numTVx = mesh->getNumTVerts();
		}
		else
		{
			if( numFaces != mesh->getNumFaces() )
			{
				Winprint("Error: %s number of faces has changed from %d on frame %d "
						 "to %d on frame %d!\n", node->GetName(), numFaces, 0, mesh->getNumFaces(), frame_id);
				return;
			}

			if( numTVx != mesh->getNumTVerts() )
			{
				Winprint("Error: %s number of UV coordinates has changed from %d on frame %d "
						 "to %d on frame %d!\n", node->GetName(), numTVx, 0, mesh->getNumTVerts(), frame_id);
				return;
			}
		}
		
		int index = 0;
		for(int b_id = 0; b_id < obj->v.batch_count; b_id++)
		{
			const int uv_id = 0x0000ffff & obj->v.api_uv_id[ obj->v.texture_batch_list[b_id] ];
			if( (short)uv_id != -1 )
			{
				//int uv_id = 0x0000ffff & obj->v.api_uv_id_list[b_id];
				assert(uv_id < numTVx);

				UVVert uv = mesh->getTVert(uv_id);//tVerts[uv_id];

				const int m_id = obj->v.mtl_batch_list[b_id];

				// adjust uv's for texture cropping
				//const float mu = obj->ml->materials[m_id]->diffuse.u;
				//const float mv = obj->ml->materials[m_id]->diffuse.v;
				//const float mw = obj->ml->materials[m_id]->diffuse.w;
				//const float mh = obj->ml->materials[m_id]->diffuse.h;

				//TransformUV(&uv.x, &uv.y, mu, mv, mw, mh);
				//AdjustUV(&uv.x, 1);

				if(frame_id == 0)
				{
					uvc.uv_chain[frame_id * uvc.vertex_count + index] =
							//obj->v.texture_batch_list[b_id];
							InsertUVVertex( &(obj->v), &(uv.x), 
							  (int)node, obj->v.api_uv_id[ obj->v.texture_batch_list[b_id] ], 0 );
				}
				else
				{
					uvc.uv_chain[frame_id * uvc.vertex_count + index] =
							InsertUVVertex( &(obj->v), &(uv.x), 0, -1, 0 );
				}

				index++;
			}
		}

		assert(index == used_batch_count);

		if (needDel) 
		{
			tri->DeleteMe();
			tri = NULL;
		}

		// sanity check (the first frame of animation should be the same as fixed uv's)
		if(frame_id == 0)
		{
			for(int z=0; z < uvc.vertex_count; z++)
			{
				int uvc_id = uvc.uv_chain[z];
				int oc_id = obj->v.texture_batch_list[ uvc.vertex_lookup[z] ];

				//if(!SameUV(&(obj->v.texture_list[2*uvc_id]), &(obj->v.texture_list[2*oc_id]), UV_TOLERANCE))
				if(uvc_id != oc_id)
				{
					Winprint("Error: inconsistent UV animation for %s !\n"
							 "First frame does not match fixed frame.", node->GetName());
					break;
				}
			}
		}	
	}


	// sanity check
	{
		for(int i=0; i < uvc.vertex_count * uvc.frame_count; i++)
		{
			if( (uvc.uv_chain[i] == -1) || (uvc.uv_chain[i] >= obj->v.texture_count) )
			{
				Winprint("Error: UV animation for %s is corrupt!\n", node->GetName());
				break;
			}
		}
	}


	char node_name[256] = {0};
	strncpy(node_name, node->GetName(), 255);
	char *pt = node_name + strlen(node_name) - 3;

	if(!strcmp(pt, "_ni") || !strcmp(pt, "_NI"))
	{
		uvc.interpolate = 0;
	}
	else
	{
		uvc.interpolate = 1;
	}

	obj->uvcl.count++;
	obj->uvcl.list = (UVChannel*)Realloc(obj->uvcl.list, obj->uvcl.count*sizeof(UVChannel));
	obj->uvcl.list[obj->uvcl.count-1] = uvc;
}

void TransformUV(float *u, float *v, const float mu, const float mv, const float mw, const float mh)
{
	float old_u = *u;
	float old_v = *v;

	// U's
	*u = old_u * mw + mu;

	// V's
	*v = 1.0f - mv - mh*(1.0f - old_v);
}

Point3 SwitchCoord(const Point3 & p, INode *from, INode *to, TimeValue t)
{
	if(from != to)
	{
		return (p * GetMyLocalNodeTM(from, t, to));
	}
	
	return p;
}

Point3 SwitchNormal(const Point3 & p, INode *from, INode *to, TimeValue t)
{
	if(from != to)
	{
		Matrix3 m1, m2;
		
		m1 = GetMyNodeTM(to, t);
		CleanMatrix3(m1); //m1.NoScale(); NoShear(m1);
		m1.NoTrans();
		
		m1 = Inverse(m1);
		m2 = GetMyNodeTM(from, t);
		CleanMatrix3(m2); //m2.NoScale(); NoShear(m2);
		m2.NoTrans();

		return (p*(m2*m1));
	}

	return p;
}

// TODO: see MtlBaseLib::RemoveDuplicates();



int AsciiExp::ExportCQ2Material(Mtl *mat, object *obj, const TimeValue t, INode *node)
{
	assert(mat || node);
	assert(obj);// && obj->tl);
	int mtl_id = -1;
	cq2Mtl *m = NULL;

	if(mat && node)
	{
		Class_ID mtl_cid ( mat->ClassID() );
		if(mtl_cid == CQ2Material_CLASS_ID)
		{
			// cast mat to new struct
			iCQ2Mtl * icq2 = (iCQ2Mtl*)mat;
			mtl_id = GetCQ2MtlID(obj->ml, icq2->GetMatName());
			if(mtl_id < 0) // new material
			{
				Interval interval;
				//std->Update(t, interval);  // eval any effects
				m = (cq2Mtl*)Malloc(sizeof(cq2Mtl));
				InitMaterialCQ2(m/*, obj->atl*/);
				m->name = _strdup(icq2->GetMatName());
				m->api_id = (int)icq2;
				mtl_id = InsertCq2Material(obj->ml, m);
				assert(mtl_id >= 0);
			}
		}
		else
		if(mat->ClassID() == Class_ID(MULTI_CLASS_ID, 0))
		{
			Winprint("Error: material %s on object %s is a MultiSub material!\n", mat->GetName(),node->GetName());
		}
		else
		{
			Winprint("Error: unknown material type %s on object %s!\n", mat->GetName(),node->GetName());
		}
	}
	else
	if(node) // export node color instead of a real material
	{
	}
	return mtl_id;
}

/*
int AsciiExp::ExportMAXMaterial(Mtl *mat, object *obj, const TimeValue t, INode *node)
{
#ifdef _DEBUG
	const char *mat_name = NULL;
	if(mat) mat_name = mat->GetName();
	const char *node_name = node->GetName();
#endif

	assert(mat || node);
	assert(obj);// && obj->tl);

	int mtl_id = -1;
	mtl *m = NULL;

	if(mat && node)
	{
		//Todo: Check for CQ2MATERIAL class here
		//And make alternate code path

		Class_ID mtl_cid ( mat->ClassID() );
		if(mtl_cid == Class_ID(DMTL_CLASS_ID, 0))
		{
			StdMat * std = (StdMat*)mat; mat = NULL;

			const int double_sided = ( TRUE == std->GetTwoSided() ) ? 1 : 0;
			mtl_id = GetMtlID(obj->ml, std->GetName(), double_sided);

			// in case two unique materials have the same name
			
			//if(mtl_id >= 0 && obj->ml->materials[mtl_id]->api_id != (int)std)
			{
			//	mtl_id = GetMtlID(&(obj->ml), (int)std);
			}
			

			if(mtl_id < 0) // new material
			{
				Interval interval;
				std->Update(t, interval);  // eval any effects

				m = (mtl*)Malloc(sizeof(mtl));
				InitMaterial(m);//, obj->atl);

				m->name = _strdup(std->GetName());
				
				m->double_sided = double_sided;

				if(!default_mat_flag)
				{
					Color color = std->GetAmbient(t);
					m->ambient.value[0]=color.r;
					m->ambient.value[1]=color.g;
					m->ambient.value[2]=color.b;

					color = std->GetDiffuse(t);
					m->diffuse.value[0]=color.r;
					m->diffuse.value[1]=color.g;
					m->diffuse.value[2]=color.b;

					color = std->GetSpecular(t);
					m->specular.value[0]=color.r;
					m->specular.value[1]=color.g;
					m->specular.value[2]=color.b;

#if MAX_RELEASE == 2500
					m->emission.value[0]=std->GetSelfIllum(t) * m->diffuse.value[0];
					m->emission.value[1]=std->GetSelfIllum(t) * m->diffuse.value[1];
					m->emission.value[2]=std->GetSelfIllum(t) * m->diffuse.value[2];	
#else
					if( (std->SupportsShaders() == TRUE) && ((StdMat2*)std)->GetSelfIllumColorOn() )
					{
						StdMat2* std2 = (StdMat2*)std;
						
						color = std2->GetSelfIllumColor(0, FALSE); //GetSelfIllumColor(t); causes an access violation when max exits
						m->emission.value[0] = color.r;
						m->emission.value[1] = color.g;
						m->emission.value[2] = color.b;
					}
					else
					{
						m->emission.value[0]=std->GetSelfIllum(t) * m->diffuse.value[0];
						m->emission.value[1]=std->GetSelfIllum(t) * m->diffuse.value[1];
						m->emission.value[2]=std->GetSelfIllum(t) * m->diffuse.value[2];
					}
#endif
					m->shininess.value[0] = std->GetShininess(t);
					m->shininess.value[1] = std->GetShinStr(t);

					m->transparency.value[0] = 1.0f - std->GetXParency(t);
				}
				else
				{
					m->ambient.value[0]=
					m->ambient.value[1]=
					m->ambient.value[2]= 1.0f;

					m->diffuse.value[0]=
					m->diffuse.value[1]=
					m->diffuse.value[2]= 1.0f;

					m->specular.value[0]=
					m->specular.value[1]=
					m->specular.value[2]= 0.0f;

					m->emission.value[0]=
					m->emission.value[1]=
					m->emission.value[2]= 0.0f;

					m->shininess.value[0] =
					m->shininess.value[1] = 0.0f;

					m->transparency.value[0] = 1.0f;
				}

				BitmapTex *d_tex = GetMap(std, ID_DI);
				BitmapTex *o_tex = GetMap(std, ID_OP);		// opacity/alpha
				BitmapTex *a_tex = GetMap(std, ID_AM);
				BitmapTex *e_tex = GetMap(std, ID_SI);		// emission/self-illumination
				BitmapTex *sp_tex = GetMap(std, ID_SP);		// specular
				BitmapTex *sh_tex = GetMap(std, ID_SH);		// shininess
				BitmapTex *bump_tex = GetMap(std, ID_BU);	// bump used as diffuse2 for now
				//BitmapTex *d2_tex = GetMap(std, ID_FI);	// filter color (will be used as diffuse 2)
				
				if(d_tex)
					m->diffuse.blend = std->GetTexmapAmt(ID_DI, t);
				if(o_tex)
					m->transparency.blend = std->GetTexmapAmt(ID_OP, t);
				if(a_tex)
					m->ambient.blend = std->GetTexmapAmt(ID_AM, t);
				if(e_tex)
					m->emission.blend = std->GetTexmapAmt(ID_SI, t);
				if(sp_tex)
					m->specular.blend = std->GetTexmapAmt(ID_SP, t);
				if(sh_tex)
					m->shininess.blend = std->GetTexmapAmt(ID_SH, t);
					
				if(bump_tex)
				{
					m->bump.blend = std->GetTexmapAmt(ID_BU, t);
					m->bump.value[0] = 1.0f;
				}
				
				
				defineTexture(d_tex, o_tex, &(m->diffuse));
				// use bump for now
				
				if(d_tex && d2_tex)
				{
					mtl_property tmp_prop;
					InitMaterialProperty(&tmp_prop, 1.0f, DIFFUSE, m->diffuse.atl);

					defineTexture((BitmapTex *)d2_tex, NULL, &tmp_prop);
					m->diffuse.texture_name2 = _strdup(tmp_prop.texture_name);
					if(tmp_prop.anim_texture_name)
					{
						Winprint("Error: animated second diffuse %s not yet supported!\n",
							tmp_prop.anim_texture_name);
					}

					FreeMtlProperty(&tmp_prop);
				}
				

				defineTexture((BitmapTex *)a_tex, NULL, &(m->ambient));
				CheckMtlPropOffset(m->ambient, m->diffuse, m->name);

				defineTexture((BitmapTex *)e_tex, NULL, &(m->emission));
				CheckMtlPropOffset(m->emission, m->diffuse, m->name);

				defineTexture((BitmapTex *)sp_tex, NULL, &(m->specular));
				CheckMtlPropOffset(m->specular, m->diffuse, m->name);

				defineTexture((BitmapTex *)sh_tex, NULL, &(m->shininess));
				CheckMtlPropOffset(m->shininess, m->diffuse, m->name);

				defineTexture((BitmapTex *)bump_tex, NULL, &(m->bump));
				CheckMtlPropOffset(m->bump, m->diffuse, m->name);
				
				m->api_id = (int)std;
				mtl_id = InsertMaterial(obj->ml, m);
				assert(mtl_id >= 0);
			}
		}
		else
		if(mat->ClassID() == Class_ID(MULTI_CLASS_ID, 0))
		{
			Winprint("Error: material %s on object %s is a MultiSub material!\n", mat->GetName(),
				node->GetName());
		}
		else
		{
			Winprint("Error: unknown material type %s on object %s!\n", mat->GetName(),
				node->GetName());
		}
	}
	else
	if(node) // export node color instead of a real material
	{
		mtl_id = GetMtlID(obj->ml, node->GetName(), 0);

		// in case two unique materials have the same name
		
		if(mtl_id >= 0 && obj->ml->materials[mtl_id]->api_id != (int)node)
		{
			mtl_id = GetMtlID(&(obj->ml), (int)node);
		}
		

		if(mtl_id < 0) // new material
		{
			m = (mtl*)Malloc(sizeof(mtl));
			InitMaterial(m, obj->atl);

			m->name = _strdup(node->GetName());

			DWORD c = node->GetWireColor();
		
			m->ambient.value[0]=GetRValue(c)/255.0f;
			m->ambient.value[1]=GetGValue(c)/255.0f;
			m->ambient.value[2]=GetBValue(c)/255.0f;

			m->diffuse.value[0]=GetRValue(c)/255.0f;
			m->diffuse.value[1]=GetGValue(c)/255.0f;
			m->diffuse.value[2]=GetBValue(c)/255.0f;

			m->api_id = (int)node;
			mtl_id = InsertMaterial(obj->ml, m);
			assert(mtl_id >= 0);
		}
	}

	if( m )
	{
		SetMtlType( m );
	}

	return mtl_id;
}*/
/*
void AddDebugMtls(mtl_lib *ml)
{
	mtl *m;
	int mtl_id;
	
	// debug mtls
	m=(mtl*)Malloc(sizeof(mtl));
	InitMaterial(m, NULL);
	m->name=(char*)Malloc((strlen("white")+1)*sizeof(char));
	strcpy(m->name, "debug0");
	m->ambient.value[0]=0.8f;
	m->ambient.value[1]=0.8f;
	m->ambient.value[2]=0.8f;
	m->diffuse.value[0]=0.8f;
	m->diffuse.value[1]=0.8f;
	m->diffuse.value[2]=0.8f;
	m->specular.value[0]=0.8f;
	m->specular.value[1]=0.8f;
	m->specular.value[2]=0.8f;
	m->shininess.value[0] = 1.0f;
	m->transparency.value[0] = 1.0f;
	mtl_id = InsertMaterial(ml, m);
	assert(mtl_id >= 0);

	m=(mtl*)Malloc(sizeof(mtl));
	InitMaterial(m, NULL);
	m->name=(char*)Malloc((strlen("red")+1)*sizeof(char));
	strcpy(m->name, "debug1");
	m->ambient.value[0]=1.0f;
	m->ambient.value[1]=0.0f;
	m->ambient.value[2]=0.0f;
	m->diffuse.value[0]=1.0f;
	m->diffuse.value[1]=0.0f;
	m->diffuse.value[2]=0.0f;
	m->specular.value[0]=1.0f;
	m->specular.value[1]=0.0f;
	m->specular.value[2]=0.0f;
	m->shininess.value[0] = 1.0f;
	m->transparency.value[0] = 1.0f;
	mtl_id = InsertMaterial(ml, m);
	assert(mtl_id >= 0);

	m=(mtl*)Malloc(sizeof(mtl));
	InitMaterial(m, NULL);
	m->name=(char*)Malloc((strlen("green")+1)*sizeof(char));
	strcpy(m->name, "debug2");
	m->ambient.value[0]=0.0f;
	m->ambient.value[1]=1.0f;
	m->ambient.value[2]=0.0f;
	m->diffuse.value[0]=0.0f;
	m->diffuse.value[1]=1.0f;
	m->diffuse.value[2]=0.0f;
	m->specular.value[0]=0.0f;
	m->specular.value[1]=1.0f;
	m->specular.value[2]=0.0f;
	m->shininess.value[0] = 1.0f;
	m->transparency.value[0] = 1.0f;
	mtl_id = InsertMaterial(ml, m);
	assert(mtl_id >= 0);

	m=(mtl*)Malloc(sizeof(mtl));
	InitMaterial(m, NULL);
	m->name=(char*)Malloc((strlen("blue")+1)*sizeof(char));
	strcpy(m->name, "debug3");
	m->ambient.value[0]=0.0f;
	m->ambient.value[1]=0.0f;
	m->ambient.value[2]=1.0f;
	m->diffuse.value[0]=0.0f;
	m->diffuse.value[1]=0.0f;
	m->diffuse.value[2]=1.0f;
	m->specular.value[0]=0.0f;
	m->specular.value[1]=0.0f;
	m->specular.value[2]=1.0f;
	m->shininess.value[0] = 1.0f;
	m->transparency.value[0] = 1.0f;
	mtl_id = InsertMaterial(ml, m);
	assert(mtl_id >= 0);
}*/

// TODO: add mix maps (check mix value is .5)
BitmapTex* GetMap(StdMat *std, const int map_type)
{
	Texmap *tmap = NULL;

	if(std && std->MapEnabled(map_type))
	{
		tmap = std->GetSubTexmap(map_type); // std->GetActiveTexmap(); std->NumSubTexmaps();
		if(tmap && tmap->ClassID() != Class_ID(BMTEX_CLASS_ID, 0x00)) // only export bitmaps
		{
			tmap = NULL;
		}
	}

	return (BitmapTex*)tmap;
}

/****************************************************************************

  Misc Utility functions
  
****************************************************************************/


// Return a pointer to a TriObject given an INode or return NULL
// if the node cannot be converted to a TriObject
TriObject* AsciiExp::GetTriObjectFromNode(INode *node, TimeValue t, BOOL *deleteIt)
{
	assert(node);
#ifdef _DEBUG
	const char *name = node->GetName();
#endif
	*deleteIt = FALSE;
	Object *obj = node->EvalWorldState(t).obj;
	assert(obj);
	if ( obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)) )
	{ 
		TriObject *tri = (TriObject *) obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
		assert(tri);
		// Note that the TriObject should only be deleted
		// if the pointer to it is not equal to the object
		// pointer that called ConvertToType()
		if (obj != tri)
		{
			*deleteIt = TRUE;
			//fprintf(stderr, "Notice: %s had to be converted to TriObject.\n",node->GetName());
		}
		return tri;
	}
	else
	{
		return NULL;
	}
}

PatchObject* AsciiExp::GetPatchObjectFromNode(INode *node, TimeValue t, int *deleteIt)
{
	*deleteIt = FALSE;
	Object *obj = node->EvalWorldState(t).obj;
	if ( obj->CanConvertToType(Class_ID(PATCHOBJ_CLASS_ID, 0)) )
	{ 
		PatchObject *tri = (PatchObject *) obj->ConvertToType(t, Class_ID(PATCHOBJ_CLASS_ID, 0));
		// Note that the TriObject should only be deleted
		// if the pointer to it is not equal to the object
		// pointer that called ConvertToType()
		if (obj != tri)
		{
			*deleteIt = TRUE;
			//fprintf(stderr, "Notice: %s had to be converted to TriObject.\n",node->GetName());
		}
		return tri;
	}
	else
	{
		return NULL;
	}
}

void DumpMatrix3(Matrix3* m)
{
	Point3 row;
	
	// Dump the whole Matrix
	row = m->GetRow(0);
	printf("%f %f %f\n",row.x,row.y,row.z);
	row = m->GetRow(1);
	printf("%f %f %f\n",row.x,row.y,row.z);
	row = m->GetRow(2);
	printf("%f %f %f\n",row.x,row.y,row.z);
	row = m->GetRow(3);
	printf("%f %f %f\n",row.x,row.y,row.z);
	
	// Decompose the matrix and dump the contents
	AffineParts ap;
	float rotAngle;
	Point3 rotAxis;
	float scaleAxAngle;
	Point3 scaleAxis;
	
	decomp_affine(*m, &ap);

	// Quaternions are dumped as angle axis.
	AngAxisFromQ(ap.q, &rotAngle, rotAxis);
	AngAxisFromQ(ap.u, &scaleAxAngle, scaleAxis);

	printf("Position         %f %f %f\n", ap.t.x, ap.t.y, ap.t.z);
	printf("Rotation         %f %f %f\n", rotAxis.x, rotAxis.y, rotAxis.z);
	printf("Rot angle        %f\n", rotAngle);
	printf("Scale            %f\n", ap.k);
	printf("Scale axis       %f %f %f\n", scaleAxis.x, scaleAxis.y, scaleAxis.z);
	printf("Scale axis angle %f\n", scaleAxAngle);
}

int GetRGBID(VRGB **rgb, const int id, const int length)
{
	assert(id > 0);

	for(int i = 0; i < id; i++)
	{
		bool same = true;
		for(int j = 0; j < length; j++)
		{
			if( abs((int)rgb[id][j].r - (int)rgb[i][j].r) > 2 ||
				abs((int)rgb[id][j].g - (int)rgb[i][j].g) > 2 ||
				abs((int)rgb[id][j].b - (int)rgb[i][j].b) > 2)
			{
				same = false;
				break;
			}
		}

		if(same)
		{
			return i;
		}
	}

	return -1;
}



void EnumModifiers( INode *node, void (__cdecl *fp)(Animatable *mod, void *user_data), void *user_data)
{
	Object* ObjectPtr = node->GetObjectRef();
	
	if ( ObjectPtr ) 
	{
		// Is derived object ?
		if (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID)
		{
			// Yes -> Cast.
			IDerivedObject* DerivedObjectPtr = static_cast<IDerivedObject*>(ObjectPtr);

			// Iterate over all entries of the modifier stack.
			const int num_mods = DerivedObjectPtr->NumModifiers();
			for( int ModStackIndex = 0; ModStackIndex < num_mods; ModStackIndex++)
			{
				// Get current modifier.
				Modifier* ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);
				
				const char *name = ModifierPtr->GetName();

				/*
				if( !strcmp( name, "Surface" ) )
				{
					int iii =0;
				}
				*/

				const SClass_ID sc_id ( ModifierPtr->SuperClassID() );
				const Class_ID c_id ( ModifierPtr->ClassID() );

				fp( ModifierPtr, user_data);	
			}
		}
	}
}

void __cdecl DumpParam2Block( Animatable *an_obj, void *user_data )
{
#if MAX_RELEASE >= 3000

	int block_cnt = an_obj->NumParamBlocks();
	for( short bid = 0; bid < block_cnt; bid++)
	{
		IParamBlock2 *pb0 = an_obj->GetParamBlock(bid);

		if( pb0 )
		{
			TCHAR *tc = pb0->GetLocalName();
			
			int num_params = pb0->NumParams();
			for(short i = 0; i < num_params; i++)
			{
				TSTR ln = pb0->GetLocalName(i); //"Clip U Offset" "Clip U Width" "Apply"
				ParamType2 type = pb0->GetParameterType(i); //TYPE_FLOAT
			}
			
			ParamBlockDesc2 *pdc = pb0->GetDesc();
			
			// Loop through all the defined parameters therein
			for( i = 0; i < pdc->count; i++)
			{
				// Get a ParamDef structure for the parameter
				const ParamDef & pD = pdc->paramdefs[i];
				
				const char *name = pD.int_name;
			}

			pb0->ReleaseDesc();
		}

		pb0 = an_obj->GetParamBlockByID(bid);
		if( pb0 )
		{
			TCHAR *tc = pb0->GetLocalName();
			
			int num_params = pb0->NumParams();
			for(short i = 0; i < num_params; i++)
			{
				TSTR ln = pb0->GetLocalName(i); //"Clip U Offset" "Clip U Width" "Apply"
				ParamType2 type = pb0->GetParameterType(i); //TYPE_FLOAT
			}
			
			ParamBlockDesc2 *pdc = pb0->GetDesc();
			
			// Loop through all the defined parameters therein
			for( i = 0; i < pdc->count; i++)
			{
				// Get a ParamDef structure for the parameter
				const ParamDef & pD = pdc->paramdefs[i];
				
				const char *name = pD.int_name;
			}

			pb0->ReleaseDesc();
		}
	}

#endif

}


void GetUVChannels(Mesh *mesh,
				   const TVFace **f_uv0, const TVFace **f_uv1,
				   const Point3 **v_uv0, const Point3 **v_uv1)
{
	*f_uv0 = *f_uv1 = NULL;
	*v_uv0 = *v_uv1 = NULL;

	*f_uv0 = mesh->mapFaces( 1 );
	*v_uv0 = mesh->mapVerts( 1 );
	if( *f_uv0 == NULL || *v_uv0 == NULL )
	{
		*f_uv0 = NULL;
		*v_uv0 = NULL;
		return;
	}

	*f_uv1 = mesh->mapFaces( 2 );
	*v_uv1 = mesh->mapVerts( 2 );
	if( *f_uv1 != NULL && *v_uv1 != NULL )
	{
		return;
	}

	if( mesh->numCVerts > 0 && mesh->vcFace && mesh->vertCol )
	{
		*f_uv1 = mesh->vcFace;
		*v_uv1 = mesh->vertCol;
	}

}

void GetMtlProperty(StdMat *std, FACE_PROPERTY *property)
{
	if(std->GetTwoSided())
	{
		*property = *property | TWO_SIDED;
	}
	else
	{
		*property = *property & ~TWO_SIDED;
	}
	
#if 0 // we now use smoothing groups instead
	int shading=std->GetShading();
	// Shade values
	switch(shading)				
	{
	case SHADE_CONST: // none
		*property = *property & ~SMOOTH_SHADED;
		*property = *property | FLAT_SHADED;
		break;
	case SHADE_PHONG: // gouraud
	case SHADE_METAL:
	case SHADE_BLINN:
		*property = *property & ~FLAT_SHADED;
		*property = *property | SMOOTH_SHADED;
		break;
	default:
		Winprint("Warning: unknown shading %d on %s\n",shading,std->GetName());
		*property = *property & ~FLAT_SHADED;
		*property = *property | SMOOTH_SHADED;
	}
#endif
}

int IsPlaceholder(INode *node)
{
	if(!strncmp(node->GetName(), "Particle", 8) ||
	   !strncmp(node->GetName(), "particle", 8))
	{
		return 1;
	}

	return 0;
}

bool IsHP(INode *node, TimeValue t)
{
	bool result = false;

	ObjectState os ( node->EvalWorldState(t) ); 
	if (os.obj)
	{					
		const SClass_ID sc_id ( os.obj->SuperClassID() );
		const Class_ID c_id ( os.obj->ClassID() );

		if( sc_id == HELPER_CLASS_ID && 
			c_id == Class_ID( DUMMY_CLASS_ID, 0 ) &&
			!IsPlaceholder(node) && // user for particle systems
			!HasLodChildren(node) )
		{
			result = true;
		}
	}

	return result;
}

void AsciiExp::DebugFn(INode* node)
{
	const char *name = node->GetName();

	TimeValue start = _MAX<TimeValue>(0, ip->GetAnimRange().Start());
	TimeValue end = ip->GetAnimRange().End();
	int delta = GetTicksPerFrame();
	int n_frames = 1 + (end - start) / delta;

	TimeValue t;
	int frame_id;
	for (t=start, frame_id=0; frame_id < n_frames; t+=delta, frame_id++)
	{
		BOOL needDel;
		TriObject* tri = GetTriObjectFromNode(node, t, &needDel);
		assert(tri);

		if( needDel )
		{
			tri->DeleteMe();
			tri = NULL;
		}
	}
}

#pragma warning( pop )