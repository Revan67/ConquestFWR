// TextureLibrary.cpp
//
//

#pragma warning( disable: 4018 4100 4201 4512 4530 4663 4688 4710 4786 )

#define WIN32_LEAN_AND_MEAN
#define TL_LOG(s) do{FILE*_tlf=fopen("texlib_diag.txt","a");if(_tlf){fputs((s),_tlf);fclose(_tlf);}}while(0)
#define TL_LOGF(fmt,...) do{FILE*_tlf=fopen("texlib_diag.txt","a");if(_tlf){fprintf(_tlf,fmt,__VA_ARGS__);fclose(_tlf);}}while(0)
//#define INITGUID
#include <windows.h>
#include <stdio.h>
#include <ddraw.h>
#include <map>
#include <string>
#pragma warning( disable: 4018 4245 4663 )	// for some reason this is required *right here* again
#include <vector>
#include <mmsystem.h>

#include "dacom.h"
#include "FDump.h"
#include "TempStr.h"
#include "TSmartPointer.h"
#include "tcomponent.h"
#include "da_d3dtypes.h"
#include "da_heap_utility.h"
#include "pixel.h"
#include "RPUL.h"
//#include "AllocLite.h"
#include "FileSys_Utility.h"
#include "IProfileParser_Utility.h"
#include "IVideoStreamControl.h"
#include "rendpipeline.h"
#include "ITextureLibrary.h"

//

#define CLSID_TextureLibrary "TextureLibrary"

//

//targa stuff
struct E1
{
	U8 value[1];
	operator int (void)
	{
		return *(U8 *)value;
	}
};
struct E2
{
	U8 value[2];
	operator int (void)
	{
		return *(U16 *)value;
	}
};
struct E4
{
	U8 value[4];
	operator int (void)
	{
		return *(U32 *)value;
	}
};


struct TGA_Header
// TARGA FILE HEADER = 18 bytes
{
	E1 identsize;             // size of ID field that follows 18 byte header (0 usually)
	E1 palette;				// type of color map 0=none, 1=has palette
	E1 imagetype;             // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

	E2 color_start;			// first color map entry in palette
	E2 num_palette_colors;	// number of colors in palette
	E1 bits_per_color;		// number of bits per palette entry 15,16,24,32

	E2 xstart;             // image x origin
	E2 ystart;             // image y origin
	E2 width;              // image width in pixels
	E2 height;             // image height in pixels
	E1 bits_per_pixel;		// image bits per pixel 8,16,24,32
	E1 descriptor;			// image descriptor bits (vh flip bits)

	int verify (void)
	{
		if (
			(palette <= 1) &&
			(imagetype & ~8) <= 3 && 
			(
			bits_per_pixel== 8 ||
			bits_per_pixel==15 || bits_per_pixel==16 ||
			bits_per_pixel==24 || bits_per_pixel==32
			)
		   )
		{
			return TRUE;
		}
		return FALSE;
	}

// MISCELLANEOUS

	int is_packed (void)
	{
		return (imagetype & 8);
	}

	// NOTE: These functions return a boolean indication of whether or not the disk data is
	// flipped with respect to the D3D image specification, i.e. first row first, first column first.
	// If these routines return true, then the data in the buffer is stored contrary to the D3D spec,
	// i.e. if (is_x_flipped()) then {last column first on disk} and if (is_y_flipped()) then {last row first on
	// disk}.
	// This value is used to determine if the disk bits should be flipped into the D3D orientation before being
	// sent to the render pipeline.
	bool is_x_flipped (void)
	{
		// Default TARGA storage is first column first. If the flag is set, then last column first.
		// Thus, if the flag is set, the image is flipped w/ repect to D3D.
		return ((descriptor & 0x10) != 0);
	}

	bool is_y_flipped (void)
	{
		// Default TARGA storage is last row first. If the flag is set, then first row first.
		// Thus, if the flag is NOT set, the image is flipped w/ respect to D3D.
		return !((descriptor & 0x20) != 0);
	}

// Note: the TGA format has redundant information on whether or
// not a palette exists... has_palette,is_indexed,num_colors==0

	int has_palette (void)
	{
		return (palette == 1);
	}
	int is_indexed (void)
	{
		return (imagetype & 7) == 1;
	}

// GREY SCALE is a unusual feature to avoid storing the palette!

	int is_grey (void)
	{
		return (imagetype & 7) == 3;
	}

// an 8-bit ALPHA can be stored in the palette or individual pixels

	int is_alpha (void)
	{
		int a = 0;
		if (is_indexed())
		{
			if (bits_per_color == 32)
				a = 8;
		}
		else if (bits_per_pixel == 32)
		{
			a = 8;
		}
		return (a > 0);
	}

	int num_colors (void)
	{
		int colors = 0;
		if (is_indexed())
		{
			colors = color_start + num_palette_colors;
		}
		else if (is_grey())
		{
			colors = 256; // fixed RGB palette
		}
		return (colors);
	}

};


// ------------------------------------------------------------------
//
// Defines the texture related UTF file format strings.
//
//

const unsigned int TXM_NAME_LEN = 255+1;

char txm_width[]		= "Image X size";
char txm_height[]		= "Image Y size";
char txm_palcount[]		= "Palette color count";
char txm_alpha8bit[]	= "Alpha 8 bit";
char txm_indices[]		= "Image indices";
char txm_colors[]		= "Image colors";
char txm_paldata[]		= "Palette RGB 888";
char txm_colors8bit[]	= "Palette 8 bit";
char txm_colors16bit[]	= "True RGB 565";
char txm_mipsearch[]	= "MIP*";

char tlib_search[]		= "*.*";
char tlib_txmname[]		= "Texture library";
char tlib_axmname[]		= "Animation library";
char tlib_axmtcount[]	= "Texture count";
char tlib_axmfcount[]	= "Frame count";
char tlib_axmfrects[]	= "Frame rects";
char tlib_axmfps[]		= "FPS";

// ------------------------------------------------------------------
// ITL_TEXTUREIMAGEDATA
//
// Used when loading texture data
//

struct ITL_TEXTUREIMAGEDATA
{
	PixelFormat format;		// format of the data
	U32			width;
	U32			height;
	U32			stride;
	char		*colors;	// actual RGBA or color indices in the format given in image_format
	char		*alphas;	// alpha map if necessary. (always 8bit)
	RGB		    palette[256];	

	ITL_TEXTUREIMAGEDATA( );
	~ITL_TEXTUREIMAGEDATA( );
};

//

ITL_TEXTUREIMAGEDATA::ITL_TEXTUREIMAGEDATA( )
{
	format.init( 0,0,0,0,0 );
	colors = NULL;
	alphas = NULL;
	width = 0;
	height = 0;
	stride = 0;
}

//

ITL_TEXTUREIMAGEDATA::~ITL_TEXTUREIMAGEDATA( )
{
	delete[] colors;
	colors = NULL;

	delete[] alphas;
	alphas = NULL;

	width = height = stride = 0;
}


// ------------------------------------------------------------------
// ITL_TEXTUREFRAME
//
// Textures are made up of at least one frame.
// 

struct ITL_TEXTUREFRAME
{
	U32		texture_id_idx;		// index into list of renderpipeline texture ids for this frame.
	float	u0, v0;
	float	u1, v1;

	ITL_TEXTUREFRAME( U32 id=0, float _u0=0.00F, float _v0=0.00F, float _u1=1.00F, float _v1=1.00F );

	void set( ITL_TEXTUREFRAME &frame );
	void set_id( U32 id=0 );
	void set_rect( float _u0=0.00F, float _v0=0.00F, float _u1=1.00F, float _v1=1.00F );
};

//

inline ITL_TEXTUREFRAME::ITL_TEXTUREFRAME( U32 id, float _u0, float _v0, float _u1, float _v1 )
{
	set_id( id );
	set_rect( _u0, _v0, _u1, _v1 );
}

//

inline void ITL_TEXTUREFRAME::set( ITL_TEXTUREFRAME &frame )
{
	texture_id_idx = frame.texture_id_idx;
	u0 = frame.u0;
	v0 = frame.v0;
	u1 = frame.u1;
	v1 = frame.v1;
}

//

inline void ITL_TEXTUREFRAME::set_id( U32 id )
{
	texture_id_idx = id;
}

//

inline void ITL_TEXTUREFRAME::set_rect( float _u0, float _v0, float _u1, float _v1 )
{
	u0 = _u0;
	v0 = _v0;
	u1 = _u1;
	v1 = _v1;
}

// ------------------------------------------------------------------
// ITL_TEXTUREOBJECT
//
// Each texture as an associated texture object.  This holds the
// instance invariant (archetype) data.
//

struct ITL_TEXTUREOBJECT;

typedef std::vector<ITL_TEXTUREOBJECT*> OBJPTR_VECTOR;
typedef std::vector<ITL_TEXTUREFRAME> FRAME_VECTOR;

struct ITL_TEXTUREOBJECT
{
public:  // Interface

	ITL_TEXTUREOBJECT();
	~ITL_TEXTUREOBJECT();

	HRESULT set_name( const char *name );
	HRESULT set_frame_rate( float fps );

	HRESULT set_frame_id( U32 frame_num, U32 texture_id_idx );
	HRESULT set_frame_rect( U32 frame_num, float _u0=0.00F, float _v0=0.00F, float _u1=1.00F, float _v1=1.00F );
	HRESULT set_frame_texture( U32 tcount, ITL_TEXTUREOBJECT* texture_object );

	ITL_TEXTUREOBJECT (const ITL_TEXTUREOBJECT& other)
	{
		texture_name = NULL;
		*this = other;
	}

	const ITL_TEXTUREOBJECT& operator = (const ITL_TEXTUREOBJECT& other)
	{
		set_name (other.texture_name);
		ref_count = other.ref_count;
		rp_texture_id = other.rp_texture_id;
		frames_per_sec = other.frames_per_sec;
		frames = other.frames;
		frame_texture_objs = other.frame_texture_objs;
		_IVideoStreamControl = other._IVideoStreamControl;
		return *this;
	}

public:  // Data
	char				*texture_name;
	int					ref_count;
	U32					rp_texture_id;			// if num_frames==1, this is the rp texture id
	float				frames_per_sec;			// default framerate

	FRAME_VECTOR		frames;

	OBJPTR_VECTOR		frame_texture_objs;
	
	COMPTR<IVideoStreamControl> _IVideoStreamControl;

	void addref (void);
};


// ------------------------------------------------------------------
// ITL_TEXTUREOBJECT Implementation
//

ITL_TEXTUREOBJECT::ITL_TEXTUREOBJECT( )
{
	texture_name = NULL;
	ref_count = 0;
	rp_texture_id = HTX_INVALID;
}

//

ITL_TEXTUREOBJECT::~ITL_TEXTUREOBJECT()
{
	delete[] texture_name;
}

//

HRESULT ITL_TEXTUREOBJECT::set_name( const char *name )
{
	delete[] texture_name;

	if (name)
	{
		texture_name = new char[strlen(name)+1];
		strcpy( texture_name, name );
	}
	else
		texture_name = NULL;

	return S_OK;
}

//

HRESULT ITL_TEXTUREOBJECT::set_frame_rate( float fps )
{
	frames_per_sec = fps;
	return S_OK;
}

//

inline HRESULT ITL_TEXTUREOBJECT::set_frame_id( U32 frame_num, U32 texture_id_idx )
{
	if (frames.size () < frame_num + 1)
		frames.resize (frame_num + 1);

	frames[frame_num].set_id( texture_id_idx );
	return S_OK;
}

//

HRESULT ITL_TEXTUREOBJECT::set_frame_rect( U32 frame_num, float _u0, float _v0, float _u1, float _v1 )
{
	if (frames.size () < frame_num + 1)
		frames.resize (frame_num + 1);

	frames[frame_num].set_rect( _u0, _v0, _u1, _v1 );
	return S_OK;
}

//

HRESULT ITL_TEXTUREOBJECT::set_frame_texture( U32 tcount, ITL_TEXTUREOBJECT* texture_object )
{
	if (frame_texture_objs.size () < tcount + 1)
		frame_texture_objs.resize (tcount + 1);

	frame_texture_objs[tcount] = texture_object;
	return S_OK;
}

//EMAURER must be called during archetype_lock critical section.
void ITL_TEXTUREOBJECT::addref (void)
{
	// boost it!
	//
	ref_count++;

	//EMAURER frames[0] points to self, don't boost it.

	// subtextures
	if(frames.size () > 1 ) {
		for( unsigned int i = 0; i < frame_texture_objs.size (); i++)
			frame_texture_objs[i]->addref ();
	}
}

// ------------------------------------------------------------------
// ITL_TEXTUREOBJECTREF
//
// Each managed reference to a textures has an associated object
// reference.  This holds the instance specific info for animated
// textures.
//

struct ITL_TEXTUREOBJECTREF
{
	ITL_TEXTUREOBJECTREF( void );

	HRESULT update( float dt );
	HRESULT get_frame( U32 the_frame_num, ITL_TEXTUREFRAME_IRP *out_frame );
	HRESULT set_frame_number( U32 frame_num );
	HRESULT set_frame_rate( float fps );
	HRESULT set_frame_time( float frame_time );

	void init (ITL_TEXTUREOBJECT*);

public:  // Data

	//EMAURER target 'texture' must persist as long as this object persists.
	ITL_TEXTUREOBJECT*	texture;
	float				local_frames_per_sec;
	const float			*frames_per_sec;		// either points to local_frames_per_sec, or texture->frames_per_sec
	float				frames_per_sec_mul;		
	U32					frame_num;
	double				frame_running_time;
	double				total_running_time;
	int					ref_count;
	ITL_PLAYCOMMAND		play_mode;
};



// ------------------------------------------------------------------
// ITL_TEXTUREOBJECTREF Implementation
//

ITL_TEXTUREOBJECTREF::ITL_TEXTUREOBJECTREF(void)
{
	ref_count = 0;
}

void ITL_TEXTUREOBJECTREF::init (ITL_TEXTUREOBJECT* _texture)
{
	texture = _texture;
	total_running_time = 0.00F;
	frames_per_sec = &texture->frames_per_sec;
	frames_per_sec_mul = 1.0f;
	
	if( texture->_IVideoStreamControl ) {
		IVSC_STREAMINFO si;
		texture->_IVideoStreamControl->get_info( &si );
		total_running_time = si.total_running_time;
	}
	else if( fabs( *frames_per_sec ) > 0.00001 ) {
		// calc total running length based on frames per sec.
		total_running_time = texture->frames.size () / *frames_per_sec - 0.000001;
	}
	
	frame_num = 0;
	frame_running_time = 0.00F;

	play_mode = ITL_PLAY_LOOPED;
}

//

