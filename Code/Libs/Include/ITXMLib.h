#ifndef ITXMLIB_H_
#define ITXMLIB_H_
//--------------------------------------------------------------------------//
//                                                                          //
//                               ITXMLib.h                                  //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $

   $Header: /Conquest/Libs/Include/ITXMLib.h 2     12/21/99 5:05p Rmarr $
*/
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include "DACOM.h"
#endif

struct IFileSystem;

typedef int TXM_ID;
const TXM_ID INVALID_TXM_ID = 0;

typedef int AXM_HANDLE;
const AXM_HANDLE INVALID_AXM_HANDLE = 0;

struct AXM_RECT
{
	TXM_ID txmid;
	float u0, v0, u1, v1;
};

#define TXM_ID_DEFINED

//
// enable()/disable() states for ITXMLib:
//
#define TXM_LOAD_MIPMAPS	1	// default = TRUE
#define TXM_USE_MIPMAPS		2	// default = TRUE

// callback function used for both txm and axm enumerations
typedef void (__cdecl * TAXM_ENUM_CALLBACK) ( const char* taxm_name, void *context );

//

#define IID_ITXMLib MAKE_IID("ITXMLib",1)

struct DACOM_NO_VTABLE ITXMLib : public IDAComponent
{
	//Will look for the "Texture library" in the current directory.
	virtual int COMAPI load_library (IFileSystem* file) = 0;

	//increments reference count by 1
	virtual TXM_ID COMAPI get_texture_id (const char* txm_name) const = 0;

	// increments reference count by 1, even if you don't know the texture name.
	virtual void COMAPI add_ref(const TXM_ID& id) = 0;

	//decrements reference count by 1 and unloads when count == 0.
	virtual void COMAPI release_texture (const TXM_ID& id) = 0;

	//'collect_garbage ()' removes all textures that have a reference count of 0.
	//after loading libraries and obtaining all texture ids necessary, a call to
	//'collect_garbage ()' will remove any unused textures and animations.
	virtual void COMAPI collect_garbage (void) = 0;

	// adds a texture generated outside of txmlib to the library
	// if 'txm_name' already exists, it replaces the original texture.
	// The reference count of the texture remains unchanged. If a new texture was added,
	// its reference count is 0.
	virtual void COMAPI set_texture_id (const char* txm_name, TXM_ID id) = 0;

    // return the name of a texture, given its texture id
	virtual void COMAPI get_texture_name(TXM_ID id, char * buffer) = 0;

	virtual void COMAPI enable(int state) = 0;
	virtual void COMAPI disable(int state) = 0;
	virtual BOOL32 COMAPI is_enabled(int state) = 0;

	//
	// If texture is indexed (palettized), get_red_bits() etc. will return 0.
	//
	virtual BOOL32 COMAPI is_indexed(TXM_ID id) = 0;

	//
	// Get internal color resolution for given texture. These 4 functions
	// will return 0 if the texture is palettized.
	//
	virtual int COMAPI get_red_bits(TXM_ID id) = 0;
	virtual int COMAPI get_green_bits(TXM_ID id) = 0;
	virtual int COMAPI get_blue_bits(TXM_ID id) = 0;
	virtual int COMAPI get_alpha_bits(TXM_ID id) = 0;

	virtual int COMAPI get_width(TXM_ID id, int mipmap_level = 0) = 0;
	virtual int COMAPI get_height(TXM_ID id, int mipmap_level = 0) = 0;

	// get texture id of ith entry
	virtual TXM_ID COMAPI get_entry_id (int id) const = 0;

// animation api 

	//increments ref count for all textures in the axm, if all found
	//currently: a texture's ref count gets inc'd by the number of times
	// the same number of time the texture is used in the animation
	virtual AXM_HANDLE COMAPI get_axm_handle (const char* axm_name) =0;

	//increments reference count by 1, even if you don't know the axm name.
	virtual void COMAPI add_axm_ref(const AXM_HANDLE& handle) =0;

	//decrements ref count for axm; unloads when count reaches zero
	virtual void COMAPI release_axm (const AXM_HANDLE& axm_handle) =0;

	virtual void COMAPI get_axm_name(AXM_HANDLE handle, char * buffer) const=0;
	virtual void COMAPI get_axm_frame(AXM_HANDLE handle, int frame, AXM_RECT & rect ) const=0;

	virtual int COMAPI get_axm_frame_count( AXM_HANDLE handle ) const=0; 

// enhanced library queries

	// returns the number of loaded textures;
	// performance warning: this involves a list traversal
	// remember: axms are composed of txms; 
	//           the txm count and the txm name set returned reflects this
	virtual int COMAPI get_txm_count( void ) const = 0;
	virtual int COMAPI get_axm_count( void ) const = 0;

	virtual void COMAPI enumerate_txms( TAXM_ENUM_CALLBACK, void *context=0 ) const = 0;
	virtual void COMAPI enumerate_axms( TAXM_ENUM_CALLBACK, void *context=0 ) const = 0;

// initialization and shutdown
	
	// flushes all internal texture data, returning the component to its initialized
	// state. Calling this is required when the render pipeline changes devices or 
	// otherwise loses all its texture data.
	virtual void reset( void ) = 0;
};

/*
	FILE FORMAT NOTES:

		documented 3-13-98 (pci)

	[Texture library]

		[texture id]						user defined, etc.

			[MIP0]							MIP1, etc.

				"Image X size"				= DWORD width
				"Image Y size"				= DWORD height

				[Palette 8 bit]				(optional)

  					"Palette color count"	= DWORD num_colors

					"Palette RGB 888"		= RGB array[num_colors]

					"Transparent index"		= DWORD color (optional)

					"Alpha 8 bit"			= BYTE array[w*h] (optional)

					"Image indices"			= BYTE array[w*h]

				[True RGB 565]				(optional)

					"transparent color"		= WORD rgb (optional)
					"image colors"			= WORD array[w*h]
					"alpha 8 bit"			= BYTE array[w*h] (optional)

		documented 6-98 (tlp)
		
	[Animation library]

		[animation id]						user defined name that forms the base texture name 
											for any and all textures in the animation

			"Texture count"					= DWORD num textures in animation
			"Frame count"					= DWORD num frames in animation
			"Frame rects"					= frame count structs of:
												DWORD texture extension
												float (4 byte) u0, v0, u1, v1

	notes: 
		each animation is composed of one or more texture panes
		and one or more sub rectangles within each pane

		rather than storing either an explicit TXM_ID or texture id per frame 
		the texture name is created by combining the animation id and a texture "extension"

		for instance if the animation id was "star" 
		then a given frame rect might have a texture extension of "0" or "1"
		at load time this is resolved into the texture ids of "star_0" and "star_1" respectively

		when querrying for the frame via the txmlib api the frame returned does include extension
		rather it returns the TXM_ID that refers to the appropriate texture which can be used
		then be used for drawing.

		it is best if the actual textures "star_0" "star_1" live in the same .txm library file
*/

#endif
