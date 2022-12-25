#ifndef __MTL_TXT_H
#define __MTL_TXT_H

#include "misc.h"
#include "sgi_utf.h"
#include "rgbutils.h"
#include "imagefilter.h"
#include "vector.h"
#include "texturecoord.h"


//#define N_PALETTE_COLORS 256 // NOT 256 to reserve last color for chroma


//extern int mip_flag;
//extern int dither_flag;
//extern int flag_565;
//extern int txt_depth;

//typedef enum {PAL8, TRUE8, TRUE565, TRUE888, ENDT=INT_MAX} color_depth;

//typedef enum {BOGUS=0, AMBIENT, DIFFUSE, SPECULAR, EMISSION, SHININESS, TRANSPARENCY,
//			  BUMP, END4=INT_MAX} mtl_prop_type;
//typedef enum {TILE=0, CLAMP, END3=INT_MAX} wrap_mode;
//typedef enum {TXT_OFF=0, TXT_ON=1, END1=INT_MAX} txm_states; 
//typedef enum {RED=0, GREEN, BLUE, ALPHA, END0=INT_MAX} colors;

/*
typedef struct _Image {
	int width;
	int height;
	char name[256];
	unsigned char *red;
	unsigned char *green;
	unsigned char *blue;
	unsigned char *alpha;
}Image;

typedef struct _mip {
  char *level;                // MIP level #/name
  int x_size;                 // texture x dim in pixels
  int y_size;                 // texture y dim in pixels
  int color_count;            // # of colors, normally 255
  
  //palette8bit *palette_8_bit;
  //trueRGB565 *true_rgb_565;
  //true8bit *true_8_bit;		  // not yet used

  color_depth depth;
  unsigned char *palette_rgb_888;
  unsigned char *image_indices;

  unsigned char *image_colors;

  unsigned char *alpha_8_bit;
}mip;

typedef struct _txt {
  char *name;             // texture name
  int identifier;         // texture serial # (starts w/ 0)
  int mip_count;          // # of mip levels (1 for now)
  mip **mip_map;          // pointer to texture data
  int first_mip;
  int only_mip;

  // will be removed when new shaders are used by libraries
  int u_mode;			  // tile or clamp
  int v_mode;			  // tile or clamp
}txt;  

typedef struct _txt_lib {
  int count;              // # of textures
  txt **textures;         // pointer to textures
}txt_lib;

//struct _mtl_property; // forward declaration

struct FRAME_RECT {
	long texext; // texture name extention; txt_id
	float u0, v0, u1, v1;
};

typedef struct _anim_txt {
	char *name;        // w/o _# extension
	int txt_count;     // same as frame count if rect 0 - 1
	int frame_count;
	FRAME_RECT *rects; // one per frame if rect 0 - 1
	float fps;
	txt_lib *tl;
	_mtl_property *mp;  // points back to the material property using it
					    // this is a hack to avoid exporting the same frames twice since it is slow
						// same frame sequence is assumed
	
	char *file_name;	// for externally referenced avi's
	int width;
	int height;
}anim_txt;

typedef struct _anim_txt_lib {
	int count;
	anim_txt *list;
	txt_lib *tl;
}anim_txt_lib;

typedef struct _mtl_property
{
	mtl_prop_type type;
	float value[3];
	float u, v, h, w;  // used for uv remapping in MAX
	float blend;
	int texture_id;  
	
	char *anim_texture_name;
	char *texture_name;

	U32 flags;	

	int frame_count;
	float fps;
	int *frame_list;
	//float *time_list; // for key framing

	anim_txt_lib *atl;
	txt_lib *tl;

	bool pointed_to_by_atl;
} mtl_property;


typedef struct _mtl {
  char *name;              // material name
  char type[32];           // type name such as "DcDt"
  int identifier;          // material id (starts w/ 0)
  int api_id;
  int double_sided;
  
  mtl_property diffuse;
  mtl_property emission;
  mtl_property bump; // diffuse2
  mtl_property specular;
  mtl_property shininess;
  mtl_property transparency;
  mtl_property ambient;

  mtl_property* CC[3];	// points to the above depending on type
  int CC_len[3];
  mtl_property* TT[3];
}mtl;
*/

struct cq2Mtl 
{
  char *name;              // material name
  //char * type[32];           // type name such as "DcDt"
  int identifier;          // material id (starts w/ 0)
  int api_id;
  
};


typedef struct _mtl_lib {
  int cq2Count;
  cq2Mtl ** cq2Materials;
}mtl_lib;

