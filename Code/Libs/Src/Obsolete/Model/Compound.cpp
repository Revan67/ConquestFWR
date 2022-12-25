
//$Header: /Libs/Dev/Src/EngComps/Model/Compound.cpp 6     10/14/99 6:12p Pbleisch $

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "Compound.h"
#include "PersistCompound.h"
#include "TSmartPointer.h"
#include "Filesys.h"
#include "fdump.h"

#ifndef NDEBUG
	#define CHECK(x)		\
	{						\
		int result = (x);	\
		ASSERT (result);	\
	}
#else								
	 #define CHECK(x)	(void)(x);
#endif

using namespace Compound;

const char NULL_OBJECT_LABEL[] = "NULL";

ALLOCLITE_GLOBAL (ARCHETYPE_ALLOC)
ALLOCLITE_GLOBAL (CNXNDEF_ALLOC)

Archetype::Archetype (const char* _label, ARCHETYPE_INDEX _archetype) : archetype (_archetype)
{
	ASSERT (label);
	strncpy (label, _label, PARTNAME_MAX);
}

Archetype::~Archetype (void)
{
	for (ARCHETYPE_LIST::iterator it = children.begin ();
		it != children.end ();
		it++)
	{
		delete *it;
		*it = NULL;
	}
}

const char* Archetype::get_label (void) const
{
	return label;
}

ARCHETYPE_INDEX Archetype::get_archetype_index (void) const
{
	return archetype;
}

const ARCHETYPE_LIST& Archetype::get_children (void) const
{
	return children;
}

const CNXNDEF_LIST& Archetype::get_connections (void) const
{
	return connections;
}

Archetype* CreateNamedArchetype (IFileSystem* fs, IEngine* engine, IFileSystem* _parts)
{
	//fs is a directory

	DAFILEDESC desc (OBJECT_NAME);
	desc.dwDesiredAccess = GENERIC_READ;
	desc.dwCreationDistribution = OPEN_EXISTING;

	HANDLE handle = fs->OpenChild (&desc);
	ASSERT ((HANDLE)-1 != handle);

	DWORD filesize = fs->GetFileSize (handle);
	char part_name[PARTNAME_MAX];

	ASSERT (filesize <= sizeof (part_name));

	DWORD bytes_read;

	CHECK (fs->ReadFile (handle, part_name, filesize, &bytes_read));
	ASSERT (bytes_read == filesize);

	CHECK (fs->CloseHandle (handle));

	desc.lpFileName = FILE_NAME;

	handle = fs->OpenChild (&desc);
	ASSERT ((HANDLE)-1 != handle);

	char file_name[PARTFILENAME_MAX];
	filesize = fs->GetFileSize (handle);

	ASSERT (filesize <= sizeof (file_name));

	CHECK (fs->ReadFile (handle, file_name, filesize, &bytes_read));
	ASSERT (bytes_read == filesize);

	CHECK (fs->CloseHandle (handle));

	desc.lpFileName = "Index";
	handle = fs->OpenChild(&desc);
	ASSERT((HANDLE)-1 != handle);

	int index;
	CHECK (fs->ReadFile (handle, &index, sizeof(index), &bytes_read));
	ASSERT(bytes_read == sizeof(index));

	CHECK (fs->CloseHandle (handle));

	//check to see if what is sought exists in the parts filesystem.
	//the parts filesystem is the directory in which "Cmpnd" was found.

	//if it exists in parts, then it should be used. otherwise use the 
	//filesystem that was passed to model as a parent.

	COMPTR<IFileSystem> file;

	desc.lpFileName = file_name;

	if (_parts==0 || _parts->CreateInstance(&desc, file) != GR_OK)
		engine->create_file_system(file_name, file);

	ARCHETYPE_INDEX archetype = engine->create_archetype (file_name, file);

	ASSERT (INVALID_ARCHETYPE_INDEX != archetype) ;

	return new Archetype (part_name, archetype);
}