HRESULT ITL_TEXTUREOBJECTREF::update( float dt_ms )
{
	int num_texture_frames = texture->frames.size();

	if( (num_texture_frames > 1) || (texture->_IVideoStreamControl != NULL) ) {
		
		double new_running_time = 0.0f;
		double diff_time_s = dt_ms * 0.001f;
		
		switch( play_mode ) {
		
		case ITL_PLAY_LOOPED:	
			frame_running_time += diff_time_s;
			new_running_time = fmod( frame_running_time, total_running_time );	
			break;

		case ITL_PLAY_PINGPONG:
			frame_running_time += frames_per_sec_mul * diff_time_s;

			if( (frames_per_sec_mul < 0.0) && (frame_running_time < 0.0) ) {
				new_running_time = 0.0f - fmod( frame_running_time, total_running_time );
				frames_per_sec_mul = -frames_per_sec_mul;
			}
			else if( (frames_per_sec_mul > 0.0) && (frame_running_time > total_running_time) ) {
				new_running_time = total_running_time - fmod( frame_running_time, total_running_time );
				frames_per_sec_mul = -frames_per_sec_mul;
			}
			else {
				new_running_time = frame_running_time;
			}
			break;

		case ITL_PLAY_ONE_TIME:
			frame_running_time += diff_time_s;
			new_running_time = __min( frame_running_time, total_running_time );
			new_running_time = __max( new_running_time, 0 );
			break;

		}
	
		set_frame_time( new_running_time );		// sets frame_num
	}

	return S_OK;
}

//

HRESULT ITL_TEXTUREOBJECTREF::get_frame( U32 the_frame_num, ITL_TEXTUREFRAME_IRP *out_frame )
{
	U32 num_texture_frames = texture->frames.size();

	if( num_texture_frames == 0 ) {
		out_frame->rp_texture_id = HTX_INVALID;
		out_frame->u0 = 0;
		out_frame->v0 = 0;
		out_frame->u1 = 1;
		out_frame->v1 = 1;
		return E_FAIL;
	}

	if( the_frame_num == ITL_FRAME_CURRENT ) {
		if( (num_texture_frames > 1) || (texture->_IVideoStreamControl != NULL) ) {
			the_frame_num = frame_num;
		}
		else {
			the_frame_num = 0;
		}
	}
	else if( the_frame_num == ITL_FRAME_LAST ) {
		the_frame_num = num_texture_frames - 1;
	}
	else if( the_frame_num >= num_texture_frames ) {
		the_frame_num = num_texture_frames - 1;
	}

	ASSERT( the_frame_num < num_texture_frames );
	ASSERT( texture->frame_texture_objs[ texture->frames[the_frame_num].texture_id_idx ] );

	if( texture->_IVideoStreamControl == NULL ) {
		out_frame->rp_texture_id = texture->frame_texture_objs[ texture->frames[the_frame_num].texture_id_idx ]->rp_texture_id;
		out_frame->u0 = texture->frames[the_frame_num].u0;
		out_frame->v0 = texture->frames[the_frame_num].v0;
		out_frame->u1 = texture->frames[the_frame_num].u1;
		out_frame->v1 = texture->frames[the_frame_num].v1;
	}
	else {
		out_frame->rp_texture_id = texture->rp_texture_id;
		out_frame->u0 = texture->frames[0].u0;
		out_frame->v0 = texture->frames[0].v0;
		out_frame->u1 = texture->frames[0].u1;
		out_frame->v1 = texture->frames[0].v1;
	}

	return S_OK;
}

//

HRESULT ITL_TEXTUREOBJECTREF::set_frame_time( float frame_time )
{
	frame_running_time = frame_time;

	if( texture->_IVideoStreamControl ) {
		texture->_IVideoStreamControl->set_stream_time( frame_time );
		texture->_IVideoStreamControl->update();
		texture->_IVideoStreamControl->get_stream_time( &frame_time );
		frame_running_time = frame_time;
	}
	else {
		ASSERT( frame_running_time >= 0.0 );
		ASSERT( frame_running_time <= total_running_time + 0.000002 );
		frame_num = ((int) floor( fmod( frame_running_time * (*frames_per_sec), texture->frames.size () ) ) );
		ASSERT( frame_num < texture->frames.size () );
		set_frame_number( frame_num );
	}
	
	return S_OK;
}

//

HRESULT ITL_TEXTUREOBJECTREF::set_frame_number( U32 _frame_num )
{
	if( _frame_num < texture->frames.size () ) {
		frame_num = _frame_num;
		return S_OK;
	}
	return E_FAIL;
}

//

HRESULT ITL_TEXTUREOBJECTREF::set_frame_rate( float fps )
{
	local_frames_per_sec = fps;
	frames_per_sec = &local_frames_per_sec;

	if( texture->_IVideoStreamControl ) {
		IVSC_STREAMINFO si;
		texture->_IVideoStreamControl->get_info( &si );
		total_running_time = si.total_running_time;
	}
	else if( fabs( *frames_per_sec ) > 0.00001 ) {
		// calc total running length based on frames per sec.
		total_running_time = texture->frames.size () / *frames_per_sec - 0.000001;
	}

	return S_OK;
}

// --------------------------------------------------------------------------

struct StrPred
{
	bool operator () (const std::string& lhs, const std::string& rhs) const
	{
		return stricmp (lhs.c_str (), rhs.c_str ()) < 0;
	}
};
		
// --------------------------------------------------------------------------
// ITL_TEXTUREFORMATMAP
//
//
//
struct ITL_TEXTUREFORMATMAP 
{
	//

	ITL_TEXTUREFORMATMAP()
	{
		fourcc = 0;

		bpp_match = 0;
		bpp_mask = 0xFFFFFFFF;
		r_match = 0;
		r_mask = 0xFFFFFFFF;
		g_match = 0;
		g_mask = 0xFFFFFFFF;
		b_match = 0;
		b_mask = 0xFFFFFFFF;
		a_match = 0;
		a_mask = 0xFFFFFFFF;
		alpha_match = 0;	
		alpha_mask = 0xFFFFFFFF;

	}

	//

	bool is_member_of( PixelFormat &test_pf, U32 num_alpha_bits_used )
	{
		bool bp,r,g,b,a,al;

		bp = r = g = b = a = al= false;

		if( test_pf.is_indexed() ) {
			bp= ((8  & bpp_mask) == bpp_match);
			r = ((8 & r_mask) == r_match);
			g = ((8 & g_mask) == g_match);
			b = ((8 & b_mask) == b_match);
			a = ((0 & a_mask) == a_match);
		}
		else {
			bp= ((test_pf.num_bits()  & bpp_mask) == bpp_match);
			r = ((test_pf.num_r_bits() & r_mask) == r_match);
			g = ((test_pf.num_g_bits() & g_mask) == g_match);
			b = ((test_pf.num_b_bits() & b_mask) == b_match);
			a = ((test_pf.num_a_bits() & a_mask) == a_match);
		}

		al= ((num_alpha_bits_used  & alpha_mask) == alpha_match);

		return bp && r && g && b && a && al;
	}

	//

	void init_wildcard( U32 _fourcc )
	{
		fourcc = _fourcc;

		bpp_match = 0;
		bpp_mask = 0;
		r_match = 0;
		r_mask = 0;
		g_match = 0;
		g_mask = 0;
		b_match = 0;
		b_mask = 0;
		a_match = 0;
		a_mask = 0;
		alpha_match = 0;	
		alpha_mask = 0;
	}

	//

	void set_bpp( U32 _bpp )
	{
		bpp_match = _bpp;
		bpp_mask = 0xFFFFFFFF;
	}

	//

	void set_r( U32 _r )
	{
		r_match = _r;
		r_mask = 0xFFFFFFFF;
	}

	//

	void set_g( U32 _g )
	{
		g_match = _g;
		g_mask = 0xFFFFFFFF;
	}

	//

	void set_b( U32 _b )
	{
		b_match = _b;
		b_mask = 0xFFFFFFFF;
	}

	//

	void set_a( U32 _a )
	{
		a_match = _a;
		a_mask = 0xFFFFFFFF;
	}

	//

	void set_alpha( U32 _alpha )
	{
		alpha_match = _alpha;
		alpha_mask = 0xFFFFFFFF;
	}

	//

	U32 bpp_match;
	U32 bpp_mask;
	U32 r_match;
	U32 r_mask;
	U32 g_match;
	U32 g_mask;
	U32 b_match;
	U32 b_mask;
	U32 a_match;
	U32 a_mask;
	U32 alpha_match;	// this is the alpha/mask for the optionally merged in alpha map.
	U32 alpha_mask;

	U32 fourcc;
};

#define MAX_TXM_NAME 64

//typedef AllocLite<ITL_TEXTUREOBJECT> ID_MAP_ALLOC;
typedef std::map<ITL_TEXTURE_ID, ITL_TEXTUREOBJECT, std::less<ITL_TEXTURE_ID> > ID_MAP;

//typedef AllocLite<ITL_TEXTURE_ID> NAME_MAP_ALLOC;
typedef std::string TXM_NAME;
typedef std::map<TXM_NAME, ITL_TEXTURE_ID, StrPred> NAME_MAP;

//typedef AllocLite<ITL_TEXTUREOBJECTREF> REF_ID_MAP_ALLOC;
typedef std::map<ITL_TEXTURE_REF_ID, ITL_TEXTUREOBJECTREF, std::less<ITL_TEXTURE_REF_ID> > REF_ID_MAP;

// --------------------------------------------------------------------------
// TextureLibrary
//
// The actual DACOM component.
//

struct TextureLibrary : ITextureLibrary,
						IAggregateComponent

{
	BEGIN_DACOM_MAP_INBOUND(TextureLibrary)
	DACOM_INTERFACE_ENTRY(ITextureLibrary)
	DACOM_INTERFACE_ENTRY2(IID_ITextureLibrary,ITextureLibrary)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

public:		// public interface
    
	// IAggregateComponent 
	GENRESULT COMAPI Initialize(void);
	GENRESULT init( AGGDESC *desc );

	// ITextureLibrary
	DEFMETHOD(set_library_state)( ITL_STATE state, U32 value ) ;
	DEFMETHOD(get_library_state)( ITL_STATE state, U32 *out_value ) ;
	DEFMETHOD(load_library)( IFileSystem *IFS, const char *library_name ) ;
	DEFMETHOD(load_texture)( IFileSystem *IFS, const char *texture_name ) ;
	DEFMETHOD(free_library)( BOOL release_all ) ;	
	DEFMETHOD(get_texture_id)( const char *texture_name, ITL_TEXTURE_ID *out_texture_id ) ;
	DEFMETHOD(has_texture_id)( const char *texture_name ) ;

	DEFMETHOD(update)( SINGLE dt ) ; 

	DEFMETHOD(add_ref_texture_id)( ITL_TEXTURE_ID texture_id, ITL_TEXTURE_REF_ID *out_texture_ref_id ) ;
	DEFMETHOD(release_texture_id)( ITL_TEXTURE_ID texture_id ) ;
	DEFMETHOD(add_ref_texture_ref)( ITL_TEXTURE_REF_ID texture_id ) ;
	DEFMETHOD(release_texture_ref)( ITL_TEXTURE_REF_ID texture_ref_id ) ;

	DEFMETHOD(get_texture_name)( ITL_TEXTURE_ID texture_id, char *out_texture_name, U32 max_buf_size ) ;
	DEFMETHOD(set_texture_name)( ITL_TEXTURE_ID texture_id, const char *texture_name) ;

	DEFMETHOD(set_texture_frame_texture)( ITL_TEXTURE_ID texture_id, U32 txm_id_idx, U32 rp_texture_id ) ;
	DEFMETHOD(set_texture_frame_id)( ITL_TEXTURE_ID texture_id, U32 frame_num, U32 txm_id_idx ) ;
	DEFMETHOD(set_texture_frame_rect)( ITL_TEXTURE_ID texture_id, U32 frame_num, float u0, float v0, float u1, float v1 ) ;
	DEFMETHOD(get_texture_frame)( ITL_TEXTURE_ID texture_id, U32 frame_num, ITL_TEXTUREFRAME_IRP *out_frame  );
	DEFMETHOD(set_texture_frame_rate)( ITL_TEXTURE_ID texture_id, float fps_rate ) ;
	DEFMETHOD(get_texture_frame_rate)( ITL_TEXTURE_ID texture_id, float *out_fps_rate ) ;
	DEFMETHOD(get_texture_frame_count)( ITL_TEXTURE_ID texture_id, U32 *out_frame_count ) ;
	DEFMETHOD(get_texture_format)( ITL_TEXTURE_ID texture_id, U32 frame_num, PixelFormat *out_texture_format ) ;
	DEFMETHOD(get_texture_ref_texture_id)( ITL_TEXTURE_REF_ID texture_ref_id, ITL_TEXTURE_ID *out_texture_id );

	DEFMETHOD(get_texture_ref_frame)( ITL_TEXTURE_REF_ID texture_ref_id, U32 frame_num, ITL_TEXTUREFRAME_IRP *out_frame ) ;
	DEFMETHOD(set_texture_ref_frame_time)( ITL_TEXTURE_REF_ID texture_ref_id, float frame_time ) ;
	DEFMETHOD(get_texture_ref_frame_time)( ITL_TEXTURE_REF_ID texture_ref_id, float *out_frame_time ) ;

	DEFMETHOD(set_texture_ref_frame_rate)( ITL_TEXTURE_REF_ID texture_ref_id, float fps_rate ) ;
	DEFMETHOD(get_texture_ref_frame_rate)( ITL_TEXTURE_REF_ID texture_ref_id, float *out_fps_rate ) ;

	DEFMETHOD(set_texture_ref_frame_num)( ITL_TEXTURE_REF_ID texture_ref_id, U32 frame_num ) ;
	DEFMETHOD(get_texture_ref_frame_num)( ITL_TEXTURE_REF_ID texture_ref_id, U32 *out_frame_num ) ;
	DEFMETHOD(set_texture_ref_play_mode)( ITL_TEXTURE_REF_ID texture_ref_id, ITL_PLAYCOMMAND play_command ) ;
	DEFMETHOD(get_texture_ref_play_mode)( ITL_TEXTURE_REF_ID texture_ref_id, ITL_PLAYCOMMAND *out_play_command ) ;

	DEFMETHOD(update_texture_ref)( ITL_TEXTURE_REF_ID texture_ref_id, SINGLE dt );

	DEFMETHOD(get_texture_count)( U32 *out_num_textures ) ;
	DEFMETHOD(get_texture)( U32 texture_num, ITL_TEXTURE_ID *out_texture_id ) ;

	
	TextureLibrary(void);
	~TextureLibrary(void);
	DA_HEAP_DEFINE_NEW_OPERATOR(TextureLibrary);

protected:	// protected interface
	HRESULT massage_texture_dimension( U32 width, U32 height, U32 &out_width, U32 &out_height );
	HRESULT choose_texture_format( ITL_TEXTUREIMAGEDATA *data, PixelFormat *out_pf );

	HRESULT parse_texture_format(  char *format_string, PixelFormat *out_format );
	HRESULT load_texture_format( IFileSystem *IFS, PixelFormat *out_format, char *out_format_dir, U32 max_len );
	HRESULT load_texture_mipmap_count( IFileSystem *IFS, U32 *out_mipmap_count );
	HRESULT load_texture_image( IFileSystem *IFS, ITL_TEXTUREIMAGEDATA &defaults, ITL_TEXTUREIMAGEDATA **out_data );
	
	//EMAURER returns a texture handle that has a reference cnt of 0 or 1 depending on the value of 'boost_ref'
	HRESULT load_texture_normal( IFileSystem *IFS, const char* texture_name, ITL_TEXTURE_ID&, bool boost_ref);

	//EMAURER returns a texture handle that has a reference cnt of 0 or 1 depending on the value of 'boost_ref'
	HRESULT load_texture_normal_old( IFileSystem *IFS, const char* texture_name, ITL_TEXTURE_ID&, bool boost_ref);
	//EMAURER returns a texture handle that has a reference cnt of 0 or 1 depending on the value of 'boost_ref'
	HRESULT load_texture_normal_new( IFileSystem *IFS, const char* texture_name, ITL_TEXTURE_ID&, bool boost_ref);
	//EMAURER returns a texture handle that has a reference cnt of 0 or 1 depending on the value of 'boost_ref'
	//This version has targa action
	HRESULT load_texture_normal_1_6( IFileSystem *IFS, const char* texture_name, ITL_TEXTURE_ID&, bool boost_ref);

	//EMAURER returns a handle with ref 0 or 1 depending on the value of 'boost_ref'
	HRESULT load_texture_extref( IFileSystem *IFS, const char* texture_name, ITL_TEXTURE_ID& hndl, bool boost_ref);

	HRESULT load_texture_extref_normal( const char *filename, const char *fullpath, IFileSystem *IFS, const char*, ITL_TEXTURE_ID&, bool boost_ref);
	HRESULT load_texture_extref_animated( const char *filename, const char *fullpath, IFileSystem *IFS, const char* texture_name, ITL_TEXTURE_ID& hndl, bool boost_ref);

	HRESULT load_texture_animated( IFileSystem *Parent_IFS, IFileSystem *Anim_IFS, const char* texture_name, ITL_TEXTURE_ID&, bool boost_ref);

	HRESULT initialize_texture_format_maps( void );
	HRESULT clear_texture_format_maps( void );
	HRESULT add_texture_format_map( int bpp, int r, int g, int b, int a, int alpha, U32 texture_class_fourcc, ITL_TEXTUREFORMATMAP **out );

	//EMAURER 'remove_at_zero' indicates that if the reference drops to zero, the object
	//should be removed from the database. 'remove_at_zero' is set to false in the case
	//of loading animated textures. 
	void relref_texture( ITL_TEXTUREOBJECT* texture, bool remove_at_zero);

	//EMAURER returns ref_cnt.
	int _release_texture_ref( ITL_TEXTURE_REF_ID texture_ref_id );

	HRESULT get_pipe_info( void );

	HRESULT load_guts (U32 uMipLevelSet, IFileSystem* IFS, ITL_TEXTUREIMAGEDATA& overrides, U32 uM, U32& rp_id);

	//EMAURER returns true if an insertion occurred. 'boost_ref' to have ref count increased
	bool build_normal_object (U32 rp_id, const char* tname, ITL_TEXTURE_ID& result, bool boost_ref);

	DEFMETHOD(load_texture)( IFileSystem *IFS, const char *texture_name, ITL_TEXTURE_ID& hndl, bool boost_ref ) ;

	//EMAURER insert and assure no duplicates
	void insert_name (const char* name, ITL_TEXTURE_ID hndl);

#ifndef NDEBUG
	void dump (void);
	void walk (void);
#endif

#ifndef FINAL_RELEASE
	void report (void) const;
#endif

protected:	// protected data
	bool					got_pipe_info;
	IRenderPipeline			*render_pipe;

	IDAComponent			*system_services;

	char					*extref_path;

	U32						library_state[ITL_STATE_MAX];

	U32						irp_needs_square;
	U32						irp_max_texture_width;
	U32						irp_max_texture_height;

	ITL_TEXTUREFORMATMAP	*texture_format_maps;
	U32 num_texture_format_maps;


	//EMAURER maps ITL_TEXTURE_IDs to ITL_TEXTUREOBJECTs
	mutable ID_MAP id_map;
	//EMAURER maps strings to ITL_TEXTURE_IDs
	NAME_MAP name_map;	

#ifdef DATHREADSAFE
	mutable CRITICAL_SECTION archetype_lock;
	mutable CRITICAL_SECTION instance_lock;
#endif

	U32 next_texture_id;	//EMAURER an ever-increasing ID generator. no already-allocated checks done.

	REF_ID_MAP ref_map;	//EMAURER maps ITL_TEXTUREREF_IDs to reference objects.
	U32 next_ref_id;	//EMAURER an ever-increasing ID generator. no already-allocated checks done.
};

