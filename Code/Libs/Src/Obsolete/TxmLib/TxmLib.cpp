//$Header: /Libs/Src/TXMLib/TxmLib.cpp 27    8/20/98 4:30p Pbleisch $
//Copyright 1997 (c) Digital Anvil, Inc.

#include "itxmlib.h"
#include "TXMLib.h"
#include "TSmartPointer.h"
#include "fdump.h"
#include <stdio.h>

#include "rendpipeline.h"

IRenderPipeline *PIPE = NULL;

#if !USE_NWO
	#include "Display.h"
#endif

// ---------------------------------------------------

U32 Entry::hash (const void *object)
{
	U8 sum;

	U8 *ptr = (U8 *) object;
	sum = 0;

	while (*ptr)
	{
		sum += *(ptr++);
	}

	sum &= 0xff;

	return sum;
}

inline BOOL32 Entry::compare (const void *object)
{
	return !stricmp (name, (C8 *) object);
}

void Entry::initialize (const void *object)
{
	strcpy(name, (C8 *) object);
	references = 0;
	alpha = false;

#if !USE_NWO
	if( !PIPE ) {
		u_mode = GL_REPEAT;
		v_mode = GL_REPEAT;
	}
	else
#endif
	{
		u_mode = D3DTADDRESS_WRAP;
		v_mode = D3DTADDRESS_WRAP;
	}
}

void Entry::shutdown(void)
{
}

void Entry::display(void)
{
}

// ---------------------------------------------------

// axms' handles unlike txm's are 
// directely related to the entry/hash index
#define AXM2IDX(a) ((a)-1)
#define IDX2AXM(i) ((i)+1)

U32 AXMEntry::hash (const void *object)
{
	U8 sum;

	U8 *ptr = (U8 *) object;
	sum = 0;

	while (*ptr)
	{
		sum += *(ptr++);
	}

	sum &= 0xff;

	return sum;
}

inline BOOL32 AXMEntry::compare (const void *object)
{
	return !stricmp (name, (C8 *) object);
}

void AXMEntry::initialize (const void *object)
{
	strcpy(name, (C8 *) object);

	references = 0;
	frame_count= 0;
}

void AXMEntry::shutdown(void)
{
	//ASSERT(references == 0);

	if (txmids)
	{
		delete txmids;
		txmids= 0;
	}
	
	if (frames)
	{
		delete frames;
		frames= 0;
	}
}

void AXMEntry::display(void)
{
	// DumpText-> names ?
}

// ---------------------------------------------------

TXMLib::TXMLib (void)
{
	load_mipmaps = true;
	use_mipmaps = true;
}

void TXMLib::reset (void)
{
	// WARNING: This is assumed to be called after the pipeline has lost all of its
	// textures, so it does not perform the normal texture unload; it simply clears
	// the list data it has. If you call this while the pipeline still owns the textures,
	// the pipeline will still have the texture data, which becomes orphaned.

	// Unfortunately, I cannot call free() on the lists, because doing so releases all of the
	// memory. Instead, we will go through and unlink every possible index in both pools, then
	// reset both lists.

	S32 i;
	for (i = 0; i < axms.list_size; ++i)
	{
		axms.unlink (i);
	}
	axms.reset ();

	for (i = 0; i < entries.list_size; ++i)
	{
		entries.unlink (i);
	}
	entries.reset ();
}