#if SUPPORT_OLD_PARTNAME_FMT
	Archetype* CreateNamedArchetype (IFileSystem* fs, 
										HANDLE handle, 
										IEngine* engine, 
										IFileSystem* _parts)
	{
		//assume the entire file length is the part name + archetype filename
		//both zero terminated

		char arch_name[MAX_PATH + PARTNAME_MAX];

		DWORD filesize = fs->GetFileSize (handle);

		ASSERT (filesize < sizeof (arch_name));

		DWORD bytes_read;

		CHECK (fs->ReadFile (handle, arch_name, filesize, &bytes_read));

		ASSERT (bytes_read == filesize);

		//check to see if what is sought exists in the parts filesystem.
		//the parts filesystem is the directory in which "Cmpnd" was found.

		//if it exists in parts, then it should be used. otherwise use the 
		//filesystem that was passed to model as a parent.

		COMPTR<IFileSystem> file;
		DAFILEDESC desc (arch_name + PARTNAME_MAX);

//		if (_parts && _parts->GetFileAttributes (arch_name + PARTNAME_MAX) != 0xFFFFFFFF)
//			parts = _parts;
//		else
//			parts = create_archetype_parent;

		if (_parts==0 || _parts->CreateInstance(&desc, file) != GR_OK)
			engine->create_file_system(arch_name + PARTNAME_MAX, file);

		ARCHETYPE_INDEX archetype = engine->create_archetype (arch_name + PARTNAME_MAX, file);

		ASSERT (INVALID_ARCHETYPE_INDEX != archetype) ;
				
		return new Archetype (arch_name, archetype);
	}
#endif


/*
form Cmpnd
{
	Root
	{
		char object_name;
		char filename;
	}
	Part1
	{
		char object_name;
		char filename;
	}
	Part2
	{
		char object_name
		char filename
	}
	Cons
	{
		...
	}
}
*/
Archetype* Archetype::find_part (const char* partname)
{
	ASSERT (partname);

	Archetype* result = NULL;

	if (!strcmp (partname, ROOT_OBJ_NAME))
	{
		result = this;
	}
	else
	{
		ARCHETYPE_LIST::iterator it = children.begin ();

		while (it != children.end ())
		{
			if (!strncmp ((*it)->label, partname, PARTNAME_MAX))
			{
				result = *it;
				break;
			}
			else
				it++;
		}
	}

	return result;
}

void JointMessage(const void *persist_joint)
{
	const Fix *fix = (Fix*)persist_joint;

	char buffer[256];
	sprintf(buffer, "Joint parent=\"%s\" child=\"%s\" failed!", fix->parent, fix->child);
	MessageBox(NULL, buffer, "Message", MB_OK | MB_ICONSTOP);
}