//ALLOCLITE_GLOBAL (ID_MAP_ALLOC)
//ALLOCLITE_GLOBAL (NAME_MAP_ALLOC)
//ALLOCLITE_GLOBAL (REF_ID_MAP_ALLOC)

#ifdef DATHREADSAFE	
	#define ECRIT(x) EnterCriticalSection (&x)
#else
	#define ECRIT(x) ((void)0)
#endif

#ifdef DATHREADSAFE	
	#define LCRIT(x) LeaveCriticalSection (&x)	
#else
	#define LCRIT(x) ((void)0)
#endif

// ------------------------------------------------------------------

TextureLibrary::TextureLibrary(void)
{
#ifdef DATHREADSAFE
	InitializeCriticalSection (&archetype_lock);
	InitializeCriticalSection (&instance_lock);
#endif

	next_texture_id = 1;
	next_ref_id = 1;

	render_pipe = NULL;
	system_services = NULL;

	num_texture_format_maps = 0;
	texture_format_maps = NULL;
	
	extref_path = NULL;

	got_pipe_info = false;
}

#ifndef NDEBUG
void TextureLibrary::walk (void)
{
	ECRIT (archetype_lock);	
	ECRIT (instance_lock);

	REF_ID_MAP::const_iterator rit = ref_map.begin ();

	for (; rit != ref_map.end (); rit++)
	{
		NAME_MAP::const_iterator nameit = name_map.find ((*rit).second.texture->texture_name);
		ASSERT (nameit != name_map.end ());

		ID_MAP::const_iterator idit (id_map.find ((*nameit).second));
		ASSERT (idit != id_map.end ());

		ASSERT (&((*idit).second) == (*rit).second.texture);
	}

	//exercise the subobjects
	{
		ID_MAP::iterator idit = id_map.begin ();

		for (; idit != id_map.end (); idit++)
		{
			(*idit).second.addref ();
			relref_texture (&(*idit).second, false);
		}
	}
	
	LCRIT (instance_lock);
	LCRIT (archetype_lock);	
}
#endif

//

#ifndef FINAL_RELEASE
void TextureLibrary::report (void) const
{
	ECRIT (archetype_lock);	

	ECRIT (instance_lock);

	REF_ID_MAP::const_iterator rit = ref_map.begin ();

	for (; rit != ref_map.end (); rit++)
	{
		GENERAL_TRACE_1 (TEMPSTR("ITL_TEXTURE_REF_ID %d [refers to \'%s\', refs %d] is still active\n", rit->first, rit->second.texture->texture_name, rit->second.ref_count));
	}

	LCRIT (instance_lock);

	NAME_MAP::const_iterator it = name_map.begin ();

	for (; it != name_map.end (); it++)
	{
		ID_MAP::const_iterator obj = id_map.find (it->second);
		ASSERT (obj != id_map.end ());

		if (obj->second.ref_count) {
			GENERAL_TRACE_1 (TEMPSTR("ITL_TEXTURE_ID %d [refers to \'%s\'] has %d dangling references\n", it->second, (const char*)(it->first.c_str ()), obj->second.ref_count));
		}
	}

	LCRIT (archetype_lock);	
}
#endif

TextureLibrary::~TextureLibrary(void)
{
#ifndef FINAL_RELEASE
	report ();
#endif

	render_pipe = NULL;
	system_services = NULL;

	clear_texture_format_maps();

	delete[] extref_path;
	extref_path = NULL;

#ifdef DATHREADSAFE
	DeleteCriticalSection (&archetype_lock);
	DeleteCriticalSection (&instance_lock);
#endif
}

//

GENRESULT COMAPI TextureLibrary::Initialize(void)
{ 
	irp_needs_square = 0;
	irp_max_texture_height = 0;
	irp_max_texture_width = 0;

	if( FAILED( system_services->QueryInterface( IID_IRenderPipeline, (void**) &render_pipe ) ) ) {
		GENERAL_ERROR( "TextureLibrary: init: unable to acquire IID_IRenderPipeline" );
		return GR_GENERIC;
	}
	render_pipe->Release();


//	db.verify_texture_array( 128 );

	initialize_texture_format_maps();

	ICOManager *DACOM = DACOM_Acquire();

	COMPTR<IProfileParser> parser;
	if( SUCCEEDED( DACOM->QueryInterface( IID_IProfileParser, parser ) ) ) {
		float fl;
		U32 u32;
		
		opt_get_u32( DACOM, parser, CLSID_TextureLibrary, "TEXTURE_LOD_LOAD", 1, &u32 ) ;
		set_library_state( ITL_STATE_TEXTURE_LOD_LOAD, u32);	

		opt_get_float( DACOM, parser, CLSID_TextureLibrary, "TEXTURE_LOD_LOAD_BIAS", 0.0, &fl ) ;
		set_library_state( ITL_STATE_TEXTURE_LOD_LOAD_BIAS, *((U32*)&fl) );
		
		opt_get_float( DACOM, parser, CLSID_TextureLibrary, "TEXTURE_LOD_LOAD_SCALE", 1.0, &fl ) ;
		set_library_state( ITL_STATE_TEXTURE_LOD_LOAD_SCALE, *((U32*)&fl) );
		
		opt_get_u32( DACOM, parser, CLSID_TextureLibrary, "TEXTURE_LOD_LOAD_MIN", 0, &u32 ) ;
		set_library_state( ITL_STATE_TEXTURE_LOD_LOAD_MIN, u32 );
		
		opt_get_u32( DACOM, parser, CLSID_TextureLibrary, "TEXTURE_DIM_SQUARE", TRUE, &u32 ) ;
		set_library_state( ITL_STATE_TEXTURE_DIM_SQUARE, u32 );
		
		opt_get_u32( DACOM, parser, CLSID_TextureLibrary, "TEXTURE_DIM_POW2", TRUE, &u32 ) ;
		set_library_state( ITL_STATE_TEXTURE_DIM_POW2, u32 );

		opt_get_u32( DACOM, parser, CLSID_TextureLibrary, "TEXTURE_LOAD_MODE", ITL_LM_USE_FIRST, &u32 ) ;
		set_library_state( ITL_STATE_TEXTURE_LOAD_MODE, u32 );

		char path[1024+1];
		opt_get_string( DACOM, parser, CLSID_TextureLibrary, "ExternalReferencesPath", "", path, 1024 );
		delete extref_path;
		extref_path = NULL;
		if( path[0] != 0 ) {
			extref_path = new char[ strlen(path) + 1 ];
			strcpy( extref_path, path );
		}
	}

	return GR_OK; 
}

//

HRESULT TextureLibrary::get_pipe_info( void )
{
	ASSERT( render_pipe );

	// TODO: make this function correctly in DX9

	irp_needs_square = 0;
	irp_max_texture_width = 2048;
	irp_max_texture_height = 2048;

	//render_pipe->query_device_ability( RP_A_TEXTURE_SQUARE_ONLY, &irp_needs_square );
	//render_pipe->query_device_ability( RP_A_TEXTURE_MAX_WIDTH, &irp_max_texture_width );
	//render_pipe->query_device_ability( RP_A_TEXTURE_MAX_HEIGHT, &irp_max_texture_height );

	got_pipe_info = true;

	return S_OK;
}

//

GENRESULT TextureLibrary::init( AGGDESC *desc )
{ 
	// This is called during normal use.  We are a normal aggregate.
	// Specifically, this is a system aggregate.
	//
	
	system_services = desc->outer;

	return GR_OK;
}

//

GENRESULT TextureLibrary::set_library_state( ITL_STATE state, U32 value ) 
{ 
	library_state[state] = value;
	return GR_OK;
}

//

GENRESULT TextureLibrary::get_library_state( ITL_STATE state, U32 *out_value ) 
{ 
	*out_value = library_state[state];
	return GR_OK;
}

//

HRESULT TextureLibrary::massage_texture_dimension( U32 width, U32 height, U32 &out_width, U32 &out_height )
{
	U32 high_bit, bit_cnt;

	out_width = width;
	out_height = height;
	
	if( !width || !height ) {
		return E_FAIL;
	}

	switch( library_state[ITL_STATE_TEXTURE_DIM_POW2] ) {
	
	case 1:	// magnify
		if( out_width & (out_width-1) ) {
			high_bit = out_width;
			bit_cnt = 0;
			while( high_bit != 1 ) {
				high_bit >>= 1;
				bit_cnt++;
			}
			
			out_width = 1<<(bit_cnt+1);
		}

		if( out_height & (out_height-1) ) {
			high_bit = out_height;
			bit_cnt = 0;
			while( high_bit != 1 ) {
				high_bit >>= 1;
				bit_cnt++;
			}
			
			out_height = 1<<(bit_cnt+1);
		}
		break;

	case 2:	// goto nearest
		if( out_width & (out_width-1) ) {
			high_bit = out_width;
			bit_cnt = 0;
			while( high_bit != 1 ) {
				high_bit >>= 1;
				bit_cnt++;
			}
			
			out_width = 1<<(bit_cnt+1);
		}

		if( out_height & (out_height-1) ) {
			high_bit = out_height;
			bit_cnt = 0;
			while( high_bit != 1 ) {
				high_bit >>= 1;
				bit_cnt++;
			}
			
			out_height = 1<<(bit_cnt+1);
		}
		break;
	
	}

	switch( library_state[ITL_STATE_TEXTURE_DIM_SQUARE] ) {
	
	case 1:	
		if( out_width == out_height || irp_needs_square == 0 ) {
			break;
		}

	case 2:
		out_width = __max( out_width, out_height );
		out_height = out_width;
	}

	return S_OK;
}

//

