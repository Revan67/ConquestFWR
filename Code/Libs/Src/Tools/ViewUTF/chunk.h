#ifndef CHUNK_H
#define CHUNK_H

#include "document.h"
#include "xfile.h"

//---------------------------------------------------------------------------
// Chunk
//---------------------------------------------------------------------------


struct Chunk
{
    #define MAX_NAME 128
    char name[MAX_NAME];

	CString type;

	int size;
    Chunk *next;			// Sibling
    Chunk *child;			// Child

	IDocument *doc;

    int modified;

	int read_levels;		// how deep has tree been parsed?

    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;

    void init (void)
    {
        size        = 0;
        next        = 0;
        child       = 0;
		doc         = 0;
        modified    = 0;
		read_levels = 0;
    }

    Chunk (void)
    {
        init();
    }

    void set_name (const char *new_name)
    {
        strcpy(name,new_name);
    }
    int is_named (const char *new_name)
    {
        return strcmp(name,new_name) == 0;
    }
    int rename (const char *new_name)
    {
        if (!is_named(new_name))
        {
			if (doc && doc->Rename(new_name))
			{
				set_name(new_name);
				modified = 1;
				return 1;
			}
        }
        return 0;
    }

    int is_folder (void)
    {
        return size == -1;
    }
    void set_folder (void)
    {
        size = -1;
    }
	void set_root (const char *n)
	{
		set_name(n);
		set_folder();
	}
	int is_root (void)
	{
		return is_folder() && (doc == 0);
	}

    void open (WIN32_FIND_DATA &item)
    {
		int dir = item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

		if (dir)
		{
			set_folder();
		}
		else
		{
			size = item.nFileSizeLow;
		}

        set_name(item.cFileName);
        ftCreationTime = item.ftCreationTime;
        ftLastAccessTime = item.ftLastAccessTime;
        ftLastWriteTime = item.ftLastWriteTime;
    }
    void close (void)
    {
		if (!is_root())
		{
			if (doc)
			{
				doc->Release();
				doc = 0;
			}
			size = 0;
		}
    }

    ~Chunk (void)
    {
        close();
    }

	int get_size (void)
	{
		int total = 0;
		if (is_folder())
		{
			for (Chunk *c=child; c; c=c->next)
			{
				total += c->get_size();
			}
		}
		else
		{
			total += size;
		}
		return total;
	}
	int get_count (void)
	{
		int count = 1;
		if (is_folder())
		{
			for (Chunk *c=child; c; c=c->next)
			{
				count += c->get_count();
			}
		}
		return count;
	}


	void copy (Chunk *src)
	{
		*this = *src;

		next = 0;
		child = 0;
	}

	int delete_doc_file (void)
	{
		int ok = FALSE;
		if (doc)
		{
			doc->CloseAllClients();

			IFileSystem *parent;
			doc->GetParentSystem(&parent);
			doc->Release();
			doc = 0;

			if (is_folder())
				ok = parent->RemoveDirectory(name);
			else
				ok = parent->DeleteFile(name);

			if (parent)
			{
				parent->Release();
			}
		}
		return ok;
	}

	void append_child (Chunk *kid)
	{
		Chunk *p = child;
		if (p == 0)
			child = kid;
		else
		{
			while (p->next)
				p = p->next;
			p->next = kid;
		}
	}

	bool export( IFileSystem& outfile );

	bool import( IFileSystem& infile );
};

//---------------------------------------------------------------------------

#endif // CHUNK_H
