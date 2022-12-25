//
// new render pipeline prototypes.
//

#include <d3d.h>
#include <d3dtypes.h>
#include <assert.h>

//
// Compile switches
//

#define DX_TRANSFORMS
#define USE_DX_TXMGR   0   // change to non-zero to use DirectX6+ texture management

//

#include "tcomponent.h"
#include "3dmath.h"
#include "pixel.h"
#include "rplist.h"
#include "rendpipe.h"
#include "itxmgr.h"

//
// Polybatch stuff:

struct ListNode
{
	const RPList *	list;
	ListNode *		prev;
	ListNode *		next;
};

//
// Ad hoc insertion-sorted list with no delete/unlink facility.
// Very specifically suited for what we need here.
//
struct SortedList
{
	int			max_entries;
	ListNode *	entries;

	ListNode *	used_entries;
	ListNode *	free_entries;

	SortedList(void)
	{
		max_entries = 128;
		entries = new ListNode[max_entries];

		reset();
	}

	~SortedList(void)
	{
		delete [] entries;
		entries = NULL;
	}

	void reset(void)
	{
		used_entries = NULL;
		free_entries = entries;

		ListNode * node = free_entries;
		ListNode * prev = NULL;

		for (int i = 0; i < max_entries; i++, node++)
		{
			node->prev = prev;
			node->next = node + 1;
			prev = node;
		}

		prev->next = NULL;
	}

// Unlinks entry from free list.
	ListNode * get_free_entry(void)
	{
		ListNode * result;
		if (free_entries)
		{
			result = free_entries;
			free_entries = result->next;
			if (free_entries)
			{
				free_entries->prev = NULL;
			}
			else
			{
			// Out of free entries. Expand list.
				max_entries <<= 1;
				ListNode * new_entries = new ListNode[max_entries];
				ListNode * new_node = new_entries;

				ListNode * prev = NULL;
				ListNode * node = used_entries;
				while (node)
				{
					new_node->list = node->list;
					new_node->prev = prev;
					new_node->next = new_node + 1;
					prev = new_node;

					node = node->next;
					new_node++;
				}

				free_entries = prev->next;
				prev->next = NULL;

				used_entries = new_entries;

				node = free_entries;
				for (int i = free_entries - new_entries; i < max_entries; i++, node++)
				{
					node->prev = prev;
					node->next = node + 1;
					prev = node;
				}

				prev->next = NULL;

				delete [] entries;
				entries = new_entries;

				result = free_entries;
				free_entries = result->next;

			}

			result->prev = result->next = NULL;
		}
		else
		{
		// Should never happen.
			result = NULL;
		}
/*
// DEBUG
ListNode * last_entry = entries + max_entries - 1;
for (int i = 0; i < max_entries; i++)
{
	ListNode * prev = entries[i].prev;
	ListNode * next = entries[i].next;

	if (prev && (prev < entries) || (prev > last_entry))
	{
		printf("OUCH\n");
	}
	if (next && (next < entries) || (next > last_entry))
	{
		printf("OUCH\n");
	}
}
*/

		return result;
	}

	void insert(const RPList * list);
};

//

struct DXStruct
{
	LPDIRECTDRAWSURFACE3	lpDDSPrimary;
	LPDIRECTDRAWSURFACE3	lpDDSBack;
	LPDIRECTDRAWSURFACE3	lpZBuffer;

	LPDIRECTDRAW2			lpDD;
	LPDIRECT3D2				lpD3D;
	LPDIRECT3DDEVICE2		lpD3DDevice;
	LPDIRECT3DVIEWPORT2		lpD3DViewport;

	int						num_texture_formats;
	PixelFormat *			texture_formats;

	PixelFormat *			screen_pixel_format;
};

//

struct RGB
{
	U8 r,g,b;
};

struct TxStateVar
{
	TxStateVar *        prev;
	TxStateVar *        next;
	D3DRENDERSTATETYPE  state;
	DWORD               value;
};

struct MipmapLevel
{
	int						width;
	int						height;				// Size (should be square)
	LPDIRECTDRAWSURFACE3	memory_surface;     // system memory surface, thrown away after complex memory surface is created
	LPDIRECTDRAWPALETTE		palette;            // palette for this surface
};

#define MAX_MIPMAP_LEVELS 9
struct Texture
{
	// Required members for LList
	Texture *		prev;
	Texture *		next;
	
	// Texture specific members
	int					    id;                  // IRenderPipeline TxHandle for this texture.
	int					    priority;

	int					    num_mipmap_levels;
	MipmapLevel			    mipmaps[MAX_MIPMAP_LEVELS];

	LList<TxStateVar>	    stateVarList;

	unsigned int		    size_in_bytes;

