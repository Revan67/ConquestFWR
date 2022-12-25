//---------------------------------------------------------------------------
//
// TYPELIST.H
//
//
//---------------------------------------------------------------------------


#ifndef TYPELIST_H
#define TYPELIST_H

//---------------------------------------------------------------------------
// TypeList
//---------------------------------------------------------------------------

#define	MAX_TYPES 512

struct TypeList
{
	int			count;

	int			size_list[MAX_TYPES];

	CStringList	name_list;

	TypeList (void)
	{
		count = 0;
	}

	void reset (void)
	{
		count = 0;
		name_list.RemoveAll();
	}

	void add (const char *name, int size)
	{
		size_list[count] = size;
		name_list.AddTail(name);
		count++;
	}

	int get_size (const char *name)
	{
		int size = -1;
		for (int i=0; i<count; i++)
		{
			POSITION pos = name_list.FindIndex(i);
			if (pos && name_list.GetAt(pos) == name)
			{
				size = size_list[i];
				break;
			}
		}
		return size;
	}
};

extern TypeList TheTypeList;

//---------------------------------------------------------------------------

CString GetTypeName (const char *name);

void ParseMemory (const char *memory, unsigned int size);
void ParseFile (const char *filename);
void ViewMemory(const char* type, void* memory);

//---------------------------------------------------------------------------

#endif // TYPELIST_H

