//
// BigImage.cpp - Source for the standard IBigImage implementations
//

//
// Design Notes:
//      The BigImage class will take an arbitrary sized bitmap and break it into textures of a size the video card
// can handle. Afterward, arbitrary rectangles on this image can be drawn onto rectangles and 4 sided polygons.
//      When rendering, the source rectangle on the image must be mapped to the world space destination polygons, breaking
// the destination up on the boundaries of the textures. Thus, each rectangular portion of a texture that intersects
// the source rectangle will generate two triangles in the output stream. They are stored in row major order.

//
// Include files
//

#include <dacom.h>
#include <rendpipeline.h>
#include <iimagesource.h>
#include "bigimage.h"
#include <tsmartpointer.h>
#include <fdump.h>

//compiler will not shut up about this... not sure why
#pragma warning(disable: 4710)	//not inlined
#include <tempstr.h>
#include <tcomponent.h>
#pragma warning(default: 4710)	//not inlined

//
// Local functions
//

static	int	NextPowerOfTwo(int num)
{
	int	p2	=1;

	if(num & (num - 1))
	{
		while(num)
		{
			num	>>=1;
			p2	<<=1;
		}
	}
	else
	{
		while(num)
		{
			num	>>=1;
			p2	<<=1;
		}
		p2	>>=1;
	}
	return	p2;
}


//returns false if the rect is empty
static	BOOL	RECT_Normalize(RECT *r)
{
	int	i;

	ASSERT(r);

	if(!(r->right - r->left) || !(r->bottom - r->top))
	{
		return	FALSE;
	}
	if((r->right - r->left) < 0)
	{
		i			=r->left;
		r->left		=r->right;
		r->right	=i;
	}
	if((r->bottom - r->top) <= 0)
	{
		i			=r->top;
		r->top		=r->bottom;
		r->bottom	=i;
	}

	return	TRUE;
}

static	float	FLT_255	=255.0f;
static	float	Magic	=12582912.0f;

//pardon the asm, mind was wandering... just packs RGBA from x, y, z
#pragma warning(disable: 4035)	//return in eax 
static	DWORD	VectorToRGB(Vector *v)
{
	float	t1, t2;
	_asm
	{
		mov		eax,v

		fld		dword ptr[eax]Vector.x	;x
		fmul	dword ptr[FLT_255]		;X

		push	ebx

		fld		dword ptr[eax]Vector.y	;y    X
		fmul	dword ptr[FLT_255]		;Y    X
		fxch	st(1)					;X    Y
		fadd	dword ptr[Magic]		;Xi   Y
		fld		dword ptr[eax]Vector.z	;z    Xi    Y
		fmul	dword ptr[FLT_255]		;Z    Xi    Y
		fxch	st(2)					;Y    Xi    Z
		fadd	dword ptr[Magic]		;Yi   Xi    Z
		fxch    st(2)					;Z    Xi    Yi
		fadd	dword ptr[Magic]		;Zi   Xi    Yi
		fxch    st(2)				    ;Yi   Xi    Zi
		fstp    dword ptr[t1]			;Xi   Zi
		fstp    dword ptr[t2]			;Zi

		mov		eax, dword ptr[t1]
		mov		edx, dword ptr[t2]

		fstp	dword ptr[t1]

		sub		edx,dword ptr[Magic]
		mov		ebx,dword ptr[t1]

		sub		eax,dword ptr[Magic]

		shl		edx,16
		sub		ebx,dword ptr[Magic]

		shl		eax,8
		or		edx,ebx

		pop		ebx
		or		eax,edx
	}
}
#pragma warning(default: 4035)	//return in eax 

//
// Class and structure definitions
//

class BigImage : public IBigImage
{
protected:
	BEGIN_DACOM_MAP_INBOUND(BigImage)
	DACOM_INTERFACE_ENTRY(IBigImage)
	DACOM_INTERFACE_ENTRY2(IID_IBigImage, IBigImage)
	END_DACOM_MAP()