int TXMLib::load_library (IFileSystem* inFile)
{
	ASSERT (inFile);
	int result = 0;
	int axmresult = 0; // not returned
	DAFILEDESC fdesc = "Texture library";

	if (inFile->SetCurrentDirectory ("Texture library"))
	{
		WIN32_FIND_DATA find_data;
		HANDLE srch;

		if (INVALID_HANDLE_VALUE != (srch = inFile->FindFirstFile ("*.*", &find_data)))
		{
			do
			{
				ASSERT (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

				if (find_data.cFileName[0] != '.')
				{
					load_texture (find_data.cFileName, inFile);
					result++;
				}

				if (!inFile->FindNextFile (srch, &find_data))
				{
					ASSERT (ERROR_NO_MORE_FILES == inFile->GetLastError ());
					break;
				}
			}
			while (true);

			inFile->FindClose (srch);
		}

		inFile->SetCurrentDirectory ("..");
	}

	if (inFile->SetCurrentDirectory ("Animation library"))
	{
		WIN32_FIND_DATA find_data;
		HANDLE srch;

		if (INVALID_HANDLE_VALUE != (srch = inFile->FindFirstFile ("*.*", &find_data)))
		{
			do
			{
				ASSERT (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

				if (find_data.cFileName[0] != '.')
				{
					load_axm (find_data.cFileName, inFile);
					axmresult++;
				}

				if (!inFile->FindNextFile (srch, &find_data))
				{
					ASSERT (ERROR_NO_MORE_FILES == inFile->GetLastError ());
					break;
				}
			}
			while (true);

			inFile->FindClose (srch);
		}

		inFile->SetCurrentDirectory ("..");
	}

	return result;
}

extern int SubmitMIPTexture (IFileSystem* fs, bool & alpha, bool load_mipmaps, bool use_mipmaps,
							 int & u_mode, int & v_mode);

TXM_ID TXMLib::load_texture (const char* _fname, IFileSystem* parent)
{
	char check_name[MAX_PATH];

	{
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		_splitpath (_fname, NULL, NULL, fname, ext);
		_makepath (check_name, NULL, NULL, fname, ext);
	}

	TXM_ID result = INVALID_TXM_ID;
	int idx(entries.search (check_name));

	if (idx == -1)
	{
		IComponentFactory* comp = parent;

		if (!parent)
		{
			comp = DACOM_Acquire ();
			ASSERT (comp);
		}

		COMPTR<IFileSystem> fs;

		DAFILEDESC desc (_fname);

		if (comp->CreateInstance (&desc, fs) == GR_OK)
		{
			int entry_index = entries.allocate(check_name);

#if !USE_NWO
			if( !PIPE ) {
				GLuint id;
				glGenTextures (1, &id);
				glBindTexture(GL_TEXTURE_2D, id);

				result = entries.list[entry_index].id = id;
			}
			else
#endif
			{
				U32 tid;
				PIPE->alloc_texture( tid);
				PIPE->set_render_state( D3DRS_TEXTUREHANDLE, tid );

				result = entries.list[entry_index].id = tid;
			}

			int num_levels = SubmitMIPTexture (fs, entries.list[entry_index].alpha, load_mipmaps, use_mipmaps,
								entries.list[entry_index].u_mode, entries.list[entry_index].v_mode);
			if (num_levels > 1)
			{
				entries.list[entry_index].min_filter = 
			#if !USE_NWO
				(!PIPE) ? GL_LINEAR_MIPMAP_NEAREST : 
			#endif
				D3DFILTER_MIPLINEAR;
			}
			else
			{
				entries.list[entry_index].min_filter = 
			#if !USE_NWO
				(!PIPE) ? GL_LINEAR :
			#endif
				D3DFILTER_LINEAR;
			}
		}
	}

	return result;
}

void TXMLib::unload_texture (unsigned int index)
{
	ASSERT (entries.list[index].index != -1);
		
	TXM_ID togo = entries.list[index].id;

#if !USE_NWO
	if( !PIPE ) {
		GLuint tg = togo;
		glDeleteTextures (1, &tg);
	}
	else
#endif
		PIPE->destroy_texture( togo );

	entries.list[index].id = INVALID_TXM_ID;

	entries.unlink (index);
}

AXM_HANDLE TXMLib::load_axm ( const char* _fname, IFileSystem* parent )
{
	char check_name[MAX_PATH];

	{
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		_splitpath (_fname, NULL, NULL, fname, ext);
		_makepath (check_name, NULL, NULL, fname, ext);
	}

	int idx= axms.search (check_name);
	AXM_HANDLE result = IDX2AXM(idx);

	if (idx == -1)
	{
		IComponentFactory* comp = parent;

		if (!parent)
		{
			comp = DACOM_Acquire ();
			ASSERT (comp);
		}

		COMPTR<IFileSystem> fs;

		DAFILEDESC desc (_fname);

		if (comp->CreateInstance (&desc, fs) == GR_OK)
		{
			HANDLE h;
			unsigned long read;
			int r;

			idx = axms.allocate(check_name);
			ASSERT( idx != -1);
			
			AXMEntry * entry = &axms.list[idx];
			result = IDX2AXM( idx );

		// texture indexing count
			desc.lpFileName = "Texture count";
			h= fs->OpenChild (&desc);
			ASSERT( h != INVALID_HANDLE_VALUE );

			r= fs->ReadFile (h, &entry->txm_count, 4, &read, NULL);
			ASSERT(r);
			fs->CloseHandle (h);

			entry->txmids = new TXM_ID[entry->txm_count];
			memset( entry->txmids, INVALID_TXM_ID, sizeof(TXM_ID) * entry->txm_count );

		// frame count
			desc.lpFileName = "Frame count";
			h= fs->OpenChild (&desc);
			ASSERT( h != INVALID_HANDLE_VALUE );

			r= fs->ReadFile (h, &entry->frame_count, 4, &read, NULL);
			ASSERT(r);
			fs->CloseHandle (h);

			entry->frames = new AXMEntry::FRAME_RECT[entry->frame_count];

		// data
			desc.lpFileName = "Frame rects";
			h= fs->OpenChild (&desc);
			ASSERT( h != INVALID_HANDLE_VALUE );

			r= fs->ReadFile (h, entry->frames, sizeof(AXMEntry::FRAME_RECT) * entry->frame_count, &read, NULL);
			ASSERT(r);
			fs->CloseHandle (h);
		}
	}

	return result;
}

bool TXMLib::inc_axm_ref (S32 idx)
{
	if ((idx >= 0) && (idx < axms.list_size))
	{
		if (axms.list[idx].index != -1)
		{
			AXMEntry *axm = &( axms.list[idx] );

			// for a failed attempt it _does_ get decremented
			++axm->references;

			for (int i=0; i< axm->txm_count; i++)
			{
				if (axm->txmids[i] != INVALID_TXM_ID)
				{
					add_ref( axm->txmids[i] );
				}
				else
				{
					char texname[_MAX_FNAME + _MAX_EXT];

					// create the extented texture name from the animation base name
					// and the frames' texture extension
					_snprintf( texname, _MAX_FNAME + _MAX_EXT, "%s_%d", axm->name, axm->frames[i].texext );

					// error checking and ref cleanup
					if ( INVALID_TXM_ID == (axm->txmids[i] = get_texture_id( texname )) )
					{
						dec_axm_ref( idx, i );
						return false;
					}
				}
			}

			return true;
		}
	}

	return false;
}

void TXMLib::dec_axm_ref (S32 idx, U32 up_to)
{
	if ((idx >= 0) && (idx < axms.list_size))
	{
		if (axms.list[idx].index != -1)
		{
			AXMEntry *axm = &(axms.list[idx]);
			ASSERT (axm->references > 0);

			// allows passing -1 to clear the whole axm
			// allows a failed increment to down only the part it uped
			if (up_to > U32(axm->txm_count))
				up_to = U32(axm->txm_count);
			
			for (U32 i=0; i< up_to; i++)
			{
				release_texture( axm->txmids[i] );
			}

			if (--axm->references == 0)
			{
				axms.unlink (idx);
			}
		}
	}
}

TXM_ID TXMLib::get_texture_id (const char* fname) const
{
	TXM_ID result = INVALID_TXM_ID;

	S32 idx = entries.search (fname);

	if (idx != -1)
	{
		result = entries.list[idx].id;
		entries.list[idx].references++;
	}
	else
	{
		char buf[128];
		sprintf (buf, "Texture \"%s\" not found.\n", fname);
		GENERAL_TRACE_1(buf);
	}

	return result;
}

AXM_HANDLE TXMLib::get_axm_handle (const char* fname)
{
	S32 idx = axms.search (fname);

	if (inc_axm_ref( idx ))
	{
		return IDX2AXM(idx);
	}

	{
		char buf[128];
		sprintf (buf, "AXM \"%s\" not found.\n", fname);
		GENERAL_TRACE_1(buf);
	}

	return INVALID_AXM_HANDLE;
}

//

void TXMLib::add_ref (const TXM_ID& texture)
{
	if (texture != INVALID_TXM_ID)
	{
		for (unsigned int i = 0; i < U32(entries.list_size); i++)
		{
			Entry *entry = &(entries.list[i]);
			if (entry->index != -1 && entry->id == texture)
			{
				entry->references++;
				break;
			}
		}
	}
}

void TXMLib::add_axm_ref (const AXM_HANDLE& haxm)
{
	inc_axm_ref(AXM2IDX(haxm));
}

//

void TXMLib::release_texture (const TXM_ID& texture)
{
	if (texture != INVALID_TXM_ID)
	{
		for (int i = 0; i < entries.list_size; i++)
		{
			if (entries.list[i].index != -1 && entries.list[i].id == texture)
			{
				if (--entries.list[i].references == 0)
				{
					unload_texture (i);
				}

				break;
			}
		}
	}
}

void TXMLib::release_axm (const AXM_HANDLE& haxm)
{
	dec_axm_ref(AXM2IDX(haxm));
}

void TXMLib::collect_garbage (void)
{
	// best to unload axms first
	for (int i=0; i < axms.list_size; i++)
	{
		if ((axms.list[i].index != -1) && (axms.list[i].references == 0))
		{
			// AXMEntry unlink does all unloading necessary
			axms.unlink (i);
		}
	}
	
	for (i = 0; i < entries.list_size; i++)
	{
		if (entries.list[i].index != -1)
		{
			if (entries.list[i].references == 0)
				unload_texture (i);
		}
	}
}

void TXMLib::get_texture_name(TXM_ID idx, char * buffer)
{
	ASSERT ((idx != INVALID_TXM_ID) && (buffer != NULL));

	for (int i = 0; i < entries.list_size; i++)
	{
		if (entries.list[i].id == idx)
		{
			strcpy( buffer, entries.list[i].name);
			break;
		}
	}
}

void TXMLib::set_texture_id (const char* txm_name, TXM_ID id)
{
	ASSERT (txm_name && id != INVALID_TXM_ID && id != -1);
	char check_name[MAX_PATH];

	{
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		_splitpath (txm_name, NULL, NULL, fname, ext);
		_makepath (check_name, NULL, NULL, fname, ext);
	}

	S32 idx = entries.search (check_name);

	//
	// already in the list, remove it
	//

	if (idx != -1)
	{
		if (entries.list[idx].id != id)
		{
			TXM_ID togo = entries.list[idx].id;

#if !USE_NWO
			if( !PIPE ) {
				GLuint tg = togo;
				glDeleteTextures(1, &tg);
			}
			else
#endif
				PIPE->destroy_texture( togo );

			entries.list[idx].id = id;


#if !USE_NWO
			int min_filter;
			if( !PIPE ) {
				glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min_filter);
			}
			else
#endif
//				PIPE->get_texture_render_state( RP_CURRENT, D3DRS_TEXTUREMIN, (U32*)&min_filter );

		// Save correct minification filter for later.
			entries.list[idx].min_filter = D3DFILTER_LINEAR; //min_filter;
		}

//		entries.list[idx].references++;
	}
	else
	{
		int index = entries.allocate(check_name);
		entries.list[index].id = id;


#if !USE_NWO
		int min_filter;
		if( !PIPE ) {
			glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min_filter);
		}
		else
#endif
//			PIPE->get_texture_render_state( RP_CURRENT, D3DRS_TEXTUREMIN, (U32*)&min_filter );

	// Save correct minification filter for later.
		entries.list[index].min_filter = D3DFILTER_LINEAR; //min_filter;
	}
}