/*
//
//targa stuff
#ifndef SGI
#pragma pack(push, 1)
#endif
struct TGA_Header
// TARGA FILE HEADER = 18 bytes
{
	U8	identsize;             // size of ID field that follows 18 byte header (0 usually)
	U8	palette;				// type of color map 0=none, 1=has palette
	U8	imagetype;             // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

	U16	color_start;			// first color map entry in palette
	U16	num_palette_colors;	// number of colors in palette
	U8	bits_per_color;		// number of bits per palette entry 15,16,24,32

	U16	xstart;             // image x origin
	U16	ystart;             // image y origin
	U16	width;              // image width in pixels
	U16	height;             // image height in pixels
	U8	bits_per_pixel;		// image bits per pixel 8,16,24,32
	U8	descriptor;			// image descriptor bits (vh flip bits)

	TGA_Header() : identsize(0), palette(0), imagetype(0), color_start(0), num_palette_colors(0), bits_per_color(0), 
		           xstart(0), ystart(0), width(0), height(0), bits_per_pixel(0), descriptor(0)
	{
	}

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

	int is_x_flipped (void)
	{
		return (descriptor & 0x10);
	}

	int is_y_flipped (void)
	{
		return !(descriptor & 0x20);
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


#ifndef SGI
#pragma pack(pop) //#pragma pack(pop, 1)
#endif
*/

//void ReadPix(char *file_name, Image *image);
//void InitImage(Image *img);
//void FreeImage(Image *img);
//void FlipImage(Image *img);
//void FlipY(void **array, int type_size, int width, int height);
//void FlipTxt(txt *t);

//void LoadTxtRGB(VRGB *rgb, txt *t);
//void GetDitherWeight(float weights[4], int x, int y, int width, int height, bool backward);
//void LoadTxtAlpha(unsigned char *alpha, txt *t);
//void calcMipLevels(VRGB *rgb, txt *t);
//void calcAlphaMipLevels(unsigned char *alpha, txt *t);
//void GetPixelError(const float *b1, const float *b2, const int size, float *max_err, float *avg_err);

file_node* CreateMtlLib(mtl_lib *ml);
void AddMtl(file_node *pt, mtl_lib *ml, int index);
//file_node* CreateMtlProperty(mtl_property *mp);
//file_node* CreateTxtLib(txt_lib *tl, anim_txt_lib *atl, file_node *node); // atl and node can be NULL
//file_node* CreateMip(mip *mp, int level_id);
//file_node* AddTxt(txt *t);
//file_node* CreateAnimTxtLib(anim_txt_lib *atl);
//file_node* AddAnimTxt(anim_txt *at);
//void ConvertTextures(txt_lib *tl, file_node *lib_out, BOOL32 xflip, BOOL32 yflip);

void InitMtlLib(mtl_lib *ml);
//void InitTxtLib(txt_lib *tl);
//void InitAnimTxtLib(anim_txt_lib *atl, txt_lib *tl);
//void InitAnimTxt(anim_txt *at, txt_lib *tl);

//void SetMtlType(mtl *m);
//void InitMaterial(mtl *m, anim_txt_lib *atl);
void InitMaterialCQ2(cq2Mtl *m);//, anim_txt_lib *atl);
//void InitMaterialProperty(mtl_property *mp, const float v, mtl_prop_type type, anim_txt_lib *atl);
//void InitTexture(txt *t);
//void InitMip(mip *mp);

//void FreeMtlLib(mtl_lib *ml);
//void FreeMtlLib(void *ml);
void FreeCQ2Mtl(cq2Mtl *m);
//void FreeMtl(mtl *m);
//void FreeMtlProperty(mtl_property *mp);
//void FreeTxt(txt *t);
//void FreeAnimTxt(anim_txt *at);
//void FreeMip(mip *mp);
//void FreeTxtLib(txt_lib *tl);
//void FreeAnimTxtLib(anim_txt_lib *atl);
//void CheckMtlPropOffset(const mtl_property & mp1, const mtl_property & mp2, const char *mtl_name);
//int InsertMaterial(mtl_lib *ml, mtl *m);
int InsertCq2Material(mtl_lib *ml, cq2Mtl *m);
//int InsertTexture(txt_lib *tl, txt *t);

//int GetTxtID(const txt_lib * const tl, const char * const name);
//int GetAnimTxtID(const anim_txt_lib * const atl, const char * const name);

int GetCQ2MtlID(const mtl_lib *ml, const char *name);
int GetCQ2MtlID(const mtl_lib *ml, const int api_id);

//mtl* GetMtlIDbyTXT(const mtl_lib *ml, const char *txt_name);


void LoadMaterials(mtl_lib * /*ml*/, file_node * /*root*/);
//void LoadTextures(txt_lib *tl, file_node *root);
//void LoadMiplevel(mip *mp, file_node *node);

//void MoveTexturesHome(char *infile, char *outfile);
//void TxmLibto3DB(char *file_name, txt_lib *tl);
//void MergeTextures(file_node *root_in, file_node *root_out);
//void InsertTxt(file_node *lib_node, file_node *texture);
//void DetachTxt(file_node *top, char *name);
//void RemoveTextures(file_node *root);
//void PrintMtl(mtl *m, int id);

bool SameMtl(const cq2Mtl * const m1, const cq2Mtl * const m2);
//bool SameMtlProp(const mtl_property & mp1, const mtl_property & mp2);

#endif