HRESULT TextureLibrary::choose_texture_format( ITL_TEXTUREIMAGEDATA *data, PixelFormat *out_pf )
{
#if 1
	int bit_count = 0;

	if( data->format.has_alpha_channel() || data->alphas ) {

		int a_min, a_max, a_diff, only_two_values=1;
		unsigned char a;

		if( data->alphas ) {
			a_min = 100000;
			a_max = 0;
			for( U32 y=0; y<data->height; y++ ) {
				for( U32 x=0; x<data->width; x++ ) {
					a = (unsigned char)data->alphas[data->width * y + x];
					a_min = __min( a_min, (int)a );
					a_max = __max( a_max, (int)a );

					if( a != a_min && a != a_max ) {
						only_two_values = 0;
					}
				}
			}

			a_diff = a_max - a_min;
			bit_count = 1;
			if( a_diff != 0 && !only_two_values ) {
				for( ; a_diff != 1; a_diff>>=1, bit_count++ );
			}
		}
		else {
			bit_count = data->format.num_a_bits();
		}
	}


	for( U32 tfm=0; tfm<num_texture_format_maps; tfm++ ) {
		
		if( texture_format_maps[tfm].is_member_of( data->format, bit_count ) ) {
			out_pf->init( texture_format_maps[tfm].fourcc, 0 );
			return S_OK;
		}
	}

	return E_FAIL;

#else
	out_pf->init( data->format );

	if( data->format.has_alpha_channel() || data->alphas ) {

		int a_min, a_max, a_diff, bit_count, only_two_values=1;
		unsigned char a;

		if( data->alphas ) {
			a_min = 100000;
			a_max = 0;
			for( U32 y=0; y<data->height; y++ ) {
				for( U32 x=0; x<data->width; x++ ) {
					a = (unsigned char)data->alphas[data->width * y + x];
					a_min = __min( a_min, (int)a );
					a_max = __max( a_max, (int)a );

					if( a != a_min && a != a_max ) {
						only_two_values = 0;
					}
				}
			}

			a_diff = a_max - a_min;
			bit_count = 1;
			if( a_diff != 0 && !only_two_values ) {
				for( ; a_diff != 1; a_diff>>=1, bit_count++ );
			}
		}
		else {
			bit_count = data->format.num_a_bits();
			if( bit_count > 1 ) {
				only_two_values = 0;
			}
		}

			
		// Find one big enough
		//
		a_min = bit_count + 1;
		a_max = 0;
		for( U32 f=0; f<num_desired_texture_formats[ITL_ALPHA]; f++ ) {
			a_diff = bit_count - desired_texture_formats[ITL_ALPHA][f].num_a_bits();
			if( a_diff <= 0 ) {
				// Big enough to serve our needs.
				out_pf->init( desired_texture_formats[ITL_ALPHA][f] );
				return S_OK;
			}
			if( a_diff < a_min ) {
				a_max = f;	// save index of minimum difference
			}
		}

		// Couldn't find one, use the one with the minimum difference
		//
		if( num_desired_texture_formats[ITL_ALPHA] ) {
			out_pf->init( desired_texture_formats[ITL_ALPHA][a_max] );
			return S_OK;
		}

		// No registered texture formats
		//
		if( bit_count > 1 ) {
			out_pf->init( 16, 4, 4, 4, 4 );
		}
		else {
			out_pf->init( 16, 5, 5, 5, 1 );
		}
	}
	else {

#if 1
		if( num_desired_texture_formats[ITL_OPAQUE] ) {
			out_pf->init( desired_texture_formats[ITL_OPAQUE][0] );
		}
		else {
			out_pf->init( 16, 5, 6, 6, 0 );
		}
#else

		int g_min, g_idx, g_diff, bit_count;

		bit_count = data->format.is_indexed() ? 8 : data->format.num_g_bits();
			
		// Find one big enough
		//
		g_min = bit_count + 1;
		g_idx = 0;
		for( U32 f=0; f<num_desired_texture_formats[ITL_OPAQUE]; f++ ) {
			g_diff = bit_count - desired_texture_formats[ITL_OPAQUE][f].num_g_bits();
			if( g_diff <= 0 ) {
				// Big enough to serve our needs.
				out_pf->init( desired_texture_formats[ITL_OPAQUE][f] );
				return S_OK;
			}
			if( g_diff < g_min ) {
				g_idx = f;	// save index of minimum difference
			}
		}

		// Couldn't find one, use the one with the minimum difference
		//
		if( num_desired_texture_formats[ITL_OPAQUE] ) {
			out_pf->init( desired_texture_formats[ITL_OPAQUE][g_idx] );
			return S_OK;
		}

		// No registered texture formats
		//
		out_pf->init( 16, 5, 6, 5, 0 );
#endif

	}
	return S_OK;

#endif

}

//

HRESULT TextureLibrary::load_texture_format( IFileSystem *IFS, PixelFormat *out_format, char *out_format_dir, U32 max_len )
{
	if( IFS->SetCurrentDirectory( txm_colors8bit ) ) {
		U32 len = __min( max_len, strlen(txm_colors8bit) );
		strncpy( out_format_dir, txm_colors8bit, len );
		out_format_dir[len] = 0;
		out_format->init( 8, 0,0,0,0 );
		IFS->SetCurrentDirectory( ".." );
		return S_OK;
	}
	else if( IFS->SetCurrentDirectory( txm_colors16bit ) ) {
		U32 len = __min( max_len, strlen(txm_colors16bit) );
		strncpy( out_format_dir, txm_colors16bit, len );
		out_format_dir[len] = 0;
		out_format->init( 16, 5,6,5,0 );
		IFS->SetCurrentDirectory( ".." );
		return S_OK;
	}
	else if( IFS->SetCurrentDirectory( "True 8 bit" ) ) {
		U32 len = __min( max_len, strlen("True 8 bit") );
		strncpy( out_format_dir, "True 8 bit", len );
		out_format_dir[len] = 0;
		out_format->init( 8, 8,0,0,0 );
		IFS->SetCurrentDirectory( ".." );
		return S_OK;
	}
	else {
		HANDLE hSearch;
		WIN32_FIND_DATA	SearchData;
		if( (hSearch = IFS->FindFirstFile( "Format_*", &SearchData )) != INVALID_HANDLE_VALUE ) {
			IFS->FindClose( hSearch );
			U32 len = __min( max_len, strlen(SearchData.cFileName) );
			strncpy( out_format_dir,  SearchData.cFileName, len );
			out_format_dir[len] = 0;
			out_format->unpersist( SearchData.cFileName );
			return S_OK;
		}
	}

	return E_FAIL;
}

//

HRESULT TextureLibrary::load_texture_image( IFileSystem *IFS, ITL_TEXTUREIMAGEDATA &defaults, ITL_TEXTUREIMAGEDATA **out_data )
{
	ITL_TEXTUREIMAGEDATA *img;
	U32 image_size = 0;
	U32 image_bpp = 0;

//	const char *txm_cstr[5] = { txm_indices, txm_indices, txm_colors, txm_colors, txm_colors  };

	if( (img = new ITL_TEXTUREIMAGEDATA()) == NULL ) {
		return GR_GENERIC;
	}

	if( FAILED( read_type<U32>( IFS, txm_width, &img->width ) ) ) {
		if( defaults.width == 0) {
			goto load_texture_image_failed;
		}
		img->width = defaults.width;
	}

	if( FAILED( read_type<U32>( IFS, txm_height, &img->height ) ) ) {
		if( defaults.height == 0 ) {
			goto load_texture_image_failed;
		}
		img->height = defaults.height;
	}

	// TODO: is this load_texture_format really necessary?
	//
	char format_dir[255+1];
	if( FAILED( load_texture_format( IFS, &img->format, format_dir, 255 ) ) ) {
		if( defaults.format.ddpf.dwRGBBitCount == 0 ) {
			goto load_texture_image_failed;
		}
		img->format.init( defaults.format );
	}

	image_bpp = (img->format.ddpf.dwRGBBitCount + 7) >> 3;
	img->stride = img->width * image_bpp;
	image_size = img->height * img->stride;

	if( img->format.is_indexed() ) {
		U32 num_colors=0;
		if( SUCCEEDED( read_type<U32>( IFS, txm_palcount, &num_colors ) ) ) {
			if( SUCCEEDED( read_chunk( IFS, txm_paldata, num_colors * sizeof(img->palette[0]), (char*)&img->palette[0] ) ) ) {
				num_colors = 1000;
			}
		}

		// Use default palette
		if( num_colors != 1000 ) {
			memcpy( &img->palette[0], &defaults.palette[0], sizeof(img->palette[0]) * 256 );
		}
	}

	U8 dummy;
	if( SUCCEEDED( read_type<U8>( IFS, txm_alpha8bit, &dummy ) ) ) {
		U32 alpha_size = img->width * img->height;
		if( (img->alphas = new char[alpha_size]) == NULL ) {
			goto load_texture_image_failed;
		}

		if( FAILED( read_chunk( IFS, txm_alpha8bit, alpha_size, img->alphas ) ) ) {
			goto load_texture_image_failed;
		}
	}

	if( (img->colors = new char[image_size]) == NULL ) {
		goto load_texture_image_failed;
	}

	if( img->format.is_indexed() ) {
		if( FAILED( read_chunk( IFS, txm_indices, image_size, img->colors ) ) ) {
			goto load_texture_image_failed;
		}
	}
	else {
		if( FAILED( read_chunk( IFS, txm_colors, image_size, img->colors ) ) ) {
			goto load_texture_image_failed;
		}
	}

	*out_data = img;
	return S_OK;


load_texture_image_failed:
	delete img;
	*out_data = NULL;
	return E_FAIL;
}

//

HRESULT TextureLibrary::load_texture_mipmap_count( IFileSystem *IFS, U32 *out_mipmapcount )
{
	HANDLE hMipSearch;
	WIN32_FIND_DATA	MipSearchData;
	U32 uMipCount = 0;

	// Count the number of mipmaps, take into account whether we are supposed to load mips.
	// if uMipCount==0, then no mipmaps, otherwise uMipCount == total number of mipmap levels.
	//
	if( (hMipSearch = IFS->FindFirstFile( "MIP*", &MipSearchData )) != INVALID_HANDLE_VALUE ) {
		while( IFS->FindNextFile( hMipSearch, &MipSearchData ) ) {
			uMipCount++;
		}
		uMipCount++;

		IFS->FindClose( hMipSearch );
	}

	if( uMipCount && (library_state[ITL_STATE_TEXTURE_LOD_LOAD] == FALSE) ) {
		uMipCount = 1;
	}

	*out_mipmapcount = uMipCount;
	return S_OK;
}

//

inline void TextureLibrary::insert_name (const char* name, ITL_TEXTURE_ID hndl)
{
	ASSERT (strlen (name) < TXM_NAME_LEN);
	std::pair<NAME_MAP::iterator, bool> pr = name_map.insert (NAME_MAP::value_type (name, hndl));
	//EMAURER assure that an insertion occurred, implying that there 
	//was not already an element named 'name.'
	ASSERT (pr.second);
}


HRESULT TextureLibrary::load_guts (U32 uMipLevelSet, IFileSystem* IFS, ITL_TEXTUREIMAGEDATA& overrides, U32 uM, U32& rp_id)
{
	ITL_TEXTUREIMAGEDATA *image_data = NULL;
	TL_LOG("load_guts: before load_texture_image\n");
	if( SUCCEEDED( load_texture_image( IFS, overrides, &image_data ) ) ) {
		TL_LOGF("load_guts: image loaded %dx%d\n", image_data ? image_data->width : -1, image_data ? image_data->height : -1);

		PixelFormat texture_pf;

		if( uMipLevelSet == 0 ) {
			choose_texture_format( image_data, &texture_pf );

			U32 w, h;
			massage_texture_dimension( image_data->width, image_data->height, w, h );

			TL_LOGF("load_guts: before create_texture %dx%d mips=%d\n", w, h, uM);
			if( FAILED( render_pipe->create_texture( w, h, texture_pf, uM, 0, rp_id) ) ) {
				GENERAL_TRACE_1( "Unable to create renderpipe texture\n" );
				TL_LOG("load_guts: create_texture FAILED\n");
				delete image_data;
				return E_FAIL;
			}
			TL_LOGF("load_guts: create_texture OK rp_id=%u\n", rp_id);
		}

		TL_LOG("load_guts: before set_texture_level_data\n");
		render_pipe->set_texture_level_data( rp_id, uMipLevelSet, image_data->width, image_data->height, image_data->stride, image_data->format, image_data->colors, image_data->alphas, image_data->palette );
		TL_LOG("load_guts: after set_texture_level_data\n");

		delete image_data;
	} else {
		TL_LOG("load_guts: load_texture_image FAILED\n");
	}

	return S_OK;
}

bool TextureLibrary::build_normal_object (U32 rp_id, const char* tname, ITL_TEXTURE_ID& hndl, bool boost_ref)
{
	bool insertion = false;

	//EMAURER Is it still true that the object being loaded is not in the database?
	//The db was locked, checked, and unlocked. Between the unlock and this lock
	//someone may have inserted the texture name that is being loaded.

	ECRIT (archetype_lock);

	NAME_MAP::const_iterator it = name_map.find (tname);

	if (it != name_map.end ())
	{
		hndl = it->second;
		ITL_TEXTUREOBJECT& texture = id_map[hndl];

		if( library_state[ITL_STATE_TEXTURE_LOAD_MODE] == ITL_LM_USE_LAST ) {

			texture.rp_texture_id = rp_id;
			texture.set_name (tname);
			
			// DO NOT reset the ref count as we want all outstanding
			// references to remain valid
			// texture.ref_count = 0; 

			texture.set_frame_id( 0, 0 );
			texture.set_frame_rect( 0 );
			texture.set_frame_rate( 0.00F );
			texture.set_frame_texture (0, &texture);

			insertion = true;	// not really an insertion, but we need to return
								// success nonetheless
		}

		if( boost_ref ) {
			texture.addref ();
		}
	}
	else
	{
		// Initialize frame and texture id data
		//

		//EMAURER allocate handle
		hndl = next_texture_id++;

		ITL_TEXTUREOBJECT& texture = id_map[hndl];

		texture.rp_texture_id = rp_id;
		texture.set_name (tname);
		texture.ref_count = 0;

		texture.set_frame_id( 0, 0 );
		texture.set_frame_rect( 0 );
		texture.set_frame_rate( 0.00F );
		texture.set_frame_texture (0, &texture);

		insert_name (tname, hndl);
		insertion = true;

		if (boost_ref)
			texture.addref ();
	}

	ASSERT (name_map.size () == id_map.size ());

	LCRIT (archetype_lock);

	return insertion;
}