	COMPTR<IRenderPipeline>	PIPE;		         // pipe to use
	int						NumTextures;         // total number of textures created. NumX * NumY
	int						NumX;                // number of texture columns in image
	int						NumY;                // number of texture row in image.
	int						Width, Height;	     // of total large image, in pixels
	U32						*THandles;           // texture handles. THandles[NumTextures]
	RPVertex                *vlist;              // list of vertices. vlist[NumTextures * 4], to handle full source rect
	U32						Color;               // output vertex color
	U8						Alpha;               // output vertex alpha
	U32						bSquareOnly;         // renderpipe requires square textures
	U32						MaxWidth, MaxHeight; // renderpipe max texture dimensions

public:
	BigImage(void)
	{
		THandles	=NULL;
		vlist		=NULL;
		Color		=0xFFFFFF;
		Alpha		=0xFF;
	}
	
//compiler will not shut up about this... not sure why
#pragma warning(disable: 4710)	//not inlined
	virtual ~BigImage(void)
	{
		if(THandles)
		{
			// Destroy the textures before freeing the buffer containing their handles. -TNB
			for (int i = 0; i < NumTextures; ++i)
			{
				if (THandles[i] != HTX_INVALID)
				{
					// Don't care about the return value here.
					PIPE->destroy_texture (THandles[i]);
				}
			}

			free(THandles);
		}
		if(vlist)
		{
			free(vlist);
		}
	}
#pragma warning(default: 4710)	//not inlined

	// Methods required for DAAggregateComponent and DAComponent templates
	// This method is called by the factory template on the object after the descriptor checks out to be
	// of the right type. -TNB
	GENRESULT	init(BIGIMAGEDESC * desc)
	{
		// Save off the render pipeline and create the textures.
		PIPE	=desc->PIPE;
		PIPE->query_device_ability(RP_A_TEXTURE_SQUARE_ONLY, &bSquareOnly);
		PIPE->query_device_ability(RP_A_TEXTURE_MAX_WIDTH, &MaxWidth);
		PIPE->query_device_ability(RP_A_TEXTURE_MAX_HEIGHT, &MaxHeight);

		if(desc->SrcImage)
		{
			if(CreateTextures(desc->SrcImage, desc->DstFmt))
			{
				return	GR_OK;
			}
		}
		else
		{
			if(CreateTextures(desc->DstFmt, desc->pBits, desc->SrcFmt, desc->Width, desc->Height, desc->Stride))
			{
				return	GR_OK;
			}
		}

		// Something bad happened, so return a generic error.
		// This will cause the template to delete this object and return an error to the code that
		// tried to create one of these things.
		return	GR_GENERIC;
	}

	// ====== IBigImage methods ======
	virtual void __stdcall RenderSquare(RECT *src, Vector *pnts);
	virtual void __stdcall RenderRects(RECT *src, RECT *dst);
	virtual void __stdcall GetDimensions(U32 & width, U32 & height) const
	{
		width	=Width;
		height	=Height;
	}
	virtual void __stdcall SetAlpha(float a)
	{
		Alpha	=a * 255.0f;
	}
	virtual void __stdcall SetColor(Vector *color)
	{
		ASSERT(color);

		Color	=VectorToRGB(color);
	}

private:
	BOOL	CreateTextures(IImageSource *isrc, PixelFormat *dstpf);
	BOOL	CreateTextures(PixelFormat *dstpf, U8 *pBits, PixelFormat *pf, int w, int h, int stride);
	void	CalcUVExtents(const RECT &srcrect);
};

BOOL	BigImage::CreateTextures(IImageSource *isrc, PixelFormat *dstpf)
{
	PixelFormat		pf;
	U32				w, h, dbpp, sbpp;
	U8				*pBits;
	BOOL			ret;

	ASSERT(isrc);
	ASSERT(dstpf);

	if(isrc->GetPreferredFormat(&pf) != GR_OK)
	{
		return	FALSE;
	}
	if(isrc->GetDimensions(w, h) != GR_OK)
	{
		return	FALSE;
	}

	dbpp	=((dstpf->ddpf.dwRGBBitCount+7)>>3);
	sbpp	=((pf.ddpf.dwRGBBitCount+7)>>3);
	pBits	=(U8 *)malloc(w * h * dbpp);

	//try to create in dest pixel format, if fail use preferred
	if(isrc->GetImage(*dstpf, (void *)pBits, w * dbpp, NULL) != GR_OK)
	{
		free(pBits);
		pBits	=(U8 *)malloc(w * h * sbpp);

		if(isrc->GetImage(pf, (void *)pBits, w * sbpp, NULL) != GR_OK)
		{
			free(pBits);
			return	FALSE;
		}
		ret	=CreateTextures(dstpf, pBits, &pf, w, h, w * sbpp);
	}
	
	ret	=CreateTextures(dstpf, pBits, dstpf, w, h, w * dbpp);

	if(pBits)
	{
		free(pBits);
	}

	return	ret;
}