//

void COMAPI TXMLib::enable(int state)
{
	switch (state)
	{
		case TXM_LOAD_MIPMAPS:
			load_mipmaps = true;
			break;

		case TXM_USE_MIPMAPS:
			if (!use_mipmaps)
			{
			// restore filter modes.
				for (int i = 0; i < entries.list_size; i++)
				{
					if (entries.list[i].index != -1)
					{
						TXM_ID id = entries.list[i].id;
						
					#if !USE_NWO
						if( !PIPE ) {
							if (glIsTexture(id))
							{
								glBindTexture(GL_TEXTURE_2D, id);
								int min_filter = entries.list[i].min_filter;
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);

							}
						}
						else 
					#endif
						{
#if IRP_NO_TXM_STATE
							if( PIPE->is_texture( id ) == GR_OK ) 
							{
								PIPE->set_render_state( D3DRS_TEXTUREHANDLE, id );
								int min_filter = entries.list[i].min_filter;
								PIPE->get_texture_render_state( RP_CURRENT, D3DRS_TEXTUREMIN, (U32*)&min_filter );
							}
#endif
						}
					}
				}

				use_mipmaps = true;
			}
			break;
	}
}

//

void COMAPI TXMLib::disable(int state)
{
	switch (state)
	{
		case TXM_LOAD_MIPMAPS:
			load_mipmaps = false;
			break;

		case TXM_USE_MIPMAPS:
			if (use_mipmaps)
			{
			// save and modify filter modes.
				for (int i = 0; i < entries.list_size; i++)
				{
					if (entries.list[i].index != -1)
					{
						TXM_ID id = entries.list[i].id;

					#if !USE_NWO
						if( !PIPE ) {
							if (glIsTexture(id))
							{
								glBindTexture(GL_TEXTURE_2D, id);
							/*
								int min_filter;
								glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min_filter);

							// Save correct minification filter.
								entries.list[i].min_filter = min_filter;
							*/
								switch (entries.list[i].min_filter)
								{
									case GL_NEAREST_MIPMAP_NEAREST:
									case GL_NEAREST_MIPMAP_LINEAR:
										glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
										break;

									case GL_LINEAR_MIPMAP_NEAREST:
									case GL_LINEAR_MIPMAP_LINEAR:
										glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
										break;
								}
							}
						}
						else
					#endif
						{
#if IRP_NO_TXM_STATE
							if( PIPE->is_texture( id ) == GR_OK ) 
							{
								PIPE->set_render_state( D3DRS_TEXTUREHANDLE, id );
								switch (entries.list[i].min_filter)
								{
									case D3DFILTER_MIPNEAREST:
									case D3DFILTER_MIPLINEAR:
										PIPE->set_texture_render_state( RP_CURRENT, D3DRS_TEXTUREMIN, D3DFILTER_NEAREST );
										break;

									case D3DFILTER_LINEARMIPNEAREST:
									case D3DFILTER_LINEARMIPLINEAR:
										PIPE->set_texture_render_state( RP_CURRENT, D3DRS_TEXTUREMIN, D3DFILTER_LINEAR );
										break;
								}
							}
#endif
						}
					}
				}

				use_mipmaps = false;
			}
			break;
	}
}