HRESULT TextureLibrary::load_texture_normal_old( IFileSystem *IFS, const char* tname, ITL_TEXTURE_ID& hndl, bool boost_ref)
{
	ITL_TEXTUREIMAGEDATA overrides;
	
	U32 rp_id = HTX_INVALID;

	GENERAL_TRACE_5( "load_texture_normal_old: file containes textures in the old format, recommend re-exporting\n" );

	// Read all miplevels, or just the image data if no mipmaps
	//
	U32 uMipCount, uMipLevelRead=0, uMipLevelSet;

	load_texture_mipmap_count( IFS, &uMipCount );

	if( uMipCount == 0 ) {
		GENERAL_WARNING( TEMPSTR( "Texture '%s' has no valid mipmap levels", tname ) );
		return E_FAIL;
	}

	for( uMipLevelSet=0; (uMipLevelSet==0) || (uMipLevelRead<uMipCount); uMipLevelRead++ ) {

		char dir_name[20+1];
		sprintf( dir_name, "MIP%d", uMipLevelRead ); 
		if( uMipCount && !IFS->SetCurrentDirectory( dir_name ) ) {
			continue;
		}

		read_type<U32>( IFS, txm_width, &overrides.width );
		read_type<U32>( IFS, txm_height, &overrides.height );
		
		if( (overrides.width <= irp_max_texture_width) && (overrides.height <= irp_max_texture_height) ) {

			// Change to format directory
			char format_dir[255+1];
			if( FAILED( load_texture_format( IFS, &overrides.format, format_dir, 255 ) ) ) {
				return E_FAIL;
			}
			
			if( !IFS->SetCurrentDirectory( format_dir ) ) {
				return E_FAIL;
			}

			load_guts (uMipLevelSet, IFS, overrides, uMipCount-uMipLevelRead, rp_id);
		}

		IFS->SetCurrentDirectory( ".." );	

		uMipLevelSet++;

		if( uMipCount && !IFS->SetCurrentDirectory( ".." ) ) {
			continue;
		}
	} // for each mip level

	if (!build_normal_object (rp_id, tname, hndl, boost_ref))
		render_pipe->destroy_texture (rp_id);

	return S_OK;
}

//

HRESULT TextureLibrary::load_texture_normal_new( IFileSystem *IFS, const char* tname, ITL_TEXTURE_ID& hndl, bool boost_ref)
{
	ITL_TEXTUREIMAGEDATA overrides;
	
	// Load optional dims for all mipmap levels.	
	read_type<U32>( IFS, txm_width, &overrides.width );
	read_type<U32>( IFS, txm_height, &overrides.height );

	// Change to format directory
	char format_dir[255+1];
	if( FAILED( load_texture_format( IFS, &overrides.format, format_dir, 255 ) ) ) {
		return E_FAIL;
	}
	
	if( !IFS->SetCurrentDirectory( format_dir ) ) {
		return E_FAIL;
	}

	// Load optional palette for all mipmap levels.	
	read_chunk( IFS, txm_paldata, 256 * sizeof(overrides.palette[0]), (char*)&overrides.palette[0] );


	// Read all miplevels, or just the image data if no mipmaps
	//
	U32 uMipCount, uMipLevelRead=0, uMipLevelSet;

	load_texture_mipmap_count( IFS, &uMipCount );

	if( uMipCount == 0 ) {
		GENERAL_WARNING( TEMPSTR( "Texture '%s' has no valid mipmap levels", tname ) );
		return E_FAIL;
	}

	// Handle mipmap lod load bias... 
	//
	if( (overrides.width > library_state[ITL_STATE_TEXTURE_LOD_LOAD_MIN]) && (overrides.height > library_state[ITL_STATE_TEXTURE_LOD_LOAD_MIN]) ) {
	
		float bias = *((float*)&library_state[ITL_STATE_TEXTURE_LOD_LOAD_BIAS]);
		float scale = *((float*)&library_state[ITL_STATE_TEXTURE_LOD_LOAD_SCALE]);
		
		U32 start_lod = uMipCount - __min( ((U32) ( (scale * uMipCount) + bias + 0.5f ) ), uMipCount );

		uMipLevelRead = __min( uMipCount-1, start_lod );

		if( overrides.width ) {
			overrides.width >>= uMipLevelRead;
		}
		if( overrides.height ) {
			overrides.height >>= uMipLevelRead;
		}
	}

	//EMAURER the renderpipe id for this texture.
	U32 rp_id = HTX_INVALID;

	for( uMipLevelSet=0; (uMipLevelSet==0) || (uMipLevelRead<uMipCount); uMipLevelRead++ ) {

		if( (overrides.width <= irp_max_texture_width) && (overrides.height <= irp_max_texture_height) ) {
			
			char dir_name[20+1];
			sprintf( dir_name, "MIP%d", uMipLevelRead ); 
			if( uMipCount && !IFS->SetCurrentDirectory( dir_name ) ) {
				continue;
			}

			load_guts (uMipLevelSet, IFS, overrides, uMipCount-uMipLevelRead, rp_id);

			uMipLevelSet++;

			if( uMipCount && !IFS->SetCurrentDirectory( ".." ) ) {
				continue;
			}
		}
	
		if( overrides.width ) {
			overrides.width >>= 1;
		}
		if( overrides.height ) {
			overrides.height >>= 1;
		}

	} // for each mip level
	
	IFS->SetCurrentDirectory( ".." );

	if (!build_normal_object (rp_id, tname, hndl, boost_ref))
		render_pipe->destroy_texture (rp_id);

	return S_OK;
}

//

HRESULT TextureLibrary::load_texture_normal_1_6( IFileSystem *IFS, const char* tname, ITL_TEXTURE_ID& hndl, bool boost_ref)
{
	U32		i, uMipCount, uMipLevelRead=0, rp_id	=HTX_INVALID;
	char	mipTxt[6];
	U32		width=0, height=0;	//silence compiler warnings

	load_texture_mipmap_count(IFS, &uMipCount);
	
	if( uMipCount == 0 ) {
		GENERAL_WARNING( TEMPSTR( "Texture '%s' has no valid mipmap levels", tname ) );
		return E_FAIL;
	}
	
	//load the tga into a mem chunk
	for(i=0;(!i) || (uMipLevelRead < uMipCount);uMipLevelRead++)
	{
		U32						fsize;
		char					*buf;
		TGA_Header				header;
		ITL_TEXTUREIMAGEDATA	*img;

		sprintf(mipTxt, "MIP%d", i);

		{
			DAFILEDESC	desc(mipTxt);
			HANDLE		h;

			if((h = IFS->OpenChild(&desc)) != INVALID_HANDLE_VALUE)
			{
				fsize	=IFS->GetFileSize(h);

				IFS->CloseHandle(h);
			}
			else
			{
				continue;
			}
		}

		buf	=new char[fsize];

		if( FAILED( read_chunk(IFS, mipTxt, fsize, (char*)buf)))
		{
			delete	[]buf;
			return	E_FAIL;
		}

		//grab header outta buf
		memcpy(&header, buf, sizeof(header));

		//check it
		if(header.verify())
		{
			if(!i)
			{
				width	=header.width;
				height	=header.height;

				//Handle mipmap lod load bias... 
				if((width > library_state[ITL_STATE_TEXTURE_LOD_LOAD_MIN]) && (height > library_state[ITL_STATE_TEXTURE_LOD_LOAD_MIN]))
				{
					float	bias	=*((float*)&library_state[ITL_STATE_TEXTURE_LOD_LOAD_BIAS]);
					float	scale	=*((float*)&library_state[ITL_STATE_TEXTURE_LOD_LOAD_SCALE]);
					
					U32		start_lod	=uMipCount - __min(((U32)((scale * uMipCount) + bias + 0.5f)), uMipCount);

					uMipLevelRead	=__min(uMipCount-1, start_lod);

					if(width)
					{
						width	>>=uMipLevelRead;
					}
					if(height)
					{
						height	>>=uMipLevelRead;
					}
				}
			}

			if((width <= irp_max_texture_width) && (height <= irp_max_texture_height))
			{
				int	k, z;

				if((img = new ITL_TEXTUREIMAGEDATA()) == NULL)
				{
					return	GR_GENERIC;
				}

				if(header.is_indexed() && header.has_palette())
				{
					img->format.init(8, 0, 0, 0, 0);
				}
				else
				{
					if(header.bits_per_pixel == 16)
					{
						img->format.init(16, 5, 5, 5, 0);
					}
					else
					{
						if(header.is_alpha())
						{
							img->format.init(32, 8, 8, 8, 8);
						}
						else
						{
							img->format.init(24, 8, 8, 8, 0);
						}
					}
				}

				//swap color elements.  Do this in texturelib load too... seems
				//redundant, but the actual on disk tga format is backwards
				if(img->format.is_indexed())
				{
					ASSERT(header.is_indexed() && header.has_palette());

					for(k=0;k < 256;k++)
					{
						for(z=0;z < (header.bits_per_color>>3);z++)
						{
							*(((char *)img->palette) + (k * (header.bits_per_color>>3)) + z)
								=*(buf + sizeof(header) + header.identsize
								+ (k * (header.bits_per_color>>3)) + (header.bits_per_color>>3) - z - 1);
						}
					}
				}

				img->width	=width;
				img->height	=height;

				img->stride	=img->width * ((img->format.ddpf.dwRGBBitCount + 7) >> 3);

				if((img->colors = new char[img->height * img->stride]) == NULL)
				{
					return	E_FAIL;
				}

				//our d3d format is 0, 0 is lower right

				if(!header.is_y_flipped())
				{
					// Y axis is flipped w/ respect to D3D orientation.
					if(header.is_x_flipped())
					{
						// X & Y axis are flipped from the D3D orientation
						// Must do a pixel by pixel copy.
						if(img->format.is_indexed())
						{
							for(k=0;k < img->height;k++)
							{
								for(z=0;z < img->width;z++)
								{
									*((img->colors + (k * img->stride)) + img->width - z - 1)
										=*(buf + (sizeof(header)+header.identsize + header.num_colors()*3) + (k * img->stride) + z);
								}
							}
						}
						else
						{
							for(k=0;k < img->height;k++)
							{
								for(z=0;z < img->stride;z++)
								{
									*((img->colors + (k * img->stride)) + img->stride - z - 1)
										=*(buf + (sizeof(header)+header.identsize) + (k * img->stride) + z);
								}
							}
						}
					}
					else
					{
						// Y axis is flipped (i.e. image origin is lower left).
						// Must do a line by line copy.
						if(img->format.is_indexed())
						{
							for(k=0;k < img->height;k++)
							{
								memcpy(img->colors + ((img->height - k - 1) * img->stride),
									buf + (sizeof(header)+header.identsize + header.num_colors()*3) + (k * img->stride),
									img->stride);
							}
						}
						else
						{
							for(k=0;k < img->height;k++)
							{
								memcpy(img->colors + ((img->height - k - 1) * img->stride),
									buf + (sizeof(header)+header.identsize) + (k * img->stride),
									img->stride);
							}
						}
					}
				}
				else
				{
					// Y axis is NOT flipped w/ respect to D3D orientation.
					if(header.is_x_flipped())
					{
						// X axis is flipped
						// Must do a pixel by pixel copy.
						if(img->format.is_indexed())
						{
							for(k=0;k < img->height;k++)
							{
								for(z=0;z < img->width;z++)
								{
									*(img->colors + ((img->height - k - 1) * img->stride) + img->width - z - 1)
										=*(buf + (sizeof(header)+header.identsize + header.num_colors()*3) + (k * img->stride) + z);
								}
							}
						}
						else
						{
							for(k=0;k < img->height;k++)
							{
								for(z=0;z < img->stride;z++)
								{
									*(img->colors + ((img->height - k - 1) * img->stride) + img->stride - z - 1)
										=*(buf + (sizeof(header)+header.identsize) + (k * img->stride) + z);
								}
							}
						}
					}
					else
					{
						// Neither X nor Y are flipped
						// Can do a giant block copy (assuming that width and stride are the same.
						if(img->format.is_indexed())
						{
							memcpy(img->colors, buf + (sizeof(header)+header.identsize + header.num_colors()*3), img->height * img->stride);
						}
						else
						{
							memcpy(img->colors, buf + (sizeof(header)+header.identsize), img->height * img->stride);
						}
					}
				}

				PixelFormat	texture_pf;

				if(i==0)
				{
					U32	w, h;

					choose_texture_format(img, &texture_pf);

					massage_texture_dimension(img->width, img->height, w, h);

					if(FAILED(render_pipe->create_texture(w, h, texture_pf, uMipCount-uMipLevelRead, 0, rp_id)))
					{
						GENERAL_TRACE_1("Unable to create renderpipe texture\n");
						delete	img;
						return	E_FAIL;
					}
				}
				
				render_pipe->set_texture_level_data(rp_id, i, img->width, img->height, img->stride, img->format, img->colors, img->alphas, img->palette);

				delete	img;

				i++;
			}
		}

		if(width)
		{
			width	>>=1;
		}
		if(height)
		{
			height	>>=1;
		}
		delete	buf;
	}

	if(!build_normal_object(rp_id, tname, hndl, boost_ref))
	{
		render_pipe->destroy_texture(rp_id);
	}

	return	S_OK;
}

//

HRESULT TextureLibrary::load_texture_normal( IFileSystem *IFS, const char* tname, ITL_TEXTURE_ID& hndl, bool boost_ref) 
{
	U32 xs;

	// Normal texture, possibly mipped
	//
	if( IFS->SetCurrentDirectory( "MIP0" ) ) {
		IFS->SetCurrentDirectory( ".." );
		return load_texture_normal_old( IFS, tname, hndl, boost_ref);
	}
	else if( SUCCEEDED( read_type<U32>( IFS, "Image X size", &xs ) ) ) {
		return load_texture_normal_new( IFS, tname, hndl, boost_ref);
	}
	else {
		return load_texture_normal_1_6( IFS, tname, hndl, boost_ref);
	}
}

//