//creation for raw bits instead of iimagesource
BOOL	BigImage::CreateTextures(PixelFormat *dstpf, U8 *pBits, PixelFormat *pf, int w, int h, int stride)
{
	int				BitOfs, wt, ht, i, j, bd;
	int				np2h, cur;

	ASSERT(dstpf);
	ASSERT(pBits);
	ASSERT(pf);
	ASSERT(w > 0);
	ASSERT(h > 0);

	NumTextures	=(w/MaxWidth + ((w % MaxWidth)? 1 : 0)) * (h/MaxHeight + ((h % MaxHeight)? 1 : 0));
	Width		=w;
	Height		=h;
	wt			=w / MaxWidth;
	ht			=h / MaxHeight;
	bd			=((pf->ddpf.dwRGBBitCount+7)>>3);
	vlist		=(RPVertex *)malloc(sizeof(RPVertex) * NumTextures * 4);

	if(NumTextures)
	{
		THandles	=(U32 *)malloc(sizeof(U32) * NumTextures);

		for (i = 0; i < NumTextures; ++i)
		{
			THandles[i] = HTX_INVALID;
		}
		
		for(j=cur=0;j < ht;j++)
		{
			for(i=0;i < wt;i++)	//full size block here
			{
				BitOfs	=(i*MaxWidth*bd)+(j*MaxHeight*stride);

				if(PIPE->create_texture(MaxWidth, MaxHeight, *dstpf, 0, 0, THandles[cur]) != GR_OK)
				{
					GENERAL_TRACE_1(TEMPSTR("Failed to allocate %d by %d, %d bpp texture.", w, h, 16));
					return	FALSE;
				}

				if(PIPE->set_texture_level_data(THandles[cur], 0, MaxWidth, MaxHeight, stride, *pf, pBits+BitOfs, NULL, NULL) != GR_OK)
				{
					PIPE->destroy_texture(THandles[cur]);
					THandles[cur]	=HTX_INVALID;
					return	FALSE;
				}
				cur++;
			}			
			if(w % MaxWidth)	//make the leftover
			{
				BitOfs	=(i*MaxWidth*bd)+(j*MaxHeight*stride);

				if(bSquareOnly)
				{
					if(PIPE->create_texture(MaxWidth, MaxHeight, *dstpf, 0, 0, THandles[cur]) != GR_OK)
					{
						GENERAL_TRACE_1(TEMPSTR("Failed to allocate %d by %d, %d bpp texture.", w, h, 16));
						return	FALSE;
					}
				}
				else
				{
					if(PIPE->create_texture(NextPowerOfTwo(w % MaxWidth), MaxHeight, *dstpf, 0, 0, THandles[cur]) != GR_OK)
					{
						GENERAL_TRACE_1(TEMPSTR("Failed to allocate %d by %d, %d bpp texture.", w, h, 16));
						return	FALSE;
					}
				}

				if(PIPE->set_texture_level_data(THandles[cur], 0, w % MaxWidth, MaxHeight, stride, *pf, pBits+BitOfs, NULL, NULL) != GR_OK)
				{
					PIPE->destroy_texture(THandles[cur]);
					THandles[cur]	=HTX_INVALID;
					return	FALSE;
				}
				cur++;
			}
		}
		
		if(h % MaxHeight)	//make the leftover w full, h pad
		{
			np2h	=NextPowerOfTwo(h % MaxHeight);

			for(i=0;i < wt;i++)
			{
				BitOfs	=(i*MaxWidth*bd)+(j*MaxHeight*stride);

				if(bSquareOnly)
				{
					if(PIPE->create_texture(MaxWidth, MaxHeight, *dstpf, 0, 0, THandles[cur]) != GR_OK)
					{
						GENERAL_TRACE_1(TEMPSTR("Failed to allocate %d by %d, %d bpp texture.", w, h, 16));
						return	FALSE;
					}
				}
				else
				{
					if(PIPE->create_texture(MaxWidth, np2h, *dstpf, 0, 0, THandles[cur]) != GR_OK)
					{
						GENERAL_TRACE_1(TEMPSTR("Failed to allocate %d by %d, %d bpp texture.", w, h, 16));
						return	FALSE;
					}
				}

				if(PIPE->set_texture_level_data(THandles[cur], 0, MaxWidth, h % MaxHeight, stride, *pf, pBits+BitOfs, NULL, NULL) != GR_OK)
				{
					PIPE->destroy_texture(THandles[cur]);
					THandles[cur]	=HTX_INVALID;
					return	FALSE;
				}
				cur++;
			}			

			if(w % MaxWidth)	//make the leftover w pad, h pad
			{
				BitOfs	=(i*MaxWidth*bd)+(j*MaxHeight*stride);

				if(bSquareOnly)
				{
					U32	max;

					if(NextPowerOfTwo(w % MaxWidth) > np2h)
					{
						max	=NextPowerOfTwo(w % MaxWidth);
					}
					else
					{
						max	=np2h;
					}
					if(PIPE->create_texture(max, max, *dstpf, 0, 0, THandles[cur]) != GR_OK)
					{
						GENERAL_TRACE_1(TEMPSTR("Failed to allocate %d by %d, %d bpp texture.", w, h, 16));
						return	FALSE;
					}
				}
				else
				{
					if(PIPE->create_texture(NextPowerOfTwo(w % MaxWidth), np2h, *dstpf, 0, 0, THandles[cur]) != GR_OK)
					{
						GENERAL_TRACE_1(TEMPSTR("Failed to allocate %d by %d, %d bpp texture.", w, h, 16));
						return	FALSE;
					}
				}
				if(PIPE->set_texture_level_data(THandles[cur], 0, w % MaxWidth, h % MaxHeight, stride, *pf, pBits+BitOfs, NULL, NULL) != GR_OK)
				{
					PIPE->destroy_texture(THandles[cur]);
					THandles[cur]	=HTX_INVALID;
					return	FALSE;
				}
				cur++;
			}
		}
		NumX	=i + ((w % MaxWidth)? 1 : 0);
		NumY	=j + ((h % MaxHeight)? 1 : 0);
	}

	return	TRUE;
}