int Archetype::read_sub_objects (IFileSystem* fs, IEngine* engine, IFileSystem* parts)
{
	int result = 0;

	WIN32_FIND_DATA find_data;

	char part_filter[32];
	strcpy (part_filter, PART_STEM);
	strcat (part_filter, "*");

	HANDLE srch = fs->FindFirstFile (part_filter, &find_data);

	if (INVALID_HANDLE_VALUE != srch)
	{
		do
		{
			Archetype* new_part = NULL;

			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				fs->SetCurrentDirectory (find_data.cFileName);
				new_part = CreateNamedArchetype (fs, engine, parts);
				fs->SetCurrentDirectory ("..");
			}
			else
			{
			#if SUPPORT_OLD_PARTNAME_FMT
				DAFILEDESC desc (find_data.cFileName);
				desc.dwDesiredAccess = GENERIC_READ;
				desc.dwCreationDistribution = OPEN_EXISTING;

				HANDLE handle = fs->OpenChild (&desc);

				ASSERT ((HANDLE)-1 != handle);

				new_part = CreateNamedArchetype (fs, handle, engine, parts);

				CHECK (fs->CloseHandle (handle));
			#else
				ASSERT (false && "expected file, found directory");
			#endif
			}

			ASSERT (new_part);
			children.push_back (new_part);

			if (!fs->FindNextFile (srch, &find_data))
			{
				int e = fs->GetLastError ();
				ASSERT (ERROR_NO_MORE_FILES == e);
				break;
			}
		}
		while (true);

		CHECK (fs->FindClose (srch));

		//pass 2:  build connections

		CHECK (fs->SetCurrentDirectory (CONNECTION_DIR_NAME));

		srch = fs->FindFirstFile ("*", &find_data);

		ASSERT (INVALID_HANDLE_VALUE != srch);

		//for all connection types

		do
		{
			if (find_data.cFileName[0] != '.')
			{
				ASSERT (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));

				DAFILEDESC desc (find_data.cFileName);

				HANDLE handle = fs->OpenChild (&desc);
				ASSERT (handle != (HANDLE)0xFFFFFFFF);

				DWORD filesize = fs->GetFileSize (handle, NULL);

				DWORD cur = fs->SetFilePointer (handle, 0, 0, FILE_CURRENT);

				//for all connections of a type

				while (cur < filesize)
				{
					ConnectionDefinition con;

					if (!strcmp (find_data.cFileName, JT_FIXED_NAME))
					{
						Fix in;
						
						DWORD outsize;

						CHECK (fs->ReadFile (handle, &in, sizeof (in), &outsize));
						ASSERT (outsize == sizeof (in));

						con.parent = find_part (in.parent);
#ifndef NDEBUG
						if(con.parent == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.parent);

						con.child = find_part (in.child);
#ifndef NDEBUG
						if(con.child == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.child);

						con.connection.type = JT_FIXED;

						JointInfo * tmp = (JointInfo *) &(con.connection);

						tmp->rel_position = in.pos;
						tmp->rel_orientation = in.orient;
					}
					else if (!strcmp (find_data.cFileName, JT_REVOLUTE_NAME))
					{
						Rev in;
						
						DWORD outsize;

						CHECK (fs->ReadFile (handle, &in, sizeof (in), &outsize));
						ASSERT (outsize == sizeof (in));

						con.parent = find_part (in.parent);
#ifndef NDEBUG
						if(con.parent == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.parent);

						con.child = find_part (in.child);
#ifndef NDEBUG
						if(con.child == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.child);

						con.connection.type = JT_REVOLUTE;

						JointInfo * tmp = (JointInfo *) &(con.connection);

						tmp->parent_point = in.parent_point;
						tmp->child_point = in.child_point;
						tmp->rel_orientation = in.rel_orientation;
						tmp->axis = in.axis;
						tmp->min0 = in.min;
						tmp->max0 = in.max;
					}
					else if (!strcmp (find_data.cFileName, JT_PRISMATIC_NAME))
					{
						Pris in;
						
						DWORD outsize;

						CHECK (fs->ReadFile (handle, &in, sizeof (in), &outsize));
						ASSERT (outsize == sizeof (in));

						con.parent = find_part (in.parent);
#ifndef NDEBUG
						if(con.parent == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.parent);

						con.child = find_part (in.child);
#ifndef NDEBUG
						if(con.child == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.child);

						con.connection.type = JT_PRISMATIC;

						JointInfo * tmp = (JointInfo *) &(con.connection);

						tmp->parent_point = in.parent_point;
						tmp->child_point = in.child_point;
						tmp->rel_orientation = in.rel_orientation;
						tmp->axis = in.axis;
						tmp->min0 = in.min;
						tmp->max0 = in.max;
					}
					else if (!strcmp (find_data.cFileName, JT_SPRING_NAME))
					{
						Spring in;
						
						DWORD outsize;

						CHECK (fs->ReadFile (handle, &in, sizeof (in), &outsize));
						ASSERT (outsize == sizeof (in));

						con.parent = find_part (in.parent);
#ifndef NDEBUG
						if(con.parent == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.parent);

						con.child = find_part (in.child);
#ifndef NDEBUG
						if(con.child == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.child);

						con.connection.type = JT_DAMPED_SPRING;

						JointInfo * tmp = (JointInfo *) &(con.connection);

						tmp->parent_point = in.parent_point;
						tmp->child_point = in.child_point;
						tmp->spring_constant = in.spring_constant;
						tmp->damping_constant = in.damping_constant;
						tmp->rest_length = in.rest_length;
					}
					else if (!strcmp(find_data.cFileName, JT_CYLINDRICAL_NAME))
					{
						Cyl in;
						
						DWORD outsize;

						CHECK (fs->ReadFile (handle, &in, sizeof (in), &outsize));
						ASSERT (outsize == sizeof (in));

						con.parent = find_part (in.parent);
#ifndef NDEBUG
						if(con.parent == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.parent);

						con.child = find_part (in.child);
#ifndef NDEBUG
						if(con.child == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.child);

						con.connection.type = JT_CYLINDRICAL;

						JointInfo * tmp = (JointInfo *) &(con.connection);

						tmp->parent_point = in.parent_point;
						tmp->child_point = in.child_point;
						tmp->rel_orientation = in.rel_orientation;
						tmp->axis = in.axis;
						tmp->min0 = in.min_trans;
						tmp->max0 = in.max_trans;

						tmp->min1 = in.min_rot;
						tmp->max1 = in.max_rot;
					}
					else if (!strcmp (find_data.cFileName, JT_SPHERICAL_NAME))
					{
						Sphere in;

						DWORD outsize;

						CHECK (fs->ReadFile (handle, &in, sizeof (in), &outsize));
						ASSERT (outsize == sizeof (in));

						con.parent = find_part (in.parent);
#ifndef NDEBUG
						if(con.parent == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.parent);

						con.child = find_part (in.child);
#ifndef NDEBUG
						if(con.child == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.child);

						con.connection.type = JT_SPHERICAL;

						JointInfo * tmp = (JointInfo *) &(con.connection);

						tmp->parent_point = in.parent_point;
						tmp->child_point = in.child_point;
						tmp->rel_orientation = in.rel_orientation;
						tmp->min0 = in.min_about_i;
						tmp->max0 = in.max_about_i;
						tmp->min1 = in.min_about_j;
						tmp->max1 = in.max_about_j;
						tmp->min2 = in.min_about_k;
						tmp->max2 = in.max_about_k;
					}
					else if (!strcmp (find_data.cFileName, JT_TRANSLATIONAL_NAME))
					{
						Trans in;

						DWORD outsize;

						CHECK (fs->ReadFile (handle, &in, sizeof (in), &outsize));
						ASSERT (outsize == sizeof (in));

						con.parent = find_part (in.parent);
#ifndef NDEBUG
						if(con.parent == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.parent);

						con.child = find_part (in.child);
#ifndef NDEBUG
						if(con.child == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.child);

						con.connection.type = JT_TRANSLATIONAL;

						JointInfo * tmp = (JointInfo *) &(con.connection);

						tmp->rel_position = in.pos;
						tmp->rel_orientation = in.orient;
					}
					else if (!strcmp (find_data.cFileName, JT_LOOSE_NAME))
					{
						Loose in;

						DWORD outsize;

						CHECK (fs->ReadFile (handle, &in, sizeof (in), &outsize));
						ASSERT (outsize == sizeof (in));

						con.parent = find_part (in.parent);
#ifndef NDEBUG
						if(con.parent == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.parent);

						con.child = find_part (in.child);
#ifndef NDEBUG
						if(con.child == NULL)
							JointMessage(&in);
#endif
						ASSERT (con.child);

						con.connection.type = JT_LOOSE;

						JointInfo * tmp = (JointInfo *) &(con.connection);

						tmp->rel_position = in.pos;
						tmp->rel_orientation = in.orient;
					}
					else
						ASSERT (false && "unknown joint type");

					connections.push_back (con);

					cur = fs->SetFilePointer (handle, 0, 0, FILE_CURRENT);
				}

				CHECK (fs->CloseHandle (handle));
			}

			if (!fs->FindNextFile (srch, &find_data))
			{
				int e = fs->GetLastError ();
				ASSERT (ERROR_NO_MORE_FILES == e);
				break;
			}
		}
		while (true);

		CHECK (fs->FindClose (srch));

		CHECK (fs->SetCurrentDirectory (".."));
	}

	return result;
}