//

BOOL32 COMAPI TXMLib::is_enabled(int state)
{
	BOOL32 result = FALSE;
	switch (state)
	{
		case TXM_LOAD_MIPMAPS:
			result = load_mipmaps;
			break;

		case TXM_USE_MIPMAPS:
			result = use_mipmaps;
			break;
	}
	return result;
}

//

BOOL32 COMAPI TXMLib::is_indexed(TXM_ID id)
{
	BOOL32 result = 0;

#if !USE_NWO
	if( !PIPE ) {

	// Save current binding.
		int save;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &save);

	// Bind new texture, query internal format.
		glBindTexture(GL_TEXTURE_2D, id);
		GLenum format;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, (int *) &format);
	// Currently only indexed mode supported is 8-bit.
		result = (format == GL_COLOR_INDEX8_EXT);

	// restore binding.
		glBindTexture(GL_TEXTURE_2D, save);
	}
	else
#endif
	{
		PixelFormat pf;

		if( PIPE->get_texture_format( id, &pf ) == GR_OK ) {
			result = pf.is_indexed();
		}
	}

	return result;
}

//

int COMAPI TXMLib::get_red_bits(TXM_ID id)
{
	int result;

#if !USE_NWO
	if( !PIPE ) {
		int save;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &save);

		glBindTexture(GL_TEXTURE_2D, id);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_RED_SIZE, &result);

		glBindTexture(GL_TEXTURE_2D, save);
	}
	else
