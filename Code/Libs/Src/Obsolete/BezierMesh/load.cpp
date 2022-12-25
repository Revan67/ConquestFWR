#include <stdio.h>
#include "Bezier.h"
#include "BezierFileIO.h"
#include "ITXMLib.h"
#include "TSmartPointer.h"
#include "FDump.h"
#include "tempstr.h"
#include "beziermesh.h"

#pragma warning( push, 4 )

#ifndef NDEBUG
	#define CHECK(x)		\
	{						\
		int result = (x);	\
		ASSERT (result);	\
	}
#else								
	#define CHECK(x)	(void)(x);
#endif

bool BezierMesh::read (IFileSystem* parent, ITXMLib* txm_lib)
{
	ASSERT(parent);
	ASSERT(txm_lib);

	COMPTR<IFileSystem> f1, f2, f3, f4;

	f1 = parent;

	{
		//
		// Read Bezier mesh
		//

		read_val(f1, "Patch count",		&patch_cnt);

		if (descend_read(f1, "Patch groups", f2) == GR_OK)
		{
			read_val(f2, "Count",		&group_cnt);
			patch_groups = new PatchGroup[group_cnt];
		
			PatchGroup * group = patch_groups;
			for (int i = 0; i < group_cnt; i++, group++)
			{
				//memset(group, 0, sizeof(*group));

				char group_name[32];
				sprintf(group_name, "Group%d", i);
				if (descend_read(f2, group_name, f3) == GR_OK)
				{
					read_val(f3, "Patch count",		&group->patch_cnt);
					read_ptr(f3, "Patch list",		&group->patch_list);
					read_val(f3, "Material",		&group->mtl_id);
				}
				else
				{
					GENERAL_FATAL ("Bezier group missing!\n");
				}
			}
		}

		if (descend_read(f1, "Geometry", f2) == GR_OK)
		{
			read_val(f2, "Vertex count",	&vertex_cnt);
			read_ptr(f2, "Vertex list",		&vertices);

			read_val(f2, "UV count",		&uv_cnt);
			read_ptr(f2, "UV list",			&uv_list);
		}

		if (descend_read(f1, "Edges", f2) == GR_OK)
		{
			read_val(f2, "Edge count",	&edge_cnt);
			read_ptr(f2, "Edge list",	&edges);
		}

		// copy tri patch order 5 aux data into vectors
		for(int gid = 0; gid < group_cnt; gid++)
		{
			PatchGroup & group = patch_groups[gid];

			for(int pid = 0; pid < group.patch_cnt; pid++)
			{
				BezierPatch & patch = group.patch_list[pid];
				if( patch.type == 3 )
				{
					for(int i = 0; i < 9; i++)
					{
						patch.vec[i] = patch.aux_index + i;
					}
				}
			}
		}

		//
		// Read optional texture library
		//

		txm_lib->load_library(f1);

		//
		// Read material library 
		//

		if (descend_read(f1, "Material library", f2) == GR_OK)
		{
			//
			// Get # of materials in this mesh's material list
			// (must be nonzero)
			//

			read_val(f2, "Material count", &material_cnt);

			ASSERT_FATAL(material_cnt);
			material_list = new Material[material_cnt];

			//
			// Walk material directories, creating each material in turn and
			// assigning it to a list entry based on its identifier value
			//

			HANDLE          search;
			WIN32_FIND_DATA found;

			search = f2->FindFirstFile("*.*", &found);

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

					if (descend_read(f2, found.cFileName, f3) == GR_OK)
					{
						DWORD read;

						int material_index;

						int r = read_val(f3, "Material identifier", &material_index);
						ASSERT_FATAL (r);

						ASSERT_FATAL (material_index < material_cnt);

						Material* cur = material_list + material_index;

						cur->name = strdup(found.cFileName);


						f3->SetCurrentDirectory("Ambient");
						
						float rgb[3];
						DAFILEDESC desc ("Constant");
						desc.lpImplementation = "DOS";

						HANDLE h = f3->OpenChild (&desc);
						if (INVALID_HANDLE_VALUE != h)
						{
							CHECK (f3->ReadFile (h, rgb, sizeof (rgb), &read))
							CHECK (f3->CloseHandle (h))

							cur->ambient.r = (unsigned char)(rgb[0] * 255);
							cur->ambient.g = (unsigned char)(rgb[1] * 255);
							cur->ambient.b = (unsigned char)(rgb[2] * 255);

							if ((cur->ambient.r != 0) || 
								(cur->ambient.g != 0) ||
								(cur->ambient.b != 0))
							{
								cur->flags |= MF_AMBIENT;
							}
						}
						f3->SetCurrentDirectory("..");

						f3->SetCurrentDirectory("Diffuse");						
						h = f3->OpenChild (&desc);
						if (INVALID_HANDLE_VALUE != h)
						{
							CHECK (f3->ReadFile (h, rgb, sizeof (rgb), &read))
							CHECK (f3->CloseHandle (h))

							cur->diffuse.r = (unsigned char)(rgb[0] * 255);
							cur->diffuse.g = (unsigned char)(rgb[1] * 255);
							cur->diffuse.b = (unsigned char)(rgb[2] * 255);

							if ((cur->diffuse.r != 0) || 
								(cur->diffuse.g != 0) ||
								(cur->diffuse.b != 0))
							{
								cur->flags |= MF_DIFFUSE;
							}

							if(f3->SetCurrentDirectory("Map"))
							{
								DAFILEDESC desc2 ("Flags");
								desc2.lpImplementation = "DOS";
								h = f3->OpenChild (&desc2);
								if (INVALID_HANDLE_VALUE != h)
								{
									U32 flags;
									f3->ReadFile (h, &flags, sizeof(flags), &read);
									ASSERT_FATAL(read == sizeof(flags));
									f3->CloseHandle (h);

									cur->texture_flags = flags;
								}

								//see if this material has a texture map associated with it
								
								desc2.lpFileName = "Name";
								h = f3->OpenChild (&desc2);
								if (INVALID_HANDLE_VALUE != h)
								{
									int size = f3->GetFileSize (h);
									ASSERT_FATAL (size);
									char* name = new char[size + 1];
									name[size] = 0;

									CHECK (f3->ReadFile (h, name, size, &read))
									CHECK (f3->CloseHandle (h))
									
									cur->texture_id = txm_lib->get_texture_id (name);
									//cur->texture_alpha_bits = txm_lib->get_alpha_bits(cur->texture_id);
									ASSERT_FATAL (cur->texture_id != INVALID_TXM_ID);

									if( txm_lib->get_alpha_bits( cur->texture_id ) )
									{
										cur->texture_flags |= TF_F_HAS_ALPHA;
									}

									delete [] name;
								}
							
								f3->SetCurrentDirectory("..");
							}
							else
							{
								cur->texture_id = INVALID_TXM_ID;							
								//cur->texture_alpha_bits = 0;
							}
						}
						f3->SetCurrentDirectory("..");

						f3->SetCurrentDirectory("Specular");						
						h = f3->OpenChild (&desc);
						if (INVALID_HANDLE_VALUE != h)
						{
							CHECK (f3->ReadFile (h, rgb, sizeof (rgb), &read))
							CHECK (f3->CloseHandle (h))

							cur->specular.r = (unsigned char)(rgb[0] * 255);
							cur->specular.g = (unsigned char)(rgb[1] * 255);
							cur->specular.b = (unsigned char)(rgb[2] * 255);

							if ((cur->specular.r != 0) || 
								(cur->specular.g != 0) ||
								(cur->specular.b != 0))
							{
								cur->flags |= MF_SPECULAR;
							}
						}
						f3->SetCurrentDirectory("..");

						f3->SetCurrentDirectory("Emission");						
						h = f3->OpenChild (&desc);
						if (INVALID_HANDLE_VALUE != h)
						{
							CHECK (f3->ReadFile (h, rgb, sizeof (rgb), &read))
							CHECK (f3->CloseHandle (h))

							cur->emission.r = (unsigned char)(rgb[0] * 255);
							cur->emission.g = (unsigned char)(rgb[1] * 255);
							cur->emission.b = (unsigned char)(rgb[2] * 255);

							if ((cur->emission.r != 0) || 
								(cur->emission.g != 0) ||
								(cur->emission.b != 0))
							{
								cur->flags |= MF_EMITTER;
							}
						}
						f3->SetCurrentDirectory("..");

						f3->SetCurrentDirectory("Shininess");	
						h = f3->OpenChild (&desc);
						if (INVALID_HANDLE_VALUE != h)
						{
							float shininess;
							CHECK (f3->ReadFile (h, &shininess, sizeof (SINGLE), &read))
							CHECK (f3->CloseHandle (h))

							cur->shininess = (unsigned char)(shininess * 255);
						}
						f3->SetCurrentDirectory("..");

						f3->SetCurrentDirectory("Transparency");
						h = f3->OpenChild (&desc);
						if (INVALID_HANDLE_VALUE != h)
						{
							float transparency;
							CHECK (f3->ReadFile (h, &transparency, sizeof (SINGLE), &read))
							CHECK (f3->CloseHandle (h))

							cur->transparency = (unsigned char)(transparency * 255);
						}
						f3->SetCurrentDirectory("..");
					}
				}
				while (f2->FindNextFile(search, &found));
			}

			f2->FindClose(search);
		}

		if (descend_read(f1, "Sphere", f2) == GR_OK)
		{
			read_val(f2, "Center", &sphere_center);
			read_val(f2, "Radius", &radius);

			ASSERT_FATAL(radius > 0.0f);
		}
		else
		{
			GENERAL_FATAL ("Render bounding Sphere missing in file!\n");
		}

		compute_bounds();
		compute_centroid();
	}
	
	GENERAL_TRACE_2(TEMPSTR("BEZIERMESH: Created %d patches, %d materials\n",
						patch_cnt,
						material_cnt));

	return true;
}


#pragma warning( pop )