HRESULT TextureLibrary::load_texture_animated( IFileSystem *Parent_IFS, IFileSystem *Anim_IFS, const char* texture_name, ITL_TEXTURE_ID& hndl, bool boost_ref)
{ 
	// Animated texture
	//
	U32 texture_count = 0;
	if( FAILED( read_type<U32>( Anim_IFS, tlib_axmtcount, &texture_count ) ) ) {
		return E_FAIL;
	}

	//EMAURER load all textures causing each one to be connected
	//to the parent.

	//EMAURER for multithreading, load each texture before creating the
	//parent. editing parent requires a database lock.

	//As each individual texture is loaded, boost it's ref count. Once
	//the compound texture object is assembled at the end of this fn,
	//release the refs, but do not delete the object if it went to 0.
	//This is advertised behavior.

	ITL_TEXTURE_ID* children = new ITL_TEXTURE_ID[texture_count];

	char txm_name[TXM_NAME_LEN];
	for( U32 txm=0; txm<texture_count; txm++ ) {
		sprintf( txm_name, "%s_%u", texture_name, txm );

		if (GR_OK != load_texture (Parent_IFS, txm_name, children[txm], true))
		{
			//EMAURER this compound texture cannot now be constructed correctly.
			//Free up the sub-textures that have been loaded.
			ECRIT (archetype_lock);
			for (U32 i = 0; i < txm; i++)
			{
				ID_MAP::iterator ch = id_map.find (children[i]);

				if (ch != id_map.end ())
					relref_texture (&(*ch).second, true);
			}

			LCRIT (archetype_lock);
				
			delete [] children;
			return E_FAIL;
		}
	}

	float fps = 0.0;
	
	if( FAILED( read_type<float>( Anim_IFS, tlib_axmfps, &fps) ) ) {
		fps = 24.0F;
	}

	U32 num_frames = 0;
	if( FAILED( read_type<U32>( Anim_IFS, tlib_axmfcount, &num_frames ) ) ) {
		return E_FAIL;
	}

	ITL_TEXTUREFRAME *frames = new ITL_TEXTUREFRAME[num_frames];

	if( FAILED( read_chunk( Anim_IFS, tlib_axmfrects, sizeof(ITL_TEXTUREFRAME)*(num_frames), (char*)frames ) ) ) {
		delete[] frames;
		return E_FAIL;
	}

	ECRIT (archetype_lock);

	//EMAURER Is it still true that the object being loaded is not in the database?
	//The db was locked, checked, and unlocked. Between the unlock and this lock
	//someone may have inserted the texture name that is being loaded.

#ifdef DATHREADSAFE
	NAME_MAP::const_iterator it = name_map.find (texture_name);

	if (it != name_map.end ())
		hndl = it->second;
	else
#endif
	{
		//EMAURER alloc/setup object and then append it to the database.

		hndl = next_texture_id++;

		ITL_TEXTUREOBJECT& texture = id_map[hndl];

		texture.set_name( texture_name );

		texture.frames.resize (num_frames);

		FRAME_VECTOR::iterator dst = texture.frames.begin ();

		for (ITL_TEXTUREFRAME* src = frames;
			src < frames + num_frames; 
			dst++, src++ ) {
			dst->set_rect (src->u0, src->v0, src->u1, src->v1 );
			dst->set_id( src->texture_id_idx );
		}

		texture.set_frame_rate( fps );

		//EMAURER construct parent, and connect

		texture.frame_texture_objs.resize (texture_count);

		OBJPTR_VECTOR::iterator it = texture.frame_texture_objs.begin ();

		for(U32 txm=0;
			txm<texture_count; 
			txm++, it++ )
		{
			ID_MAP::iterator ch = id_map.find (children[txm]);

			if (ch != id_map.end ())
			{
				*it = &(*ch).second;

				//EMAURER we boosted the ref count above. Take it down
				//without removing zero ref objects. 
				relref_texture (*it, false);
			}
			else
				ASSERT (0 && "part of animated texture missing in action");
		}

		insert_name (texture_name, hndl);
	}

	if (boost_ref)
	{
		ID_MAP::iterator idit (id_map.find (hndl));
	
		if (idit != id_map.end ())
			(*idit).second.addref ();
	}

	ASSERT (name_map.size () == id_map.size ());

	LCRIT (archetype_lock);

	delete[] frames;
	delete [] children;

	return S_OK;
}

//

HRESULT TextureLibrary::load_texture_extref( IFileSystem *IFS, const char* texture_name, ITL_TEXTURE_ID& hndl, bool boost_ref)
{
	char filename[MAX_PATH+1];
	char pathname[MAX_PATH+1], *fn;

	if( FAILED( read_string( IFS, "Filename", MAX_PATH, filename ) ) ) {
		return E_FAIL;
	}

	if( !SearchPath( extref_path, filename, NULL, MAX_PATH, pathname, &fn ) ) {
		return E_FAIL;
	}

	float fps;
	if( SUCCEEDED( read_type<float>( IFS, tlib_axmfps, &fps ) ) ) {
		return load_texture_extref_animated( filename, pathname, IFS, texture_name, hndl, boost_ref);
	}

	return load_texture_extref_normal( filename, pathname, IFS, texture_name, hndl, boost_ref);
}

//

HRESULT TextureLibrary::load_texture_extref_normal( const char *filename, const char *fullpath, IFileSystem *IFS, const char*, ITL_TEXTURE_ID&, bool boost_ref)
{
	return E_FAIL;
}

//

HRESULT TextureLibrary::load_texture_extref_animated( const char *filename, 
														const char *fullpath, 
														IFileSystem *IFS, 
														const char* texture_name, 
														ITL_TEXTURE_ID& hndl,
														bool boost_ref)
{
	hndl = ITL_INVALID_ID;

	U32 width=0, height=0;
	if( FAILED( read_type<U32>( IFS, txm_width, &width ) ) ) {
		return E_FAIL;
	}

	if( FAILED( read_type<U32>( IFS, txm_height, &height ) ) ) {
		return E_FAIL;
	}

	U32 tw, th, flags = 0;
	char pformat[128+1];
	PixelFormat pf;

	read_type<U32>( IFS, "Texture create flags", &flags );

	massage_texture_dimension( width, height, tw, th );

	if( FAILED( read_string( IFS, "Texture format", 128, pformat ) ) ) {
		strcpy( pformat, "Format_TRUE_3__5_6_5" );
	}

	pf.unpersist( pformat );

	U32 rp_texture_id = HTX_INVALID;

	if( FAILED( render_pipe->create_texture( tw, th, pf, 1, flags | IRP_CTF_VIDEO_TARGET, rp_texture_id ) ) ) {
		GENERAL_TRACE_1( "load_texture_extref_animated: unable to create renderpipe texture\n" );
		return E_FAIL;
	}

	COMPTR<IVideoStreamControl> vid;

	HRESULT hr = E_FAIL;
	if( SUCCEEDED( render_pipe->get_texture_interface( rp_texture_id, IID_IVideoStreamControl, vid) ) ) {
		if( SUCCEEDED( vid->load( fullpath, NULL, IVSC_F_AUTOSTART|IVSC_F_STREAM ) ) ) {
			hr = S_OK;
		}
	}
	
	ECRIT (archetype_lock);	

	//EMAURER Is it still true that the object being loaded is not in the database?
	//The db was locked, checked, and unlocked. Between the unlock and this lock
	//another thread may have inserted the texture that is being loaded.

#ifdef DATHREADSAFE
	NAME_MAP::const_iterator it = name_map.find (texture_name);

	if (it != name_map.end ())
	{
		hndl = it->second;
		render_pipe->destroy_texture (rp_texture_id);
	}
	else
#endif
	{
		//EMAURER allocate handle
		hndl = next_texture_id++;

		insert_name (texture_name, hndl);
		
		// Initialize frame and texture id data
		//
		
		ITL_TEXTUREOBJECT& texture = id_map[hndl];
		texture.rp_texture_id = rp_texture_id;
		texture.set_frame_id( 0, 0 );
		texture.set_frame_rect( 0 );
		texture.set_frame_rate( 0.00F );
		texture.set_name (texture_name);
		texture.set_frame_texture (0, &texture);

		texture._IVideoStreamControl = vid;

		if (boost_ref)
			texture.addref ();
	}

	ASSERT (id_map.size () == name_map.size ());

	LCRIT (archetype_lock);	

	return hr;
}

//

#ifndef NDEBUG
#include <stdio.h>

static bool trash = true;

void TextureLibrary::dump (void)
{
	ECRIT (archetype_lock);	

	FILE* f;

	if (trash)
		f = fopen ("txmdump.txt", "w");
	else
		f = fopen ("txmdump.txt", "a");

	trash = false;
		
	NAME_MAP::const_iterator it = name_map.begin ();

	fputs ("BEGIN ARCHETYPE DUMP\n" ,f);

	for (; it != name_map.end (); it++)
	{
		ID_MAP::const_iterator obj = id_map.find (it->second);
		assert (obj != id_map.end ());

		char buf[256];
		sprintf (buf, " texture \'%s\' has id %d, refs %d\n", (const char*)(it->first.c_str ()), it->second, obj->second.ref_count);
		fputs (buf, f);
	}

	fputs ("END ARCHETYPE DUMP\n", f);

	LCRIT (archetype_lock);	

	ECRIT (instance_lock);

	fputs ("BEGIN REF DUMP\n", f);

	REF_ID_MAP::const_iterator rit = ref_map.begin ();

	for (; rit != ref_map.end (); rit++)
	{
		char buf[256];
		sprintf (buf, " ref %d refers to arch %s, refs %d\n", rit->first, rit->second.texture->texture_name, rit->second.ref_count);
		fputs (buf, f);
	}

	LCRIT (instance_lock);

	fputs ("END REF DUMP\n", f);
	fclose (f);
}
#endif	

GENRESULT TextureLibrary::load_texture( IFileSystem *_IFS, const char *full_texture_name, ITL_TEXTURE_ID& out, bool boost_ref)
{
	char texture_name[MAX_PATH], fname[_MAX_FNAME], ext[_MAX_EXT];

	_splitpath( full_texture_name, NULL, NULL, fname, ext );
	_makepath( texture_name, NULL, NULL, fname, ext );

	out = ITL_INVALID_ID;

	ECRIT (archetype_lock);	
	
	NAME_MAP::const_iterator it = name_map.find (texture_name);

	if (it != name_map.end ())
	{
		out = it->second;

		if (boost_ref)
		{
			ID_MAP::iterator idit (id_map.find (out));
			ASSERT (idit != id_map.end ());
			(*idit).second.addref ();
		}
	}

	LCRIT (archetype_lock);	

	// if the load mode is not first, then we don't really care about the
	// face that this texture was previously loaded
	//
	if( (out != ITL_INVALID_ID) && (library_state[ITL_STATE_TEXTURE_LOAD_MODE] == ITL_LM_USE_FIRST) )
		return GR_OK;

	if( !got_pipe_info ) {	// we have to do this here too because load_texture
		get_pipe_info();	// can be called by the outside world before any load_library
	}						// is called.


	IComponentFactory	*ICF = _IFS;
	if( ICF == NULL ) {
		ICF = DACOM_Acquire ();
	}

	const char *directories[3] = { NULL, tlib_txmname, tlib_axmname };
	U32 dir, in = 0;
	HRESULT hr = E_FAIL;

	for( dir=0; dir<3; dir++ ) {
		if( directories[dir] ) {
			if( (in = _IFS->SetCurrentDirectory( directories[dir] )) == 0 ) {
				continue;
			}
		}

		COMPTR<IFileSystem> IFS;
		DAFILEDESC desc( full_texture_name );

		if( SUCCEEDED( ICF->CreateInstance( &desc, IFS ) ) ) {

			U32 texture_count;
			if( SUCCEEDED( read_type<U32>( IFS, tlib_axmtcount, &texture_count ) ) ) {
				// back parent fs back up one directory (if necessary) 
				// this allows load_texture_animated() to load the child textures
				// correctly in all cases.
				//
				if( in ) {
					BOOL r = _IFS->SetCurrentDirectory( ".." );
					ASSERT (r);
				}

				hr = load_texture_animated( _IFS, IFS, texture_name, out, boost_ref );
				
				if( in ) {
					BOOL r = _IFS->SetCurrentDirectory( directories[dir] );	
					ASSERT (r);
				}
				break;
			}
			else if( SUCCEEDED( read_type<U32>( IFS, "Filename", &texture_count ) ) ) {
				hr = load_texture_extref( IFS, texture_name, out, boost_ref );
				break;
			}
			else {
				hr = load_texture_normal( IFS, texture_name, out, boost_ref);
				break;
			}
		}

		if( in ) {
			BOOL r = _IFS->SetCurrentDirectory( ".." );
			ASSERT (r);
			in = 0;
		}
	}

	if( in ) {
		BOOL r = _IFS->SetCurrentDirectory( ".." );
		ASSERT (r);
		in = 0;
	}

	if (!out)
	{
		int i = 42;
		i++;
	}

	if( FAILED( hr ) ) {
		return GR_GENERIC;
	}

	
	return GR_OK;
}


GENRESULT TextureLibrary::load_texture( IFileSystem *_IFS, const char *full_texture_name ) 
{ 
	ITL_TEXTURE_ID dummy;
	GENRESULT ret = load_texture (_IFS, full_texture_name, dummy, false);

#ifndef FINAL_RELEASE
	ECRIT (archetype_lock);

	ASSERT (name_map.size () == id_map.size ());

	LCRIT (archetype_lock);
#endif

	return ret;
}

//

GENRESULT TextureLibrary::load_library( IFileSystem *IFS, const char *library_name ) 
{ 
		//return GR_GENERIC;
		// TODO: Make this work with DX9
		// -Ryan

	if( IFS == NULL ) {
		GENERAL_WARNING( "IFileSystem argument is NULL in load_library" );
		return GR_GENERIC;
	}
	
	if( !got_pipe_info ) get_pipe_info();

	WIN32_FIND_DATA TxmSearchData;
	HANDLE hTxmSearch;

	if( library_name == NULL ) {
		library_name = tlib_txmname;
	}

	bool got_any = false;

	// Load Texture Library.
	//
	TL_LOGF("load_library: trying SetCurrentDirectory('%s')\n", library_name);
	if( IFS->SetCurrentDirectory( library_name ) ) {
		TL_LOG("load_library: entered Texture library dir\n");

		if( (hTxmSearch = IFS->FindFirstFile( tlib_search, &TxmSearchData )) != INVALID_HANDLE_VALUE ) {
			do {
				if( TxmSearchData.cFileName[0] != '.' ) {
					TL_LOGF("load_library: loading texture '%s'\n", TxmSearchData.cFileName);
					if (GR_OK != load_texture( IFS, TxmSearchData.cFileName ))
						GENERAL_WARNING (TEMPSTR ("failed to load \'%s\'", TxmSearchData.cFileName));
					TL_LOGF("load_library: done texture '%s'\n", TxmSearchData.cFileName);
				}
			} while( IFS->FindNextFile( hTxmSearch, &TxmSearchData ) );

			IFS->FindClose (hTxmSearch);
		} else {
			TL_LOG("load_library: FindFirstFile returned INVALID_HANDLE_VALUE\n");
		}

		IFS->SetCurrentDirectory( ".." );
		got_any = true;
	} else {
		TL_LOG("load_library: Texture library dir not found\n");
	}

	TL_LOGF("load_library: trying SetCurrentDirectory('%s')\n", tlib_axmname);
	if( IFS->SetCurrentDirectory( tlib_axmname ) ) {
		TL_LOG("load_library: entered Animation library dir\n");

		if( (hTxmSearch = IFS->FindFirstFile( tlib_search, &TxmSearchData )) != INVALID_HANDLE_VALUE ) {
			do {
				if( TxmSearchData.cFileName[0] != '.' ) {
					TL_LOGF("load_library: loading anim texture '%s'\n", TxmSearchData.cFileName);
					if (GR_OK != load_texture( IFS, TxmSearchData.cFileName ))
						GENERAL_WARNING (TEMPSTR ("failed to load \'%s\'", TxmSearchData.cFileName));
					TL_LOGF("load_library: done anim texture '%s'\n", TxmSearchData.cFileName);
				}
			} while( IFS->FindNextFile( hTxmSearch, &TxmSearchData ) );

			IFS->FindClose (hTxmSearch);
		} else {
			TL_LOG("load_library: FindFirstFile (anim) returned INVALID_HANDLE_VALUE\n");
		}

		IFS->SetCurrentDirectory( ".." );
		got_any = true;
	} else {
		TL_LOG("load_library: Animation library dir not found\n");
	}

	TL_LOGF("load_library: returning %s\n", got_any ? "GR_OK" : "GR_GENERIC");
	return got_any? GR_OK : GR_GENERIC;
}