#endif
	{
		PixelFormat pf;

		if( PIPE->get_texture_format( id, &pf ) == GR_OK ) {
			result = pf.rwidth;
		}
	}

	return result;
}

//

int COMAPI TXMLib::get_green_bits(TXM_ID id)
{
	int result;

#if !USE_NWO
	if( !PIPE ) {
		int save;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &save);

		glBindTexture(GL_TEXTURE_2D, id);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_GREEN_SIZE, &result);

		glBindTexture(GL_TEXTURE_2D, save);
	}
	else
#endif
	{
		PixelFormat pf;

		if( PIPE->get_texture_format( id, &pf ) == GR_OK ) {
			result = pf.gwidth;
		}
	}

	return result;
}

//

int COMAPI TXMLib::get_blue_bits(TXM_ID id)
{
	int result;

#if !USE_NWO
	if( !PIPE ) {
		int save;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &save);

		glBindTexture(GL_TEXTURE_2D, id);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_BLUE_SIZE, &result);

		glBindTexture(GL_TEXTURE_2D, save);
	}
	else
#endif
	{
		PixelFormat pf;

		if( PIPE->get_texture_format( id, &pf ) == GR_OK ) {
			result = pf.bwidth;
		}
	}

	return result;
}

//

int COMAPI TXMLib::get_alpha_bits(TXM_ID id)
{
	int result;

#if !USE_NWO
	if( !PIPE ) {
		int save;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &save);

		glBindTexture(GL_TEXTURE_2D, id);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &result);

		glBindTexture(GL_TEXTURE_2D, save);
	}
	else