	// D3D
#if USE_DX_TXMGR
	// When DirectX manages the texture, we only need the one surface.
	LPDIRECTDRAWSURFACE3    surface;            // surface automatically managed by DirectX.
#else
	// When we manage the textures, we need to maintain seperate system memory and device surfaces.
	// We also need to maintain a handle to the texture when it is in video memory, for passing to
	// the primitive drawing functions.
	D3DTEXTUREHANDLE	    handle;             // handle to device_surface
	LPDIRECTDRAWSURFACE3	memory_surface;		// complex system memory surface for mip-mapped textures.
	LPDIRECTDRAWSURFACE3	device_surface;     // complex video memory surface for mip-mapped textures.
#endif
};

//

struct RenderPipeline : IRenderPipeline, IAggregateComponent, ITextureManager
{
	BEGIN_DACOM_MAP_INBOUND(RenderPipeline)
	DACOM_INTERFACE_ENTRY(IRenderPipeline)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY(ITextureManager)
	END_DACOM_MAP()

public:

	float					h_scale, v_scale;
	float					h_offset, v_offset;

	float					znear, zfar;

	float					screen_w, screen_h;

	HWND					hWnd;
	LPDIRECTDRAW2			lpDD;
	LPDIRECT3D2				lpD3D;
	LPDIRECT3DDEVICE2		lpD3DDevice;
	LPDIRECTDRAWSURFACE3	lpZBuffer;
	LPDIRECTDRAWSURFACE3	lpDDSPrimary;
	LPDIRECTDRAWSURFACE3	lpDDSBack;
	LPDIRECT3DVIEWPORT2		lpD3DViewport;

	bool					DX_objects_owned;

	DDCAPS					hw_caps;
	DDCAPS					hel_caps;

	PixelFormat				screen_pixel_format;

	int						num_texture_formats;
	PixelFormat				texture_formats[64];

	int						num_zbuffer_formats;
	DDPIXELFORMAT			zbuffer_formats[64];

	SortedList				opaque_lists;
	SortedList				alpha_lists;

	int						num_opaque_polys;
	int						num_alpha_polys;

// Need separate pools for opaque & alpha lists.
	unsigned int			opaque_index;
	unsigned int			opaque_pool_size;
	unsigned char *			opaque_pool;

	unsigned int			alpha_index;
	unsigned int			alpha_pool_size;
	unsigned char *			alpha_pool;

//	LPDIRECT3DTEXTURE2		texture;
	unsigned int			texture;
	bool					blend;
	D3DBLEND				src_func;
	D3DBLEND				dst_func;
	D3DCMPFUNC				depth_func;

	bool					depth_sort_alpha;

#ifdef DX_TRANSFORMS
	D3DMATRIX				Mworld;
	D3DMATRIX				Mview;
	D3DMATRIX				Mproj;
	DWORD					x, y, w, h;		// viewport.
	DWORD					last_x, last_y, last_w, last_h;
#else
	float					Mview[4][4];
	float					Mproj[4][4];
	float					M[4][4];
#endif

	int						auto_flush;

	bool					display_mode_set;
	bool					exclusive_mode_set;

	// ------------- Texture Management Variables -------------
	LList<Texture>						tlist;
		// Linked list of textures, in MRU order.
	DynamicArray< TPointer< Texture > >	tindex;
		// Array of texture objects, indexed by TxHandle.
		// NOTE: This is the true texture array.  The list above is just
		// the MRU list.

	int min_txm_width;
	int min_txm_height;
	int max_txm_width;
	int max_txm_height;
		// Hardware reported minimum and maximum texture dimensions

	bool use_mipmaps;
		// true if mipmaps are to be used (i.e. the hardware supports them)
		// false otherwise

	// ------------------------ Methods ------------------------

	bool setupD3D(void);
	void shutdownD3D(void);

	bool create_surfaces(void);
	void destroy_surfaces(void);

	void render_opaque_lists(void);
	void render_alpha_lists_depth_sorted(void);
	void render_alpha_lists_unsorted(void);

	void do_unclipped_list(const RPList * list, struct RPVERTEX *& v_ptr);
	void do_unclipped_indexed_list(const RPList * list, struct RPVERTEX *& v_ptr, U16 *& i_ptr);

	void clip_list(const RPList * list, struct RPVERTEX *& v_ptr, int cnt);
	void clip_indexed_list(const RPList * list, struct RPVERTEX *& v_ptr, U16 *& i_ptr, int cnt);

	HRESULT restore_surface(LPDIRECTDRAWSURFACE3 surface);

	void compute_M(void);

