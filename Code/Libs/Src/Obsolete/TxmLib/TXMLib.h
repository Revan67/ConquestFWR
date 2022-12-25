//$Header: /Libs/Src/TXMLib/TXMLib.h 18    8/07/98 6:21p Pbleisch $
//Copyright 1997 (c) Digital Anvil, Inc.

#include "stddat.h"
#include <windows.h>
#include "FileSys.h"

#include "rendpipeline.h"

#ifndef TXM_ID_DEFINED
	#define TXM_ID_DEFINED
	
	typedef int TXM_ID;
	const TXM_ID INVALID_TXM_ID = 0;

	typedef S32 AXM_HANDLE;
	const AXM_HANDLE INVALID_AXM_HANDLE = 0;

	struct AXM_RECT
	{
		TXM_ID txmid;
		float u0, v0, u1, v1;
	};

	typedef void (__cdecl * TAXM_ENUM_CALLBACK) ( const char* taxm_name, void *context );
#endif

struct Entry
{
	//BEGIN HashPool stuff
	U32 hash_key;

	Entry *hash_next; 
	Entry *hash_prev;

	Entry *next;
	Entry *prev;

	S32 index;

	static U32 hash (const void *object);

	BOOL32 compare (const void *object);
	void initialize(const void *object);
	void shutdown(void);
	void display(void);
	//END HashPool stuff

	TXM_ID id;
	unsigned long references;

	int min_filter;
	bool alpha;

	char name[_MAX_FNAME + _MAX_EXT];

	int u_mode;
	int v_mode;

	Entry (void) 
	{ 
		id = INVALID_TXM_ID; 
	}
};

struct AXMEntry
{
	//BEGIN HashPool stuff
	U32 hash_key;

	AXMEntry *hash_next; 
	AXMEntry *hash_prev;

	AXMEntry *next;
	AXMEntry *prev;

	S32 index;

	static U32 hash (const void *object);

	BOOL32 compare (const void *object);
	void initialize(const void *object);
	void shutdown(void);
	void display(void);
	//END HashPool stuff

	unsigned long references;

	char name[_MAX_FNAME + _MAX_EXT];
	
	int frame_count;
	int txm_count;

	struct FRAME_RECT
	{
		// texture name extention
		long texext;
		float u0, v0, u1, v1;
	};

	FRAME_RECT * frames;
	TXM_ID * txmids;

	AXMEntry (void)
	{
		frames = 0;
		txmids = 0;
	}
};

extern IRenderPipeline *PIPE;

class TXMLib
{
	HashPool<Entry, 32> entries;
	HashPool<AXMEntry, 32> axms;

	bool load_mipmaps;
	bool use_mipmaps;

	TXM_ID load_texture (const char* fname, IFileSystem* fs);
	void unload_texture (unsigned int index);

	AXM_HANDLE load_axm ( const char* fname, IFileSystem* fs );
	bool inc_axm_ref( S32 index );
	void dec_axm_ref( S32 index, U32 upto = -1 );

protected:

	public:

		TXMLib (void);

		//either 'fname' or 'parent' can be NULL but not both
		int load_library (IFileSystem* file);

		//increments reference count for texture, if found.
		TXM_ID get_texture_id (const char* txm_name) const;

		//increments ref count for all textures in the axm, if all found
		//currently: a texture's ref count gets inc'd by the number of times
		// the same number of time the texture is used in the animation
		AXM_HANDLE get_axm_handle (const char* axm_name);

		//increments reference count by 1, even if you don't know the texture name.
		virtual void COMAPI add_ref(const TXM_ID& id);

		//increments reference count by 1, even if you don't know the axm name.
		void add_axm_ref(const AXM_HANDLE& handle);

		//decrements reference count for texture, unloads when count becomes 0.
		void release_texture (const TXM_ID& txm_id);

		//decrements ref count for axm; unloads when count reaches zero
		void release_axm (const AXM_HANDLE& axm_handle);

		//collect garbage once all texture ids have been obtained from all loaded libraries.
		//'collect_garbage ()' will unload all textures and animations that are not referred to.
		void collect_garbage (void);

		// adds a texture generated outside of txmlib to the library
		// if 'txm_name' already exists, it replaces the original texture.
		// the new texture has a reference count of 1+reference count of original texture.
		virtual void COMAPI set_texture_id (const char* txm_name, TXM_ID id);

        virtual void COMAPI get_texture_name(TXM_ID id, char * buffer);

		virtual void COMAPI enable(int state);
		virtual void COMAPI disable(int state);
		virtual BOOL32 COMAPI is_enabled(int state);

		virtual BOOL32 COMAPI is_indexed(TXM_ID id);

		virtual int COMAPI get_red_bits(TXM_ID id);
		virtual int COMAPI get_green_bits(TXM_ID id);
		virtual int COMAPI get_blue_bits(TXM_ID id);
		virtual int COMAPI get_alpha_bits(TXM_ID id);

		virtual int COMAPI get_width(TXM_ID id, int mipmap_level = 0);
		virtual int COMAPI get_height(TXM_ID id, int mipmap_level = 0);

		// get texture id of ith entry
		virtual TXM_ID COMAPI get_entry_id(int id) const;

		void get_axm_name(AXM_HANDLE handle, const char * buffer) const;
		void get_axm_frame(AXM_HANDLE handle, int frame, AXM_RECT & rect ) const;
		int  get_axm_frame_count( AXM_HANDLE handle ) const;

		// returns the number of loaded textures;
		// this isnt the number of allocted blocks in the entries list;
		// so it involves a list traversal
		int get_txm_count( void ) const;
		int get_axm_count( void ) const;

		void enumerate_txms( TAXM_ENUM_CALLBACK, void *context=NULL ) const;
		void enumerate_axms( TAXM_ENUM_CALLBACK, void *context=NULL ) const;

		// Reset this object back to it initial, no-textures-loaded state.
		void reset (void);
};