//

GENRESULT TextureLibrary::free_library( BOOL release_all )
{
	//EMAURER the nature of the reference database is such that
	//it should never have entries with a 0 ref count, unlike the
	//texture object database which can have 0 ref objects
	//after a call to load_library ().
	//

	if (release_all)
	{
		ECRIT (instance_lock);

		REF_ID_MAP::const_iterator rit = ref_map.begin ();

		while (rit != ref_map.end ())
		{
			while (_release_texture_ref (rit->first))
				;

			rit = ref_map.begin ();
		}

		LCRIT (instance_lock);
	}

	ECRIT (archetype_lock);	

	ID_MAP::iterator it = id_map.begin ();

	while (it != id_map.end ())
	{
		if (release_all || it->second.ref_count <= 0)
		{
			if( it->second.rp_texture_id != HTX_INVALID ) {
				render_pipe->destroy_texture( it->second.rp_texture_id );
			}

			name_map.erase (it->second.texture_name);
			it = id_map.erase (it);
		}
		else
			it++;
	}

	ASSERT (name_map.size () == id_map.size ());

	LCRIT (archetype_lock);	

	return GR_OK;
}

//

GENRESULT TextureLibrary::update( SINGLE dt ) 
{
	ECRIT (archetype_lock);	
	ECRIT (instance_lock);

	REF_ID_MAP::iterator beg = ref_map.begin();
	REF_ID_MAP::iterator end = ref_map.end();
	REF_ID_MAP::iterator tex;

	for( tex = beg; tex != end; tex++ ) {
		(*tex).second.update( dt );
	}
	
	LCRIT (instance_lock);
	LCRIT (archetype_lock);	

	return GR_OK;
}

//

//
//EMAURER assume already in critical section
void TextureLibrary::relref_texture( ITL_TEXTUREOBJECT* texture, bool remove_at_zero)
{
	// subtextures
	if( texture->frames.size () > 1 ) {
		for( OBJPTR_VECTOR::iterator idit = texture->frame_texture_objs.begin (); 
			idit != texture->frame_texture_objs.end ();
			idit++ ) {
			relref_texture (*idit, remove_at_zero);
		}
	}

	// release big-daddy ref count
	//

	texture->ref_count--;

	if( (texture->ref_count <= 0) && remove_at_zero) {
		NAME_MAP::iterator it = name_map.find (texture->texture_name);
		ASSERT (it != name_map.end ());

		ID_MAP::iterator idit = id_map.find (it->second);
		ASSERT (idit != id_map.end ());

		if( idit->second.rp_texture_id != HTX_INVALID ) {
			render_pipe->destroy_texture( idit->second.rp_texture_id );
		}

		id_map.erase (idit);
		name_map.erase (it);

		ASSERT (id_map.size () == name_map.size ());
	}
}

//
 
GENRESULT TextureLibrary::get_texture_id( const char *texture_name, ITL_TEXTURE_ID *out_texture_id ) 
{
	GENRESULT result = GR_GENERIC;

	ECRIT (archetype_lock);	

	NAME_MAP::const_iterator it = name_map.find (texture_name);

	if (it != name_map.end ())
	{
		ID_MAP::iterator ot = id_map.find (it->second);

		ASSERT (ot != id_map.end ());

		if( out_texture_id ) {
			ot->second.addref ();
			*out_texture_id = it->second;
		}
		result = GR_OK;
	}
	else
	{
		ITL_TEXTURE_ID tid;
		(void)build_normal_object (HTX_INVALID, texture_name, tid, false);
		result = get_texture_id (texture_name, out_texture_id);
	}

	LCRIT (archetype_lock);	

	return result;
}

//

GENRESULT TextureLibrary::release_texture_id( ITL_TEXTURE_ID texture_id )
{
	GENRESULT result = GR_GENERIC;

	ECRIT (archetype_lock);	
	
	ID_MAP::iterator it = id_map.find (texture_id);

	if (it != id_map.end ())
	{
		relref_texture (&it->second, true);
		result = GR_OK;
	}

	ASSERT (id_map.size () == name_map.size ());

	LCRIT (archetype_lock);	

	return result;
}

//

GENRESULT TextureLibrary::has_texture_id( const char *texture_name )
{
	GENRESULT result;

	ECRIT (archetype_lock);	

	result = (name_map.end () != name_map.find (texture_name)) ? GR_OK : GR_GENERIC;

	LCRIT (archetype_lock);	

	return result;
}

//

GENRESULT TextureLibrary::get_texture_name( ITL_TEXTURE_ID texture_id, char *out_texture_name, U32 max_buf_size ) 
{
	GENRESULT result = GR_GENERIC;

	out_texture_name[0] = 0;

	ECRIT (archetype_lock);	

	ID_MAP::const_iterator it = id_map.find (texture_id);

	if (it != id_map.end ())
	{
		U32 len = __min( strlen( it->second.texture_name ), max_buf_size );
		memcpy( out_texture_name, it->second.texture_name, len );
		out_texture_name[len] = 0;
		result = GR_OK;
	}

	LCRIT (archetype_lock);	

	return result;
}

//

GENRESULT TextureLibrary::set_texture_name( ITL_TEXTURE_ID texture_id, const char *texture_name) 
{
	GENRESULT gr = GR_GENERIC;

	ECRIT (archetype_lock);

	NAME_MAP::const_iterator nit = name_map.find (texture_name);

	if (nit != name_map.end ())
	{
		//EMAURER name not already in the database.
		ID_MAP::iterator it = id_map.find (texture_id);

		if (it != id_map.end ())
		{
			name_map.erase (it->second.texture_name);
			it->second.set_name (texture_name);
			insert_name (texture_name, it->first);

			gr = GR_OK;
		}
	}

	LCRIT (archetype_lock);	

	return gr;
}

//

GENRESULT TextureLibrary::set_texture_frame_texture( ITL_TEXTURE_ID texture_id, U32 txm_id_idx, U32 rp_texture_id ) 
{
	GENRESULT result = GR_GENERIC;
	ID_MAP::iterator it;

	ECRIT (archetype_lock);	

	ID_MAP::iterator dst = id_map.find (texture_id);
	if (dst == id_map.end ()) {
		LCRIT( archetype_lock );
		return GR_GENERIC;
	}

	for( it = id_map.begin(); it != id_map.end();	it++ ) {

		if( it->second.rp_texture_id == rp_texture_id ) {

			if (txm_id_idx < dst->second.frame_texture_objs.size ())
				relref_texture (dst->second.frame_texture_objs[txm_id_idx], true);

			if (dst->first != it->first)
				it->second.addref ();

			dst->second.set_frame_texture (txm_id_idx, &(it->second));
			result = GR_OK;

			break;
		}
	}
	
	if(it == id_map.end())	//texture not found
	{
		char			tempName[64];
		ITL_TEXTURE_ID	ignoredTexId;

		sprintf(tempName, "Anon%i", (U32)rp_texture_id);

		if(!build_normal_object(rp_texture_id, tempName, ignoredTexId, false))
		{
			result	=GR_GENERIC;
		}
		else
		{
			ID_MAP::iterator frame_texture = id_map.find( ignoredTexId );
			dst->second.set_frame_texture (txm_id_idx, &(frame_texture->second));
			result	=GR_OK;
		}
	}

	LCRIT (archetype_lock);

	return result;
}

//

GENRESULT TextureLibrary::set_texture_frame_id( ITL_TEXTURE_ID texture_id, U32 frame_num, U32 txm_id_idx  ) 
{
	GENRESULT result = GR_GENERIC;

	ECRIT (archetype_lock);	

	ID_MAP::iterator it = id_map.find (texture_id);

	if (it != id_map.end ())
	{
		if( frame_num == ITL_FRAME_LAST ) {
			frame_num = it->second.frames.size () - 1;
		}
		if( frame_num < it->second.frames.size ()) {
			it->second.set_frame_id( frame_num, txm_id_idx );
			result = GR_OK;
		}
	}

	LCRIT (archetype_lock);	

	return result;
}

//

GENRESULT TextureLibrary::set_texture_frame_rect( ITL_TEXTURE_ID texture_id, U32 frame_num, float u0, float v0, float u1, float v1 )
{
	GENRESULT result = GR_GENERIC;

	ECRIT (archetype_lock);	

	ID_MAP::iterator it = id_map.find (texture_id);

	if (it != id_map.end ())
	{
		if( frame_num == ITL_FRAME_LAST ) {
			frame_num = it->second.frames.size () - 1;
		}
		if( frame_num < it->second.frames.size () ) {
			it->second.set_frame_rect( frame_num, u0, v0, u1, v1 );
			result = GR_OK;
		}
	}

	LCRIT (archetype_lock);	

	return result;
}

//

GENRESULT TextureLibrary::get_texture_frame( ITL_TEXTURE_ID texture_id, U32 frame_num, ITL_TEXTUREFRAME_IRP *frame  ) 
{
	GENRESULT result = GR_GENERIC;

	ECRIT (archetype_lock);	

	ID_MAP::iterator it = id_map.find (texture_id);

	if (it != id_map.end ())
	{
		if( frame_num == ITL_FRAME_LAST ) {
			frame_num = it->second.frames.size () - 1;
		}
		if( frame_num < it->second.frames.size () ) {
			frame->rp_texture_id = it->second.frame_texture_objs[ it->second.frames[frame_num].texture_id_idx ]->rp_texture_id;
			frame->u0			 = it->second.frames[frame_num].u0;
			frame->v0			 = it->second.frames[frame_num].v0;
			frame->u1			 = it->second.frames[frame_num].u1;
			frame->v1			 = it->second.frames[frame_num].v1;
			result = GR_OK;
		}
	}

	LCRIT (archetype_lock);	

	if (result != GR_OK)
	{
		frame->rp_texture_id = HTX_INVALID;
		frame->u0 = 0;
		frame->v0 = 0;
		frame->u1 = 1;
		frame->v1 = 1;
	}

	return result;
}

//

GENRESULT TextureLibrary::set_texture_frame_rate( ITL_TEXTURE_ID texture_id, float fps_rate )
{
	GENRESULT result = GR_GENERIC;

	ECRIT (archetype_lock);	

	ID_MAP::iterator it = id_map.find (texture_id);

	if (it != id_map.end ())
	{
		it->second.frames_per_sec = fps_rate;
		result = GR_OK;
	}

	LCRIT (archetype_lock);	

	return result;
}

//

GENRESULT TextureLibrary::get_texture_frame_rate( ITL_TEXTURE_ID texture_id, float *out_fps_rate )
{
	GENRESULT result = GR_GENERIC;
	*out_fps_rate = 0.00F;

	ECRIT (archetype_lock);	

	ID_MAP::iterator it = id_map.find (texture_id);

	if (it != id_map.end ())
	{
		*out_fps_rate = it->second.frames_per_sec;
		result = GR_OK;
	}

	LCRIT (archetype_lock);	

	return result;
}

//

GENRESULT TextureLibrary::get_texture_frame_count( ITL_TEXTURE_ID texture_id, U32 *out_frame_count )
{
	GENRESULT result = GR_GENERIC;
	*out_frame_count = 0;

	ECRIT (archetype_lock);	

	ID_MAP::iterator it = id_map.find (texture_id);

	if (it != id_map.end ())
	{
		*out_frame_count = it->second.frames.size ();
		result = GR_OK;
	}

	LCRIT (archetype_lock);	

	return result;
}

//

GENRESULT TextureLibrary::add_ref_texture_id( ITL_TEXTURE_ID texture_id, ITL_TEXTURE_REF_ID *out_texture_ref_id ) 
{
	GENRESULT result = GR_GENERIC;

	if( out_texture_ref_id ) {
		*out_texture_ref_id = ITL_INVALID_REF_ID;
	}

	ECRIT (archetype_lock);	

	ID_MAP::iterator it = id_map.find (texture_id);

	if (it != id_map.end ())
	{
		it->second.addref ();
		result = GR_OK;
	}

	if( out_texture_ref_id && (result == GR_OK)) {
		ECRIT (instance_lock);	

		ITL_TEXTURE_REF_ID nu = next_ref_id++;

		ITL_TEXTUREOBJECTREF& p = ref_map[nu];
		p.init (&(it->second));
		p.ref_count++;

		LCRIT (instance_lock);	
		*out_texture_ref_id = nu;
	}

	LCRIT (archetype_lock);	

	return result;
}

//


GENRESULT TextureLibrary::add_ref_texture_ref( ITL_TEXTURE_REF_ID texture_ref_id ) 
{
	GENRESULT result = GR_GENERIC;

	if (texture_ref_id != ITL_INVALID_REF_ID)
	{
		//EMAURER this order is important for prevention of deadlock
		ECRIT (archetype_lock);	
		ECRIT (instance_lock);	

		REF_ID_MAP::iterator it = ref_map.find (texture_ref_id);

		if (it != ref_map.end ())
		{
			it->second.ref_count++;
			result = GR_OK;
		}

		LCRIT (instance_lock);	
		LCRIT (archetype_lock);	
	}

	return result;
}

//

int TextureLibrary::_release_texture_ref( ITL_TEXTURE_REF_ID texture_ref_id )
{
	int result = -1;

	if (texture_ref_id != ITL_INVALID_REF_ID)
	{
		//EMAURER this order is important for prevention of deadlock.
		//always obtain archetype_lock before instance_lock
		ECRIT (archetype_lock);	
		ECRIT (instance_lock);	

		REF_ID_MAP::iterator it = ref_map.find (texture_ref_id);

		if (it != ref_map.end ())
		{
			result = --(it->second.ref_count);
			ASSERT (it->second.ref_count >= 0);

			if (!it->second.ref_count)
			{
				relref_texture (it->second.texture, true);
				ref_map.erase (it);
			}
		}

		LCRIT (instance_lock);	
		LCRIT (archetype_lock);	
	}
	
	return result;
}

GENRESULT TextureLibrary::release_texture_ref( ITL_TEXTURE_REF_ID texture_ref_id ) 
{
	GENRESULT result = GR_GENERIC;

	if (-1 != _release_texture_ref (texture_ref_id))
		result = GR_OK;

	return result;
}

//

