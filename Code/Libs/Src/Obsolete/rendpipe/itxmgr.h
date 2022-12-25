#ifndef ITXMGR_H
#define ITXMGR_H
//
// ITXMMGR.H - The texture manager interface
//

//
// Design Notes:
//		There are two directions we can take here: fully expose the DirectX surface management interface, or 
//	expose only the kind of surface manipulation we intend to use.  The first methods adds code without adding
//	value, since all of the complexity of DirectX is preserved, but a level of indirection and inefficiency is added.
//	The second method limits what is possible, but can definitely simplify the code which uses this interface.
//		The second method makes more sense for us, since we are primarily interested in texture management
//	rather than general 2D surface manipulation. If general 2D surface support is added, including 2D drawing
//	primitives, they should become DACOM instances instead of being managed by the procedural interfaces below.
//		What we need:
//		1) Texture creation/destruction, including mip-map levels
//		2) Setting the bits for any mip level of a texture
//		3) Setting the palette for indexed textures
//		4) Setting state values which are bound to the texture, like filtration algorithms, wrapping, etc.
//		5) Direct access to the surface memory
//		6) Inter-texture blitting. [*** Do we really need this?]
//      7) Activating/Deactivating textures
//

//
// Include files
//

#include <d3d.h>
#include "dacom.h"
#include "pixel.h"

//
// Simple type definitions
//

typedef DWORD TxHandle;

enum TmGlobalState
{
	TMGS_MIPMAP_ENABLE,
	TMGS_MANAGED,
	TMGS_TEXTURE_ENABLE
};

//
// Class and structure definitions
//

struct TxLock
{
	int             width, height;
	DWORD           pitch;
	void *          pixels;
	PixelFormat     pf;
};

struct TxDeviceInfo
{
	int             min_width, min_height;
	int             max_width, max_height;
	unsigned int    max_memory;
	unsigned int    available_memory;
	int             tmu_count;			// texture management units (doohickeys that fiddle with texels)
};

// Yes, I know that this is evil. -TNB
#pragma pack(1)
struct TxRGB
{
	int  r,g,b;
};
#pragma pack()

//
// Constants
//

const TxHandle HTX_INVALID = 0;

//

struct ITextureManager : public IDAComponent
{
	virtual void COMAPI set_texture_global_state (TmGlobalState &newState, DWORD newValue) = 0;
		// Sets the specified global state to the specified value.  These are global texture manager
		// states.

	virtual BOOL32 COMAPI get_texture_device_info (TxDeviceInfo &info) = 0;
		// Retrieves internal information about the texture manager.  Returns true if the information could be
		// retreived, false if the information could not be retrieved.

	virtual TxHandle COMAPI create_texture (int width, int height, const PixelFormat &desiredformat, int numMipLevels) = 0;
		// Creates a texture with the given dimensions, internal format, and number of mip levels.
		// The texture will automatically get managed by IRenderPipeline.
		// Behind the scenes, this is allocating a number of surfaces and attaching them togather into a
		// chain of mip maps.  It also creates internal structures used to track per-texture state variables.
		// If DX6 is available, it will create the texture as a managed texture.
		// If TMGS_MANAGED is set to false, this will attempt to create the texture on the device itself.
		// Returns HTX_INVALID if the texture cannot be created.

	virtual void COMAPI destroy_texture (TxHandle which) = 0;
		// Destroys the given texture, including all of its mip levels.
		// Note that it does not return success or failure.  Debug versions will assert if you attempt to destroy
		// a non-valid texture, and release versions will simply report the event via IDumpText.

	virtual BOOL32 COMAPI lock_texture (TxHandle which, int level, TxLock *lockData) = 0;
		// Locks the given mip level of the given texture for writing (and reading if possible).
		// The pointer to the pixel data, the pitch of the buffer, and a copy of the pixel format
		// of the data is stored in the lockData structure.
		// Returns TRUE on success, FALSE on error. The nature of the error is reported via IDumpText.
		// NOTE: Locking multiple levels of a texture at the same time is explicitly supported, but each level
		// may be locked only once.

	virtual void COMAPI unlock_texture (TxHandle which, int level) = 0;
		// Unlocks the given level of the given texture. Debug versions will assert if the texture is not already
		// locked; release versions will report the error via IDumpText.

	virtual BOOL32 COMAPI set_texture_level_data (TxHandle which, int level, int srcWidth, int srcHeight, int srcStride, const PixelFormat &srcFormat, void *srcPixels) = 0;
		// Copies the data from the specified buffer into the specified level of the specified texture.

	virtual void COMAPI set_texture_render_state(TxHandle which, D3DRENDERSTATETYPE state, DWORD value) = 0;
		// This will bind a state value to the given texture, so that when the texture is made active, the
		// state will be set to the value given as well. If more than one state variable needs to be set, this
		// function can be called multiple times. If the state has already been set for this texture, its value is
		// changed to the given value, otherwise the state is bound to the texture and its value stored. Only states
		// which have been bound to a texture will be set when the texture is made active.  To unbind the value from
		// the texture, you must use clear_texture_render_state().
		// Only texture related D3D states are allowed.

	virtual BOOL32 COMAPI get_texture_render_state(TxHandle which, D3DRENDERSTATETYPE state, DWORD *valuePtr) = 0;
		// This will return the value of a render state variable bound to the given texture.  If the state has
		// not been bound to the texture, the function returns FALSE.  If the state HAS been bound, it returns TRUE
		// and *valuePtr will be set to the value of the state variable.

	virtual void COMAPI clear_texture_render_state (TxHandle which, D3DRENDERSTATETYPE state) = 0;
		// Once a value has been set for texture's render state variable, that variable will be set whenever the texture
		// is made active. The only way to unbind the value from the texture is to call this function.
		// Debug versions assert if the given texture does not have the given state variable bound to it. Release
		// versions will report this condition via IDumpText.

	virtual BOOL32 COMAPI set_texture_palette (TxHandle which, int start, int length, const TxRGB *colors) = 0;
		// Sets the given range of colors in the palette for the given texture.  Returns FALSE if the given
		// texture is not indexed, or if the start or start+length values are outside the valid index range for
		// the given texture.

	virtual BOOL32 COMAPI get_texture_palette (TxHandle which, int start, int length, TxRGB *colors) = 0;
		// Gets the palette colors for the given range of the given texture.  Returns FALSE if the given texture
		// is not indexed, or if the start or start+length exceed the given texture's value index range.

	virtual BOOL32 COMAPI blit_texture (TxHandle hDest, RECT destRect, TxHandle hSrc, RECT srcRect) = 0;
		// Blits data from one texture to another.  It will only work if the two textures have the same pixel format.
		// Returns TRUE if it succeeds, FALSE otherwise.  Debug versions assert if the pixel formats are not the same,
		// release versions report the error via IDumpText.

	virtual void COMAPI activate_texture (TxHandle which, int stage) = 0;
		// This makes the given texture the current texture for the given stage.  Passing HTX_INVALID will disable
		// the texture for the given stage. If the given texture is invalid, it will assert in debug versions and
		// report the error via IDumpText in release versions.
};

//

#endif