#endif
	{
		PixelFormat pf;

		if( PIPE->get_texture_format( id, &pf ) == GR_OK ) {
			result = pf.awidth;
		}
	}

	return result;
}

//

int COMAPI TXMLib::get_width(TXM_ID id, int mipmap_level)
{
	int result;

#if !USE_NWO
	if( !PIPE ) {
		int save;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &save);

		glBindTexture(GL_TEXTURE_2D, id);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, mipmap_level, GL_TEXTURE_WIDTH, &result);

		glBindTexture(GL_TEXTURE_2D, save);
	}
	else 
#endif
		PIPE->get_texture_dim( id, (U32*)&result, NULL, NULL );

	return result;
}

//

int COMAPI TXMLib::get_height(TXM_ID id, int mipmap_level)
{
	int result;

#if !USE_NWO
	if( !PIPE ) {
		int save;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &save);

		glBindTexture(GL_TEXTURE_2D, id);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, mipmap_level, GL_TEXTURE_HEIGHT, &result);

		glBindTexture(GL_TEXTURE_2D, save);
	}
	else
#endif
		PIPE->get_texture_dim( id, NULL, (U32*)&result, NULL );

	return result;
}

//

TXM_ID COMAPI TXMLib::get_entry_id(int id) const
{
	if((id >= 0) && (id < entries.list_size))
	{
		for (int i = 0; i < entries.list_size; i++)
		{
			if(entries.list[i].index != -1)
			{
				if(id > 0)
				{
					id--;
				}
				else
				{	
					return entries.list[i].id;
				}
			}
		}
	}

	return INVALID_TXM_ID;
}

//

void TXMLib::get_axm_name(AXM_HANDLE haxm, const char * buffer) const
{
	if (buffer)
	{
		*((char*) buffer)=0;

		int idx = AXM2IDX(haxm);

		if ((idx >= 0) && (idx < axms.list_size))
		{
			if (axms.list[idx].index != -1)
			{
				strcpy((char *) buffer, axms.list[idx].name);
			}
		}
	}
}

void TXMLib::get_axm_frame(AXM_HANDLE haxm, int framenum, AXM_RECT & rect ) const
{
	int idx = AXM2IDX(haxm);

	ASSERT ((idx >= 0) && (idx < axms.list_size));
		
	AXMEntry * axm = &axms.list[idx];
	ASSERT(axm->index != -1);

	AXMEntry::FRAME_RECT * frame= &axm->frames[framenum % axm->frame_count];
	
	rect.txmid= axm->txmids[ frame->texext ];
	rect.u0= frame->u0;
	rect.v0= frame->v0;
	rect.u1= frame->u1;
	rect.v1= frame->v1;
}

int TXMLib::get_axm_frame_count( AXM_HANDLE haxm ) const
{
	int idx = AXM2IDX(haxm);
	
	if ((idx >= 0) && (idx < axms.list_size))
	{
		if (axms.list[idx].index != -1)
		{
			return axms.list[idx].frame_count;
		}
	}

	return 0;
}

int TXMLib::get_txm_count( void ) const
{
	int count = 0;
	
	for (int i=0; i< entries.list_size; i++)
	{
		count += (entries.list[i].index != -1);
	}

	return count;
}

int TXMLib::get_axm_count( void ) const
{
	int count = 0;
	
	for (int i=0; i< axms.list_size; i++)
	{
		count += (axms.list[i].index != -1);
	}

	return count;
}

void TXMLib::enumerate_txms( TAXM_ENUM_CALLBACK cbf, void *context ) const
{
	if (cbf)
	{
		for (int i=0; i< entries.list_size; i++)
		{
			if (entries.list[i].index != -1)
				cbf( entries.list[i].name, context );
		}
	}
}

void TXMLib::enumerate_axms( TAXM_ENUM_CALLBACK cbf, void *context ) const
{
	if (cbf)
	{
		for (int i=0; i< axms.list_size; i++)
		{
			if (axms.list[i].index != -1)
				cbf( axms.list[i].name, context );
		}
	}
}