void Archetype::decouple (IEngine* engine)
{
	if (engine)
		engine->release_archetype (archetype);
	archetype = INVALID_ARCHETYPE_INDEX;

	for (ARCHETYPE_LIST::iterator it = children.begin ();
		it != children.end ();
		it++)
	{
		(*it)->decouple (engine);
	}
}

Archetype* Archetype::BuildRoot (IFileSystem* fs, IEngine* engine, IFileSystem* parts)
{
	ASSERT (fs);
	ASSERT (engine);

	Archetype* result = NULL;

	DWORD attr = fs->GetFileAttributes (ROOT_OBJ_NAME);

	//If there is a Root chunk, then that contains an
	//archetype filename and a part name

	if (0xFFFFFFFF != attr)
	{
		if (attr & FILE_ATTRIBUTE_DIRECTORY)
		{
			//do new format
			fs->SetCurrentDirectory (ROOT_OBJ_NAME);
			result = CreateNamedArchetype (fs, engine, parts);
			fs->SetCurrentDirectory ("..");
		}
		else
		{
		#if SUPPORT_OLD_PARTNAME_FMT
			DAFILEDESC desc (ROOT_OBJ_NAME);
			desc.dwDesiredAccess = GENERIC_READ;
			desc.dwCreationDistribution = OPEN_EXISTING;

			HANDLE handle = fs->OpenChild (&desc);

			ASSERT ((HANDLE)-1 != handle);

			result = CreateNamedArchetype (fs, handle, engine, parts);
		#else
			ASSERT (!(attr & FILE_ATTRIBUTE_DIRECTORY));
		#endif
		}
	}
	else
	{
		//There is no named root object

		result = new Archetype (NULL_OBJECT_LABEL, INVALID_ARCHETYPE_INDEX);
		ASSERT (result);
	}

	return result;
}

Archetype* Archetype::Build (IFileSystem* fs, 
								IEngine* engine, 
								IFileSystem* parts)
{
	Archetype* root = BuildRoot (fs, engine, parts);
	root->read_sub_objects (fs, engine, parts);
	return root;
}