GENRESULT TextureLibrary::get_texture_ref_texture_id( ITL_TEXTURE_REF_ID texture_ref_id, ITL_TEXTURE_ID *out_texture_id )
{
	GENRESULT result = GR_GENERIC;

	if (texture_ref_id != ITL_INVALID_REF_ID)
	{
		//EMAURER this order is important for prevention of deadlock.
		//always obtain archetype_lock before instance_lock
		ECRIT (archetype_lock);	
		ECRIT (instance_lock);	

		REF_ID_MAP::iterator it = ref_map.find (texture_ref_id);

		if (it != ref_map.end ())
		{
			it->second.texture->addref ();

			ID_MAP::const_iterator cit = id_map.begin ();

			for (; cit != id_map.end (); cit++)
			{
				if (&(cit->second) == it->second.texture)
				{
					*out_texture_id = cit->first;
					result = GR_OK;
					break;
				}
			}
		}

		LCRIT (instance_lock);	
		LCRIT (archetype_lock);	
	}

	return result;
}


//

#define BEGIN_TEXTURE_REF_FN_BOILERPLATE \
	GENRESULT result = GR_GENERIC; \
\
	if (texture_ref_id != ITL_INVALID_REF_ID) \
	{ \
		ECRIT (instance_lock); \
\
		REF_ID_MAP::iterator it = ref_map.find (texture_ref_id); \
\
		if (it != ref_map.end ()) \
		{ 


#define END_TEXTURE_REF_FN_BOILERPLATE \
			result = GR_OK; \
		} \
\
		LCRIT (instance_lock); \
	} \
\
	return result;

GENRESULT TextureLibrary::get_texture_ref_frame( ITL_TEXTURE_REF_ID texture_ref_id, U32 frame_num, ITL_TEXTUREFRAME_IRP *out_frame ) 
{
	BEGIN_TEXTURE_REF_FN_BOILERPLATE
	it->second.get_frame (frame_num, out_frame);
	END_TEXTURE_REF_FN_BOILERPLATE
}

//

GENRESULT TextureLibrary::set_texture_ref_frame_time( ITL_TEXTURE_REF_ID texture_ref_id, float frame_time ) 
{
	BEGIN_TEXTURE_REF_FN_BOILERPLATE
	it->second.set_frame_time (frame_time);
	END_TEXTURE_REF_FN_BOILERPLATE
}

//

GENRESULT TextureLibrary::get_texture_ref_frame_time( ITL_TEXTURE_REF_ID texture_ref_id, float *out_frame_time  ) 
{
	BEGIN_TEXTURE_REF_FN_BOILERPLATE
	*out_frame_time = it->second.frame_running_time;
	END_TEXTURE_REF_FN_BOILERPLATE
}

//

GENRESULT TextureLibrary::set_texture_ref_frame_num( ITL_TEXTURE_REF_ID texture_ref_id, U32 frame_num ) 
{
	BEGIN_TEXTURE_REF_FN_BOILERPLATE
	it->second.set_frame_number (frame_num);
	END_TEXTURE_REF_FN_BOILERPLATE
}

//

GENRESULT TextureLibrary::get_texture_ref_frame_num( ITL_TEXTURE_REF_ID texture_ref_id, U32 *out_frame_num  ) 
{
	BEGIN_TEXTURE_REF_FN_BOILERPLATE
	*out_frame_num = it->second.frame_num;
	END_TEXTURE_REF_FN_BOILERPLATE
}

//

GENRESULT TextureLibrary::set_texture_ref_frame_rate( ITL_TEXTURE_REF_ID texture_ref_id, float fps_rate ) 
{
	BEGIN_TEXTURE_REF_FN_BOILERPLATE
	it->second.set_frame_rate (fps_rate);
	END_TEXTURE_REF_FN_BOILERPLATE
}

//

GENRESULT TextureLibrary::get_texture_ref_frame_rate( ITL_TEXTURE_REF_ID texture_ref_id, float *out_fps_rate ) 
{
	BEGIN_TEXTURE_REF_FN_BOILERPLATE
	*out_fps_rate = *(it->second.frames_per_sec);
	END_TEXTURE_REF_FN_BOILERPLATE
}

//

GENRESULT TextureLibrary::set_texture_ref_play_mode( ITL_TEXTURE_REF_ID texture_ref_id, ITL_PLAYCOMMAND play_command ) 
{
	BEGIN_TEXTURE_REF_FN_BOILERPLATE
	it->second.play_mode = play_command;
	END_TEXTURE_REF_FN_BOILERPLATE
}

//


GENRESULT TextureLibrary::get_texture_ref_play_mode( ITL_TEXTURE_REF_ID texture_ref_id, ITL_PLAYCOMMAND *out_play_command ) 
{
	BEGIN_TEXTURE_REF_FN_BOILERPLATE
	*out_play_command = it->second.play_mode;
	END_TEXTURE_REF_FN_BOILERPLATE
}

//

GENRESULT TextureLibrary::update_texture_ref( ITL_TEXTURE_REF_ID texture_ref_id, SINGLE dt )
{
	BEGIN_TEXTURE_REF_FN_BOILERPLATE
	it->second.update( dt );
	END_TEXTURE_REF_FN_BOILERPLATE
}

//

GENRESULT TextureLibrary::get_texture_count( U32 *out_num_textures ) 
{
	ECRIT (archetype_lock);	

	*out_num_textures = id_map.size ();

	LCRIT (archetype_lock);	

	return GR_OK;
}

//

GENRESULT TextureLibrary::get_texture( U32 texture_num, ITL_TEXTURE_ID *out_texture_id ) 
{
	GENRESULT result = GR_GENERIC;

	ECRIT (archetype_lock);	

	U32 ctr = 0;
	ID_MAP::iterator it;

	for (it = id_map.begin ();
		it != id_map.end () && (ctr < texture_num);
		it++, ctr++)
		;	//nothing

	if (it != id_map.end ())
	{
		result = GR_OK;

		if (out_texture_id)
		{
			it->second.addref ();
			*out_texture_id = it->first;
		}
	}

	LCRIT (archetype_lock);	

	return result;
}

//

GENRESULT TextureLibrary::get_texture_format( ITL_TEXTURE_ID texture_id, U32 frame_num, PixelFormat *out_texture_format ) 
{
	GENRESULT result = GR_GENERIC;

	U32 rp_id = 0;

	ECRIT (archetype_lock);	

	ID_MAP::iterator it = id_map.find (texture_id);
	
	if (it != id_map.end ())
	{
		if( it->second.frames.size () > 0 ) {
			rp_id = it->second.frame_texture_objs[0]->rp_texture_id;
		}
	}

	LCRIT (archetype_lock);	

	if (rp_id)
		result = render_pipe->get_texture_format( rp_id, out_texture_format );
	else
		out_texture_format->init( 0,0,0,0,0 );

	return result;
}

//

HRESULT TextureLibrary::clear_texture_format_maps( void )
{
	delete[] texture_format_maps;
	texture_format_maps = NULL;
	
	num_texture_format_maps = 0;
	
	return S_OK;
}

//

HRESULT TextureLibrary::add_texture_format_map( int bpp, int r, int g, int b, int a, int alpha, U32 texture_class_fourcc, ITL_TEXTUREFORMATMAP **out )
{
	ITL_TEXTUREFORMATMAP *tfc;

	for( U32 tt=0; tt<num_texture_format_maps; tt++ ) {
//		if( texture_format_maps[tt].fourcc == fourcc && texture_format_classes[tt].num_formats == 0 ) {
//			return 	&texture_format_classes[tt];
//		}
	}

	if( (tfc = new ITL_TEXTUREFORMATMAP[ num_texture_format_maps + 1 ]) != NULL ) {
		if( num_texture_format_maps ) {
			memcpy( tfc, texture_format_maps, num_texture_format_maps * sizeof(ITL_TEXTUREFORMATMAP) );
			delete[] texture_format_maps;
		}
		texture_format_maps = tfc;

		texture_format_maps[num_texture_format_maps].init_wildcard( texture_class_fourcc );
		if( bpp >= 0 )	texture_format_maps[num_texture_format_maps].set_bpp( bpp );
		if( r >= 0 )	texture_format_maps[num_texture_format_maps].set_r( r );
		if( g >= 0 )	texture_format_maps[num_texture_format_maps].set_g( g );
		if( b >= 0 )	texture_format_maps[num_texture_format_maps].set_b( b );
		if( a >= 0 )	texture_format_maps[num_texture_format_maps].set_a( a );
		if( alpha >= 0 ) texture_format_maps[num_texture_format_maps].set_alpha( alpha );

		num_texture_format_maps++;
		*out = &texture_format_maps[num_texture_format_maps-1];
		return S_OK;
	}

	return E_FAIL;
}

//

//

HRESULT TextureLibrary::initialize_texture_format_maps( void )
{
	ITL_TEXTUREFORMATMAP *tfm = NULL;

	clear_texture_format_maps();

	// 8:8:8:8:*,0 = DAOP
	add_texture_format_map(  8,  8,  8,  8, -1,  0, MAKEFOURCC( 'D','A','O','P' ), &tfm ) ;

	// *:*:*:*:1,0 = DAA1
	add_texture_format_map( -1, -1, -1, -1,  1,  0, MAKEFOURCC( 'D','A','A','1' ), &tfm ) ;

	// *:*:*:*:4,0 = DAA4
	add_texture_format_map( -1, -1, -1, -1,  4,  0, MAKEFOURCC( 'D','A','A','4' ), &tfm ) ;

	// *:*:*:*:8,0 = DAA8
	add_texture_format_map( -1, -1, -1, -1,  8,  0, MAKEFOURCC( 'D','A','A','8' ), &tfm ) ;

	// *:*:*:*:*,0 = DAOT
	add_texture_format_map( -1, -1, -1, -1, -1,  0, MAKEFOURCC( 'D','A','O','T' ), &tfm ) ;

	// *:*:*:*:*,1 = DAA1
	add_texture_format_map( -1, -1, -1, -1, -1,  1, MAKEFOURCC( 'D','A','A','1' ), &tfm ) ;

	// *:*:*:*:*,4 = DAA4
	add_texture_format_map( -1, -1, -1, -1, -1,  4, MAKEFOURCC( 'D','A','A','4' ), &tfm ) ;

	// *:*:*:*:*,* = DAA8
	add_texture_format_map( -1, -1, -1, -1, -1, -1, MAKEFOURCC( 'D','A','A','8' ), &tfm ) ;


	
	// Read optional overrides from the ini file
	//

	ICOManager *DACOM = DACOM_Acquire();
	COMPTR<IProfileParser> IPP;
	
	char *p, szBuffer[1024+1], *mask, *fourcc, *n;
	int line = 0;
	HANDLE hTFC;

	if( SUCCEEDED( DACOM->QueryInterface( IID_IProfileParser, (void**) &IPP ) ) ) {
		if( (hTFC = IPP->CreateSection( "TextureFormatMaps" )) != 0 ) {
			
			while( IPP->ReadProfileLine( hTFC, line, szBuffer, 1024 ) ) {
				
				line++;

				if( (p = strchr( szBuffer, ';' )) != NULL ) {	// remove in-line comments
					*p = 0;
				}

				p = szBuffer;

				while( *p && strchr( " \t", *p ) ) p++;			// remove leading whitespace of lvalue

				if( strnicmp( p, "clear", 3 ) == 0 ) {
					clear_texture_format_maps();
					continue; // continue with next profile line
				}

				mask = p;

				while( *p && (*p != '=') ) p++;				// get '='
				*p = 0;	p++;								// skip '=', get rvalue

				while( *p && strchr( " \t", *p ) ) p++;		// remove leading whitespace of fourcc

				if( *p ) {
					fourcc = p;

					int r=-1,g=-1,b=-1,a=-1,alpha=-1,bpp=-1;

					// bpp
					n = mask;
					while( *mask && strchr( " \t", *mask ) ) mask++;			// remove leading whitespace of mask part
					while( *mask && strchr( "1234567890*", *mask ) ) mask++;	// find end of mask part
					*mask = 0;	*mask++;
					if( !strchr( n, '*' ) ) bpp = atoi( n );

					// r
					n = mask;
					while( *mask && strchr( " \t:", *mask ) ) mask++;			// remove leading whitespace of mask part
					while( *mask && strchr( "1234567890*", *mask ) ) mask++;	// find end of mask part
					*mask = 0;	*mask++;
					if( !strchr( n, '*' ) ) r = atoi( n );

					// g
					n = mask;
					while( *mask && strchr( " \t:", *mask ) ) mask++;			// remove leading whitespace of mask part
					while( *mask && strchr( "1234567890*", *mask ) ) mask++;	// find end of mask part
					*mask = 0;	*mask++;
					if( !strchr( n, '*' ) ) g = atoi( n );

					// b
					n = mask;
					while( *mask && strchr( " \t:", *mask ) ) mask++;			// remove leading whitespace of mask part
					while( *mask && strchr( "1234567890*", *mask ) ) mask++;	// find end of mask part
					*mask = 0;	*mask++;
					if( !strchr( n, '*' ) ) b = atoi( n );

					// a
					n = mask;
					while( *mask && strchr( " \t:", *mask ) ) mask++;			// remove leading whitespace of mask part
					while( *mask && strchr( "1234567890*", *mask ) ) mask++;	// find end of mask part
					*mask = 0;	*mask++;
					if( !strchr( n, '*' ) ) a = atoi( n );

					// alpha
					n = mask;
					while( *mask && strchr( " \t:", *mask ) ) mask++;			// remove leading whitespace of mask part
					while( *mask && strchr( "1234567890*", *mask ) ) mask++;	// find end of mask part
					*mask = 0;	*mask++;
					if( !strchr( n, '*' ) ) alpha = atoi( n );

					add_texture_format_map( bpp, r, g, b, a, alpha, MAKEFOURCC( fourcc[0], fourcc[1], fourcc[2], fourcc[3] ), &tfm );
				}
			}
			
			IPP->CloseSection( hTFC );
		}
	}

	return S_OK;
}

// ------------------------------------------------------------------
// DLL Related code
// ------------------------------------------------------------------
//--------------------------------------------------------------------------
// linker bug
void main (void)
{
}

//--------------------------------------------------------------------------
//  
BOOL COMAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it with DACOM manager
	//
		case DLL_PROCESS_ATTACH:
		{
			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE( hinstDLL );

			ICOManager *DACOM = DACOM_Acquire();
			IComponentFactory *server1;

			// Register System aggragate factory
			if( DACOM && (server1 = new DAComponentFactory2<DAComponentAggregate<TextureLibrary>, AGGDESC>(CLSID_TextureLibrary)) != NULL ) {
				DACOM->RegisterComponent( server1, CLSID_TextureLibrary, DACOM_NORMAL_PRIORITY );
				server1->Release();
			}
			
			break;
		}

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}


// EO