//don't have time to deal with finding why class func doesn't work
static	void	CrossProduct(const Vector & v1, const Vector & v2, Vector *v3)
{
	ASSERT(v3);

	v3->x	=v1.y * v2.z - v1.z * v2.y;
	v3->y	=v1.z * v2.x - v1.x * v2.z;
	v3->z	=v1.x * v2.y - v1.y * v2.x;
}

static	float	DotProduct(const Vector & v1, const Vector & v2)
{
	return	v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

void	BigImage::CalcUVExtents(const RECT	&srcrect)
{
	// Set the uv coordinates of all the vertices in the vertex list for this image.

	// First, find

	U32		i, j, np2w, np2h;
	float	leftu, rightu, topv, bottomv;

	// For each column of textures...
	for(i=0;i < (U32)NumX;i++)
	{
		// If the left side of the source rectangle is on or to the left of this column AND
		// the right side of the source rectangle is on or to the right of this column, i.e. if the
		// source rectangle intersects this column...
		if((srcrect.left / MaxWidth) <=i && (srcrect.right / MaxWidth) >=i)
		{
			// Calculate the width of this column of textures.
			// If this column is not the last, it must be the maximum size...
			if(i < (U32)NumX-1)	//middles are always MaxWidth
			{
				np2w	=MaxWidth;
			}
			// else we are on the last column. If the total image width is not an even multiple of the max
			// texture width, the last column's size is scaled to the next power of two.
			else if(Width % MaxWidth)
			{
				// NOTE: There is no need to bump this up to the next power of two. The render pipeline stretchs
				// the texture data to fit the texture, so what we need here is simply the remainder.
				np2w	=(Width % MaxWidth);
			}
			// else the last column is the maximum width.
			else
			{
				np2w	=MaxWidth;
			}

			// If the left side of the source rectangle lies in this column, the u value for the left side of
			// the current square will be set according to where the source rectangle lies in this texture.
			// Otherwise, it will be set to the left side of the texture.
			if((srcrect.left / MaxWidth) == i)
			{
				leftu	=(float)(srcrect.left % MaxWidth) / (float)np2w;
			}
			else
			{
				leftu	=0.0f;
			}

			// If the right side of the source rectangle lies in this row, the u value for the right side of the
			// current square will be set according to where the source rectangle lies in this texture.
			// Otherwise, it will be set to the right side of the texture.
			if((srcrect.right / MaxWidth) == i)
			{
				rightu	=(float)(srcrect.right % MaxWidth) / (float)np2w;
			}
			else
			{
				rightu	=1.0f;
			}

			// For each fow in this column...
			for(j=0;j < (U32)NumY;j++)
			{
				// The 4 vertices for this portion are indexed in vlist[] in row major order.
				// Get the index to the first vertex
				int	vidx	=4*(i+j*NumX);

				// If the top of the source rectangle is on or above this row AND
				// the bottom of the source rectangle is on or below this row, i.e. if the
				// source rectangle intersects this row...
				if((srcrect.top / MaxHeight) <=j && (srcrect.bottom / MaxHeight) >=j)
				{
					// This row and column intersect the source rectangle, so we must generate proper
					// UV coordinates for them.

					// Calculate the height of this row of textures.
					// If this row is not the last, it must be the maximum size...
					if(j < (U32)NumY-1)	//middles are always MaxHeight
					{
						np2h	=MaxHeight;
					}
					// else we are on the last row. If the total image height is not an even multiple of the max
					// texture height, the last row's size is scaled to the next power of two.
					else if(Height % MaxHeight)
					{
						// NOTE: There is no need to bump this up to the next power of two. The render pipeline stretchs
						// the texture data to fit the texture, so what we need here is simply the remainder.
						np2h	=(Height % MaxHeight);
					}
					// else the last row is the maximum height.
					else
					{
						np2h	=MaxHeight;
					}

					// If the top side of the source rectangle lies in this row, the v value for the top side of the
					// current square will be set according to where the source rectangle lies in this texture.
					// Otherwise, it will be set to the top side of the texture.
					if((srcrect.top / MaxHeight) == j)
					{
						topv	=(float)(srcrect.top % MaxHeight) / (float)np2h;
					}
					else
					{
						topv	=0.0f;
					}

					// If the bottom side of the source rectangle lies in this row, the v value for the bottom side of the
					// current square will be set according to where the source rectangle lies in this texture.
					// Otherwise, it will be set to the bottom side of the texture.
					if((srcrect.bottom / MaxHeight) == j)
					{
						bottomv	=(float)(srcrect.bottom % MaxHeight) / (float)np2h;
					}
					else
					{
						bottomv	=1.0f;
					}

					// Store the u and v values into the 4 vertices for this square.
					vlist[vidx + 0].u	=leftu;
					vlist[vidx + 1].u	=rightu;
					vlist[vidx + 2].u	=leftu;
					vlist[vidx + 3].u	=rightu;
					vlist[vidx + 0].v	=topv;
					vlist[vidx + 1].v	=topv;
					vlist[vidx + 2].v	=bottomv;
					vlist[vidx + 3].v	=bottomv;
				}
				// The source rectangle does not intersect this row. Set the v coordinates for this row to zero
				// and the u coordinates for this column to their correct values.
				// column to 0.
				else
				{
					vlist[vidx + 0].u	=leftu;
					vlist[vidx + 1].u	=rightu;
					vlist[vidx + 2].u	=leftu;
					vlist[vidx + 3].u	=rightu;
					for(int k=0;k < 4;k++)
					{
						vlist[vidx + k].v	=0.0f;
					}
				}
			}
		}
		// The source rectangle does not intersect this column. Set the uv coordinates for all the
		// squares in this column to 0.
		else
		{
			for(j=0;j < (U32)NumY;j++)
			{
				for(int k=0;k < 4;k++)
				{
					vlist[4*(i+j*NumX) + k].u	=0.0f;
					vlist[4*(i+j*NumX) + k].v	=0.0f;
				}
			}
		}
	}
}

//top left is the first vertex used in xy space, winding order goes +x then +y
void	__stdcall	BigImage::RenderSquare(RECT *src, Vector *pnts)
{
	int		i, j;
	static	U16	vrt_index[6]	={ 0, 1, 2, 1, 3, 2 };
	U32		UFlag, VFlag;
	RECT	srcrect, dstrct;
	Vector	xaxis, yaxis, zaxis, LastPos, Ofs, deltpos, colPos;
	float	minx, miny, maxx, maxy, dot;

	ASSERT(pnts);

	// Assuming that the 4 points are co-planar, we calcuate two basis vectors for the surface of the
	// image in world space. These basis vectors will be used to map the source rectangle into world space.
	//get axis vectors
	xaxis	=pnts[1] - pnts[0];
	yaxis	=pnts[2] - pnts[0];

	xaxis.normalize();
	yaxis.normalize();

	CrossProduct(yaxis, xaxis, &zaxis);
	zaxis.normalize();

	// Here we find the bounding rectangle, in our planar space, of the destination polygon.
	// This rectangle is what the source rectangle maps onto.
	// *** This seems wrong. Can't we just map the corners and linearly interpolate?
	//get extents of xy space
	minx	=miny	=696969.0f;
	maxx	=maxy	=-696969.0f;

	for(i=0;i < 4;i++)
	{
		dot	=DotProduct(xaxis, pnts[i]);
		if(dot < minx)
		{
			minx	=dot;
		}
		if(dot > maxx)
		{
			maxx	=dot;
		}

		dot	=DotProduct(yaxis, pnts[i]);
		if(dot < miny)
		{
			miny	=dot;
		}
		if(dot > maxy)
		{
			maxy	=dot;
		}
	}
	dstrct.top		=dstrct.left	=0;
	dstrct.right	=maxx;
	dstrct.bottom	=maxy;
	
	// Save the texture addressing modes and set them to clamp.
	PIPE->get_texture_stage_state( 0, D3DTSS_ADDRESSU, &UFlag);
	PIPE->get_texture_stage_state( 0, D3DTSS_ADDRESSV, &VFlag);
	PIPE->set_texture_stage_state( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
	PIPE->set_texture_stage_state( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );

	// If we were not given a source rectangle, use the entire image.
	if(src)
	{
		srcrect	=*src;

		if(!RECT_Normalize(&srcrect))
		{
			srcrect.top		=0;
			srcrect.left	=0;
			srcrect.right	=Width-1;
			srcrect.bottom	=Height-1;
		}
	}
	else
	{
		srcrect.top		=0;
		srcrect.left	=0;
		srcrect.right	=Width-1;
		srcrect.bottom	=Height-1;
	}

	// Fill out the UV coordinates for each of the tiles. Tiles that do not intersect the source rectangle
	// are set to (0,0).
	CalcUVExtents(srcrect);

	float Xscale = (float)(maxx-minx)/(float)(srcrect.right - srcrect.left);
	float Yscale = (float)(maxy-miny)/(float)(srcrect.bottom - srcrect.top);

	// Start with the first point.
	colPos = LastPos = pnts[0];

	// For each column of tiles...
	for(i=0;i < NumX;i++)
	{
		// Calculate the U width of this column by calculating the U width of the first tile in this column.
		// Calculate the corresponding planar space width via ratios.
		float	columnScale	=vlist[4*i + 1].u - vlist[4*i].u;
		float	tileWidth;

		// If this column is the last and the image width is not an even multiple of the maximum tile width, i.e.
		// if the last column was allocated at something other than the max tile width, then adjust the
		// planar space width accordingly. 

		if(i == (NumX -1) && (Width % MaxWidth))
		{
			tileWidth = (float)(Width % MaxWidth) * Xscale;
		}
		else
		{
			tileWidth = MaxWidth * Xscale;
		}
		tileWidth *= columnScale;

		// Set the current position to the start of this column
		LastPos = colPos;

		// For each row of tiles...
		for(j=0;j < NumY;j++)
		{
			// Calculate the index of the vertices for this tile.
			// Calculate the V height of this row by calculating the V height of the first tile in this row.
			// Calculate the corresponding planar space height via ratios.
			int		vidx	    =4*(i+j*NumX);
			float	rowScale	=vlist[vidx + 2].v - vlist[vidx].v;
			float	tileHeight;

			// If this row is the last and the image height is not an even multiple of the maximum tile height, i.e.
			// if the last row was allocated at something other than the max tile height, then adjust the
			// planar space height accordingly. 

			if(j == (NumY -1) && (Height % MaxHeight))
			{
				tileHeight = (float)(Height % MaxHeight) * Yscale;
			}
			else
			{
				tileHeight = MaxHeight * Yscale;
			}
			tileHeight *= rowScale;

			// If the U width is not zero, i.e. if this tile is valid...
			if(columnScale != 0.0f)	//project verts along the axis's
			{
				// Transform the 4 corners of this tile in planar space to world space via the basis
				// vectors we calculated above.

				deltpos			=LastPos;
				vlist[vidx + 0].pos	=deltpos;

				Ofs.set(tileWidth, 0.0f, 0.0f);

				deltpos.x	+=DotProduct(xaxis, Ofs);
				deltpos.y	+=DotProduct(yaxis, Ofs);
				deltpos.z	+=DotProduct(zaxis, Ofs);

				vlist[vidx + 1].pos	=deltpos;
				deltpos				=LastPos;

				Ofs.set(0.0f, tileHeight, 0.0f);

				deltpos.x	+=DotProduct(xaxis, Ofs);
				deltpos.y	+=DotProduct(yaxis, Ofs);
				deltpos.z	+=DotProduct(zaxis, Ofs);

				vlist[vidx + 2].pos	=deltpos;
				deltpos				=LastPos;

				Ofs.set(tileWidth, tileHeight, 0.0f);

				deltpos.x	+=DotProduct(xaxis, Ofs);
				deltpos.y	+=DotProduct(yaxis, Ofs);
				deltpos.z	+=DotProduct(zaxis, Ofs);

				vlist[vidx + 3].pos	=deltpos;

				// Fill in the color and alpha values
				for(int k=0;k < 4;k++)
				{
					vlist[vidx + k].color	=Color;
					vlist[vidx + k].a		=Alpha;
				}

				// Draw the two triangles for this tile
				PIPE->set_texture_stage_texture(0, THandles[(i+j*NumX)]);
				PIPE->draw_indexed_primitive(D3DPT_TRIANGLELIST, (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1), vlist+(vidx), 4, vrt_index, 6, 0);

				// Move the current world position to the next row
				Ofs.set(0.0f, tileHeight, 0.0f);

				LastPos.x	+=DotProduct(xaxis, Ofs);
				LastPos.y	+=DotProduct(yaxis, Ofs);
				LastPos.z	+=DotProduct(zaxis, Ofs);
			}
		}

		// Move the column position to the next column.
		Ofs.set(tileWidth, 0.0f, 0.0f);

		colPos.x	+=DotProduct(xaxis, Ofs);
		colPos.y	+=DotProduct(yaxis, Ofs);
		colPos.z	+=DotProduct(zaxis, Ofs);
	}

	// Restore the clamping state.
	PIPE->set_texture_stage_state( 0, D3DTSS_ADDRESSU, UFlag);
	PIPE->set_texture_stage_state( 0, D3DTSS_ADDRESSV, VFlag);
}


void	__stdcall	BigImage::RenderRects(RECT *src, RECT *dst)
{
	int			i, j;
	float		LastX, LastY;
	U32			UFlag, VFlag;
	RECT		srcrect;
	static	U16	vrt_index[6]	={ 0, 1, 2, 1, 3, 2 };
	RPVertex	OutVerts[4];

	ASSERT(dst);

	PIPE->get_texture_stage_state( 0, D3DTSS_ADDRESSU, &UFlag);
	PIPE->get_texture_stage_state( 0, D3DTSS_ADDRESSV, &VFlag);
	PIPE->set_texture_stage_state( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
	PIPE->set_texture_stage_state( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );

	// If a source rectangle is not provided, use the full source image rectangle.
	if(src)
	{
		srcrect	=*src;

		if(!RECT_Normalize(&srcrect))
		{
			srcrect.top		=0;
			srcrect.left	=0;
			srcrect.right	=Width-1;
			srcrect.bottom	=Height-1;
		}
	}
	else
	{
		srcrect.top		=0;
		srcrect.left	=0;
		srcrect.right	=Width-1;
		srcrect.bottom	=Height-1;
	}

	// Fill out the UV coordinates for each of the tiles. Tiles that do not intersect the source rectangle
	// are set to (0,0).
	CalcUVExtents(srcrect);

	// The easiest way to think about this is as a series of coordinate changes.
	// The coordinate systems in use are:
	// 1) Source image coordinates: 0-Width, 0-Height
	// 2) Tile coordinates: 0-NumX, 0-NumY
	// 3) Destination Coordinates: dst->left - dst->right, dst->top - dst->bottom
	// 4) Source rectangle coordinates: srcrect.left - srcrect.right, srcrect.top - srcrect.bottom
	// The scales from source image coordinates to destination coordinates is:
	//    Xscale => (dst->right - dst->left)/(srcrect.right - srcrect.left);
	//    Yscale => (dst->bottom - dst->top)/(srcrect.bottom - srcrect.top);
	// Given a coordinate in source image space, the destination coordinate is:
	//    Xdest = (Xsrc - srcrect.left) * Xscale + dst->left;
	//    Ydest = (Ysrc - srcrect.top) * Yscale + dst->top;
	// The size of each drawn tile in source image space can be had by taking each drawn tile's U or V span
	// and using it as a scale for the source tile in destination space, which is simply Xscale or Yscale times
	// MaxWidth or (Width%MaxWidth), and MaxHeight or (Height%MaxHeight), respectively.

	LastX	=dst->left;
	
	float	Xscale	=(float)(dst->right - dst->left)/(float)(srcrect.right - srcrect.left);
	float	Yscale	=(float)(dst->bottom - dst->top)/(float)(srcrect.bottom - srcrect.top);

	for(i=0;i < NumX;i++)
	{
		// The U coordinates are normalized, so the U distance of this tile can be used as a general
		// ratio of the width of the intersection of the source rectangle with this source tile to the width
		// of this source tile. Using ratios from there, we can calculate the coorsponding with of this tile
		// in the destination rect.

		LastY	=dst->top;

		float	columnScale	=vlist[4*i+1].u - vlist[4*i].u;
		float	tileWidth;

		if(i == (NumX -1) && (Width % MaxWidth))
		{
			tileWidth = (float)(Width % MaxWidth) * Xscale;
		}
		else
		{
			tileWidth = MaxWidth * Xscale;
		}
		tileWidth *= columnScale;

		for(j=0;j < NumY;j++)
		{
			int		vidx	=4*(i+j*NumX);
			float	rowScale =    vlist[vidx+2].v - vlist[vidx].v;
			float   tileHeight;

			if(j == (NumY -1) && (Height % MaxHeight))
			{
				tileHeight = (float)(Height % MaxHeight) * Yscale;
			}
			else
			{
				tileHeight = MaxHeight * Yscale;
			}
			tileHeight *= rowScale;

			// If this tile intersected the source rectangle...
			if(columnScale != 0.0f)
			{
				// Fill out the 4 vertices using the width and height of calculated for this tile.
				vlist[vidx + 0].pos.set(LastX,			    LastY, 0.0f);
				vlist[vidx + 1].pos.set(LastX + tileWidth,	LastY, 0.0f);
				vlist[vidx + 2].pos.set(LastX,			    LastY + tileHeight, 0.0f);
				vlist[vidx + 3].pos.set(LastX + tileWidth,	LastY + tileHeight, 0.0f);

				// Fill in the color and alpha values.
				for(int k=0;k < 4;k++)
				{
					vlist[vidx + k].color	=Color;
					vlist[vidx + k].a		=Alpha;
					vlist[vidx + k].pos.z	=0.0f;

					OutVerts[k]				=vlist[vidx + k];
				}

				// Draw the triangles
				PIPE->set_texture_stage_texture(0, THandles[(i+j*NumX)]);
				PIPE->draw_indexed_primitive(D3DPT_TRIANGLELIST, (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1), OutVerts, 4, vrt_index, 6, 0);

				// Do the next row.
				LastY	+= tileHeight;
			}
		}

		// Do the next column
		LastX	+= tileWidth;
	}

	// Restore the texture addressing states.
	PIPE->set_texture_stage_state( 0, D3DTSS_ADDRESSU, UFlag);
	PIPE->set_texture_stage_state( 0, D3DTSS_ADDRESSV, VFlag);
}

void	bigimage_startup(void)
{
	ICOManager			*DACOM	=DACOM_Acquire();
	IComponentFactory	*server;
	
	server	=new DAComponentFactory<DAComponent<BigImage>,BIGIMAGEDESC>("IBigImage");
	
	DACOM->RegisterComponent(server, "IBigImage", DACOM_LOW_PRIORITY);
	
	server->Release();
}