	inline void Transform2D3D(D3DMATRIX & dst, const Transform & src)
	{
	//
	// Transpose rotation.
	//
		dst.m[0][0] = src.d[0][0];
		dst.m[1][0] = src.d[0][1];
		dst.m[2][0] = src.d[0][2];
		dst.m[0][1] = src.d[1][0];
		dst.m[1][1] = src.d[1][1];
		dst.m[2][1] = src.d[1][2];
		dst.m[0][2] = src.d[2][0];
		dst.m[1][2] = src.d[2][1];
		dst.m[2][2] = src.d[2][2];

		dst.m[3][0] = src.translation.x;
		dst.m[3][1] = src.translation.y;
		dst.m[3][2] = src.translation.z;
		dst.m[0][3] = 
		dst.m[1][3] = 
		dst.m[2][3] = 0;
		dst.m[3][3] = 1;
	}


public:

	RenderPipeline(void);
	~RenderPipeline(void);

    // IAggregateComponent interface
	GENRESULT COMAPI Initialize(void)
	{ 
		return GR_OK; 
	}

	GENRESULT init(AGGDESC *desc)
	{ 
		return GR_OK;
	}

	// IRenderPipeline interface
	BOOL32 COMAPI startup(HWND hWnd, int hres, int vres, int bpp, BOOL32 zbuffer, BOOL32 flip_if_possible);
	BOOL32 COMAPI startup(HWND hWnd, const DirectXInfo * dxinfo);
	void COMAPI shutdown(void);

	void COMAPI set_display_mode(int hres, int vres, int bpp);
	void COMAPI restore_display_mode(void);

	void COMAPI getDXinfo(DirectXInfo * dxinfo);

	void COMAPI enable(int state);
	void COMAPI disable(int state);
	BOOL32 COMAPI is_enabled(int state);

	void COMAPI set_opaque_pool_size(U32 bytes);
	void COMAPI set_alpha_pool_size(U32 bytes);

	U32 COMAPI get_opaque_pool_size(void);
	U32 COMAPI get_alpha_pool_size(void);

// This goes straight through to D3D.
	void COMAPI set_render_state(D3DRENDERSTATETYPE state, DWORD value);

	void COMAPI set_viewport(int x, int y, int w, int h);

	void COMAPI set_modelview(const Transform & modelview);
	void COMAPI set_ortho(float x, float y, float w, float h, float znear = -1.0, float zfar = +1.0);
	void COMAPI set_perspective(float fovy, float aspect, float znear, float zfar);

	void COMAPI set_auto_flush(int max_opaque_polys);

	void COMAPI clear_buffers(void);
	void COMAPI swap_buffers(void);

	void COMAPI begin_scene(void);

	// This gets stored with the list.
		void COMAPI set_list_render_state(D3DRENDERSTATETYPE state, DWORD value);

		void COMAPI submit_list(D3DPRIMITIVETYPE type, const RPVertex1 * verts, int num_verts, bool clip);
		void COMAPI submit_indexed_list(D3DPRIMITIVETYPE type, const RPVertex1 * verts, int num_verts, const U16 * indices, int num_indices, bool clip);

		int flush_opaque(void);
		int flush_translucent(bool depth_sort);

		int COMAPI flush(DWORD flags = RP_OPAQUE | RP_TRANSLUCENT_DEPTH_SORTED);

	void COMAPI end_scene(void);

	// ITextureManager interface
	TxHandle COMAPI create_texture (int width, int height, const PixelFormat &desiredformat, int numMipLevels);
	void COMAPI destroy_texture (TxHandle which);
	BOOL32 COMAPI lock_texture (TxHandle which, int level, TxLock *lockData);
	void COMAPI unlock_texture (TxHandle which, int level);
	void COMAPI set_texture_render_state(TxHandle which, D3DRENDERSTATETYPE state, DWORD value);
	BOOL32 COMAPI get_texture_render_state(TxHandle which, D3DRENDERSTATETYPE state, DWORD *valuePtr);
	void COMAPI clear_texture_render_state (TxHandle which, D3DRENDERSTATETYPE state);
	BOOL32 COMAPI set_texture_palette (TxHandle which, int start, int length, const TxRGB *colors);
	BOOL32 COMAPI get_texture_palette (TxHandle which, int start, int length, TxRGB *colors);
	BOOL32 COMAPI blit_texture (TxHandle hDest, RECT destRect, TxHandle hSrc, RECT srcRect);
	void COMAPI set_texture_global_state (TmGlobalState &newState, DWORD newValue);
	BOOL32 COMAPI get_texture_device_info (TxDeviceInfo &info);
	BOOL32 COMAPI set_texture_level_data (TxHandle which, int level, int srcWidth, int srcHeight, int srcStride, const PixelFormat &srcFormat, void *srcPixels);
	void COMAPI activate_texture (TxHandle which, int stage);
};