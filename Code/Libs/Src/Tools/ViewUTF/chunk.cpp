#include "stdafx.h"
#include "chunk.h"

#include "document.h"
//#include "xfile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-----------------------------------------------------------------------------------------------------

bool Chunk::export( IFileSystem& outfile )
{
	if( is_folder() )
	{
		outfile.CreateDirectory(name);
		outfile.SetCurrentDirectory(name);

		Chunk* c = child;
		while(c)
		{
			c->export(outfile);
			c = c->next;
		}

		outfile.SetCurrentDirectory("..");
	}
	else
	{
		void* block = malloc( doc->GetFileSize() );
		DWORD dwNumRead = 0;
		if( doc->ReadFile(0, block, doc->GetFileSize(), &dwNumRead ) )
		{
			DAFILEDESC desc(name);
			desc.lpImplementation = "UTF";
			IFileSystem* file = FS_Create(&desc,&outfile);
			if( file )
			{
				file->WriteFile(0, block, dwNumRead, &dwNumRead);
				file->CloseHandle(0);
			}
		}
		delete [] block;
	}

	return 0;
}

//-----------------------------------------------------------------------------------------------------

bool Chunk::import( IFileSystem& infile )
{
	return 0;
}
