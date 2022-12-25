//--------------------------------------------------------------------------//
//                                                                          //
//                               TManager.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/TManager.cpp 11    9/07/00 9:33a Jasony $

   Texture memory management
*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>

#include "TManager.h"
#include "Startup.h"
#include "CQTrace.h"
#include "IImageReader.h"
#include "THashList.h"

#include <FileSys.h>
#include <TSmartPointer.h>
#include <EventSys.h>
#include <IConnection.h>
#include <HeapObj.h>
#include <TComponent.h>
#include <WindowManager.h>
#include <HKEvent.h>

#include <commctrl.h>

//--------------------------------------------------------------------------//
//
struct TMNODE
{
	TMNODE * pNext;	
	//
	// Hash function derives 8-bit key from string by
	// adding all ASCII character values modulo 256
	//
	
	static inline U32 TMNODE::hash (const char *ptr)
	{
		U8 result = 0;
		char a=*ptr++;
		while (a)
		{
			result += a|0x20;		// convert to lower case
			a = *ptr++;
		}
		return result;
	}
	
	U32 hash (void)
	{
		return hash(name);
	}

	inline bool compare (const char *ptr)
	{
		return (strcmp(ptr, name) == 0);
	}

	inline bool compare (U32 value)
	{
		return (value == textureID);
	}

	inline void print (void)
	{
		CQTRACE12("Texture '%s' has %d dangling references!", name, refCount);
	}
	
	TMNODE (const char * ptr)
	{
		strncpy(name, ptr, sizeof(name));
		name[sizeof(name)-1] = 0;
		textureID = 0;
		refCount = 1;
		pNext = 0;
	}
	
	~TMNODE (void)
	{
		if (textureID && PIPE)
			PIPE->destroy_texture(textureID);
		textureID = 0;
		name[0] = 0;
		refCount = 0;
	}
	
	//
	// User data
	//
	U32 textureID;
	char name[32];
	U32 refCount;
};
//--------------------------------------------------------------------------//
//
struct DANODE
{
	DANODE * pNext;

	U32		textureID;
	U32		resolution;
	bool	bAlpha;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	~DANODE (void)
	{
		if (textureID && PIPE)
			PIPE->destroy_texture(textureID);
	}
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE TManager : public ITManager, IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(TManager)
	DACOM_INTERFACE_ENTRY(ITManager)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

 	//------------------------

	U32 eventHandle;
	HashList<TMNODE, const char *, 256> textureList;

	DANODE * pInUseList, * pJustFreedList, *pFreedList;

	TManager (void);

	~TManager (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ITManager methods */

	virtual U32 __stdcall CreateTextureFromMemory (void *pMemory, DA::FILETYPE type, const PixelFormat &format, const char *name);

	virtual U32 __stdcall CreateTextureFromFile (const char *fileName, IComponentFactory *parentFile, DA::FILETYPE type, const PixelFormat &format);

	virtual U32 __stdcall AddTextureRef (U32 textureID);

	virtual U32 __stdcall ReleaseTextureRef (U32 textureID);

	virtual void __stdcall Flush (void);

	virtual U32 __stdcall GetFirstTexture();

	virtual U32 __stdcall GetNextTexture(U32 textureID);

	virtual U32 __stdcall GetPrevTexture(U32 textureID);

	virtual U32 __stdcall CreateDrawAgentTexture (U32 resolution, bool bAlpha);

	virtual void __stdcall ReleaseDrawAgentTexture (U32 textureID);

	virtual void Initialize(InitInfo & info){};

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* TManager methods */

	static U32 __stdcall createTextureFromFile (const char *fileName, IComponentFactory *parentFile, DA::FILETYPE type, const PixelFormat &format);

	static U32 __stdcall createTextureFromMemory (void *pMemory, DA::FILETYPE type, const PixelFormat &format, const char *name);

	void reset (void);

	void addTexture (const char * name, U32 textureID);

	TMNODE * findTexture (U32 textureID);

	void moveDATextures (void);

	IDAComponent * getBase (void)
	{
		return static_cast<ITManager *>(this);
	}
};
//--------------------------------------------------------------------------//
//
TManager::TManager (void)
{
}
//--------------------------------------------------------------------------//
//
TManager::~TManager (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GS && GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);

	reset();
}
//------------------------------------------------------------------------
//
static U32 __fastcall nearestPower (S32 number)
{
	int i = 31;

	while (number > 0)
	{
		number <<= 1;
		i--;
	}

	if (i && (number==0 || (number << 1) != 0))
		i++;	// round up

	return (1 << i);
}
//------------------------------------------------------------------------
//
static inline void makeSquare (U32 & width, U32 & height)
{
	while (width < height)
		width *= 2;
	while (height < width)
		height *= 2;
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT TManager::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_UPDATE:
		moveDATextures();
		break;
	}
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
U32 TManager::CreateTextureFromMemory (void *pMemory, DA::FILETYPE type, const PixelFormat &format, const char *name)
{
	TMNODE * node = textureList.findHashedNode(name);

	if (node)
	{
		node->refCount++;
		return node->textureID;
	}
	
	U32 result = createTextureFromMemory(pMemory, type, format, name);
	if (result)
		addTexture(name, result);

	return result;
}
//--------------------------------------------------------------------------//
//
U32 TManager::CreateTextureFromFile (const char *fileName, IComponentFactory *parentFile, DA::FILETYPE type, const PixelFormat &format)
{
	TMNODE * node = textureList.findHashedNode(fileName);

	if (node && !strstr(fileName, "testTest"))
	{
		node->refCount++;
		return node->textureID;
	}

	U32 result = createTextureFromFile(fileName, parentFile, type, format);
	if (result)
		addTexture(fileName, result);

	return result;
}
//--------------------------------------------------------------------------//
//
U32 TManager::AddTextureRef (U32 textureID)
{
	TMNODE * node = findTexture(textureID);
	CQASSERT(node && "invalid texture id (ignorable)");

	if (node)
	{
		node->refCount++;
		return node->refCount;
	}
	else
		return 0;
}
//--------------------------------------------------------------------------//
//
U32 TManager::ReleaseTextureRef (U32 textureID)
{
	if (textureID == 0)
		return 0;
	else
	{
		TMNODE * node = findTexture(textureID);
		CQASSERT((node || CQFLAGS.b3DEnabled==0) && "invalid texture id (ignorable)");
	
		if (node)
		{
			CQASSERT(node->refCount);
			if (--node->refCount == 0)
			{
				textureList.removeNode(node);
				delete node;
				return 0;
			}

			return node->refCount;
		}
		else
			return 0;
	}
}
//--------------------------------------------------------------------------//
//
void TManager::Flush (void)
{
	reset();
}
//--------------------------------------------------------------------------//
//
U32 TManager::GetFirstTexture()
{
	TMNODE * node = textureList.getFirstNode();
	if(node)
		return node->textureID;
	return 0;
}
//--------------------------------------------------------------------------//
//
U32 TManager::GetNextTexture(U32 textureID)
{
	TMNODE * node = textureList.findNextNode(textureID);
	if(node)
		return node->textureID;
	return 0;
}
//--------------------------------------------------------------------------//
//
U32 TManager::GetPrevTexture(U32 textureID)
{
	TMNODE * node = textureList.findPrevNode(textureID);
	if(node)
		return node->textureID;
	return 0;
}
//--------------------------------------------------------------------------//
//
U32 TManager::CreateDrawAgentTexture (U32 resolution, bool bAlpha)
{
	U32 result=0;
	PixelFormat desiredFormat(16, 5, 6-bAlpha, 5, bAlpha);		// GL_RGB5_A1
	//DANODE * pInUseList, * pJustFreedList, *pFreedList;
	DANODE * pNode, *pPrev;
	//
	// see if there is a matching texture in the freed list
	//
	if ((pNode=pFreedList) != 0)
	{
		pPrev = 0;
		while (pNode)
		{
			if (pNode->resolution == resolution && pNode->bAlpha == bAlpha)
				break;
			pPrev = pNode;
			pNode = pNode->pNext;
		}
		if (pNode)	// found a match
		{
			if (pPrev)
				pPrev->pNext = pNode->pNext;
			else
				pFreedList = pNode->pNext;

			result = pNode->textureID;

			pNode->pNext = pInUseList;
			pInUseList = pNode;

			goto Done;
		}
	}
	//
	// else we need to create a new texture
	//
	if (PIPE->create_texture(resolution, resolution, desiredFormat, 1, 0, result) != GR_OK)
		CQBOMB0("Couldn't create texture");

	pNode = new DANODE;
	pNode->pNext = pInUseList;
	pInUseList = pNode;
	pNode->bAlpha = bAlpha;
	pNode->resolution = resolution;
	pNode->textureID = result;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
void TManager::ReleaseDrawAgentTexture (U32 textureID)
{
	DANODE * pNode, *pPrev;
	//
	// try to find the matching node
	//
	if ((pNode=pInUseList) != 0)
	{
		pPrev = 0;
		while (pNode)
		{
			if (pNode->textureID == textureID)
				break;

			pPrev = pNode;
			pNode = pNode->pNext;
		}
		if (pNode)
		{
			if (pPrev)
				pPrev->pNext = pNode->pNext;
			else
				pInUseList = pNode->pNext;

			pNode->pNext = pJustFreedList;
			pJustFreedList = pNode;
		}
	}
}
//--------------------------------------------------------------------------//
//
void TManager::moveDATextures (void)
{
	DANODE * pNode;

	if (pJustFreedList)
	{
		if (pFreedList == 0)
		{
			pFreedList = pJustFreedList;
			pJustFreedList = 0;
		}
		else
		{
			pNode = pJustFreedList;
			while (pNode->pNext)
				pNode = pNode->pNext;
			pNode->pNext = pFreedList;
			pFreedList = pJustFreedList;
			pJustFreedList = 0;
		}
	}
}




//--------------------------------------------------------------------------//
//

#define MAX_TEX_DIM 2048
static int importance[MAX_TEX_DIM*MAX_TEX_DIM];
static COLORREF texLoadBitsA[MAX_TEX_DIM*MAX_TEX_DIM];
static COLORREF texLoadBitsB[MAX_TEX_DIM*MAX_TEX_DIM];
static COLORREF texLoadBitsC[MAX_TEX_DIM*MAX_TEX_DIM];


void createMipmap(COLORREF * bits, COLORREF * newBits, int originalSize, bool bSharpMipmaps)
{
	int i;
	if (bSharpMipmaps)
	{
	for (i = 0; i < originalSize; i++)
		for (int j = 0; j < originalSize; j++)
		{
			int nj = j < originalSize-1 ? 1 : -j;
			int ni = i < originalSize-1 ? 1 : -i;
			int pj = j > 0 ? -1 : (originalSize-1);
			int pi = i > 0 ? -1 : (originalSize-1);

			COLORREF* thisPixel = &bits[j*originalSize + i];
			COLORREF* nearPixels[8];
			nearPixels[0] = &bits[(j+nj)*originalSize + (i+0)];
			nearPixels[1] = &bits[(j+pj)*originalSize + (i+0)];
			nearPixels[2] = &bits[(j+0)*originalSize + (i+ni)];
			nearPixels[3] = &bits[(j+0)*originalSize + (i+pi)];
			nearPixels[4] = &bits[(j+nj)*originalSize + (i+ni)];
			nearPixels[5] = &bits[(j+nj)*originalSize + (i+pi)];
			nearPixels[6] = &bits[(j+pj)*originalSize + (i+ni)];
			nearPixels[7] = &bits[(j+pj)*originalSize + (i+pi)];
			
			importance[j*originalSize + i] = 0;
			for (int k = 0; k < 8; k++)
			{
				for (int l = 0; l < 4; l++)
				{
					importance[j*originalSize + i] += abs(((U8*) thisPixel)[l] - ((U8*) nearPixels[k])[l]);
				}
			}
			if ((i == 0 || j == 0 || i == originalSize-1 || j == originalSize-1)	// transparent edge pixels  must be preserved (for clamping textures)
				&& ((U8*) thisPixel)[3] == 0)
			{
				importance[j*originalSize + i] += 99999;
			}
		}
	}


	for (i = 0; i < originalSize/2; i++)
		for (int j = 0; j < originalSize/2; j++)
		{
			int localImport[4];
			COLORREF * copyFrom[4];
			copyFrom[0] = &bits[2*j*originalSize + i*2];
			copyFrom[1] = &bits[2*j*originalSize + i*2+1];
			copyFrom[2] = &bits[(2*j+1)*originalSize + i*2];
			copyFrom[3] = &bits[(2*j+1)*originalSize + i*2+1];
			if (bSharpMipmaps)
			{
				localImport[0] = importance[2*j*originalSize + i*2];
				localImport[1] = importance[2*j*originalSize + i*2+1];
				localImport[2] = importance[(2*j+1)*originalSize + i*2];
				localImport[3] = importance[(2*j+1)*originalSize + i*2+1];
			}

			U32 r(0), g(0), b(0), a(0);
			int maxImport = 0;
			int mainPixel = 0;
			for (int k = 0; k < 4; k++)
			{
				if (maxImport <= localImport[k])
				{
					maxImport = localImport[k];
					mainPixel = k;
				}
				r += ((U8*) copyFrom[k])[0];
				g += ((U8*) copyFrom[k])[1];
				b += ((U8*) copyFrom[k])[2];
				a += ((U8*) copyFrom[k])[3];
			}
			if (bSharpMipmaps)
			{
				r += 7 * ((U8*) copyFrom[mainPixel])[0];
				g += 7 * ((U8*) copyFrom[mainPixel])[1];
				b += 7 * ((U8*) copyFrom[mainPixel])[2];
				a += 7 * ((U8*) copyFrom[mainPixel])[3];

				r /= 11;
				g /= 11;
				b /= 11;
				a /= 11;
			}
			else
			{
				r /= 4;
				g /= 4;
				b /= 4;
				a /= 4;
			}

			
			((U8*)&newBits[j*(originalSize/2) + i])[0] = (U8)r;
			((U8*)&newBits[j*(originalSize/2) + i])[1] = (U8)g;
			((U8*)&newBits[j*(originalSize/2) + i])[2] = (U8)b;
			((U8*)&newBits[j*(originalSize/2) + i])[3] = (U8)a;
			
			//newBits[j*(originalSize/2) + i] = *copyFrom[mainPixel];
		}
}




void convertBumpmap(COLORREF * src,COLORREF * dst, int originalSize)
{	// converts the pixels to a signed format
	for (int i = 0; i < originalSize; i++)
	{
		for (int j = 0; j < originalSize; j++)
		{
			U8* srcPixel =(U8*) &src[j*originalSize + i];
			U8* dstPixel =(U8*) &dst[j*originalSize + i];
			Vector tmp = Vector(srcPixel[0],srcPixel[1],0);
			tmp /= 255.0;
			tmp.x -= .5;
			tmp.y -= .5;
			tmp.z = .5;
			tmp.normalize();
			tmp.x += .5;
			tmp.y += .5;
			tmp *= 255;
			dstPixel[0]=tmp.x;
			dstPixel[1]=tmp.y;
			dstPixel[2]=tmp.z;
			dstPixel[3]=srcPixel[2];
		}
	}
}




U32 TManager::createTextureFromMemory (void *pMemory, DA::FILETYPE type, const PixelFormat &format, const char *name)
{
	U32 textureID=0;
	COLORREF *bits = texLoadBitsA;
	COMPTR<IImageReader> reader;
	U32 width, height;
	BOOL32 bForceRGB=(format.awidth != 0); //user wants alpha;
	RGB palette[256];
	RECT rect;

#ifdef _DEBUG
	if (CQFLAGS.b3DEnabled==0)
		CQTRACE10("Attempt to create a texture in 2D mode.");
#endif

	switch (type)
	{
	case DA::BMP:
		CreateBMPReader(reader);
		break;
	case DA::TGA:
		CreateTGAReader(reader);
		break;
	case DA::VFX:
		CreateVFXReader(reader);
		break;
	default:
		CQERROR0("Invalid image type!");
		goto Done;
	}

	reader->LoadImage(pMemory, 0, 0);
	width = reader->GetWidth();
	height = reader->GetHeight();

	{
		U32 nearWidth, nearHeight;

		nearWidth  = nearestPower(width);
		nearHeight = nearestPower(height);

		if (nearWidth != width || nearHeight != height || width != height)
		{
			CQERROR3("Attempt to create a %dx%d texture from \"%s\".\n\tTextures must be square and a power of two.", 
				width, height, (name?name:""));

			makeSquare(nearWidth, nearHeight);
			width = nearWidth;
			height = nearHeight;
		}
	}

	rect.top = rect.left = 0;
	rect.right = width - 1;
	rect.bottom = height - 1;

	{
	int numMipmapLevels = (int)(log((float)width)/log(2.0f) -1.0f);


	if (PIPE->create_texture(width,height,format,numMipmapLevels,0,textureID) != GR_OK)
		CQBOMB2("create_texture() failed. Requested = %dx%d", width, height);

	//
	// if image is paletted, use the index version
	//
	if (bForceRGB==0 && reader->GetColorTable(PF_RGB, palette) == GR_OK)
	{
		reader->GetImage(PF_COLOR_INDEX, bits, &rect);
		PIPE->set_texture_level_data(textureID, 0, width, height, width, 
			PixelFormat(PF_COLOR_INDEX),
			bits, NULL, palette);
	}
	else	// use RGBA values
	{
		reader->GetImage(PF_RGBA, bits, &rect);

		bool bump = (strstr(name, "_bump.tga")!= 0);
		COLORREF * bumpBits = texLoadBitsC;


		if (bump) convertBumpmap(bits,bumpBits, width);
		else bumpBits = bits;

		PIPE->set_texture_level_data(textureID, 0, width, height, width*sizeof(COLORREF), 
			PixelFormat(PF_RGBA),
			bumpBits, 0, 0);

		COLORREF * newBits = texLoadBitsB;
		int mipmapSize = width;
		
		for(int level = 1; level < numMipmapLevels; level++)
		{
			mipmapSize /= 2;
			if (mipmapSize < 1) break;
			createMipmap(bits,newBits,mipmapSize * 2, false);

			if (bump) convertBumpmap(newBits,bumpBits, width);
			else bumpBits = newBits;


			PIPE->set_texture_level_data(textureID, level, mipmapSize, mipmapSize, mipmapSize*sizeof(COLORREF), 
				PixelFormat(PF_RGBA),
				bumpBits, 0, 0);
			COLORREF * tmpSwap = bits;
			bits = newBits;
			newBits = tmpSwap;
		}
	}
	}
	
Done:	
//	free(bits);
	return textureID;
}
//--------------------------------------------------------------------------//
//
U32 TManager::createTextureFromFile (const char *fileName, IComponentFactory *parentFile, DA::FILETYPE type, const PixelFormat &format)
{
	U32 textureID=0;


	// temporary kludge... unless it never gets changed... in which case it would be a permanent kludge... which would be bad... probably...
	if (strstr(fileName, ".dds"))
	{
		PIPE->create_cube_texture_from_file(fileName, parentFile,textureID);
		return textureID;
	}



	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc = fileName;
	HANDLE hMapping;
	U8 * pMemory;

	fdesc.lpImplementation = "DOS";
 
	if (parentFile == 0)
		parentFile = DACOM;

	if (parentFile->CreateInstance(&fdesc, file) != GR_OK)
	{
		//CQFILENOTFOUND(fdesc.lpFileName);
		goto Done;
	}

	hMapping = file->CreateFileMapping();
	pMemory = (U8*) file->MapViewOfFile(hMapping);

	if (type == DA::UNKTYPE)
	{
		U32 filetype = ((U32 *)(fileName + strlen(fileName) - 4))[0] & ~0x20202000;

		switch (filetype)
		{
		case 'PMB.':
			type = DA::BMP;
			break;
		case 'PHS.':
			type = DA::VFX;
			break;
		case 'AGT.':
			type = DA::TGA;
			break;
		}
	}

	switch (type)
	{
	case DA::BMP:
		textureID = createTextureFromMemory(pMemory+14, type, format, fileName);
		break;
	default:
		textureID = createTextureFromMemory(pMemory, type, format, fileName);
		break;
	}

	file->UnmapViewOfFile(pMemory);
	file->CloseHandle(hMapping);

Done:
	return textureID;
}
//--------------------------------------------------------------------------//
//
void TManager::reset (void)
{
	textureList.print();
	textureList.reset();

	DANODE * node;

	node=pInUseList;
	while (node)
	{
		pInUseList = node->pNext;
		delete node;
		node=pInUseList;
	}

	node=pJustFreedList;
	while (node)
	{
		pJustFreedList = node->pNext;
		delete node;
		node=pJustFreedList;
	}

	node=pFreedList;
	while (node)
	{
		pFreedList = node->pNext;
		delete node;
		node=pFreedList;
	}
}
//--------------------------------------------------------------------------//
//
void TManager::addTexture (const char * name, U32 textureID)
{
	TMNODE * node = new TMNODE(name);
	node->textureID = textureID;
	textureList.addNode(node);
}
//--------------------------------------------------------------------------//
//
TMNODE * TManager::findTexture (U32 textureID)
{
	TMNODE * node = textureList.findNode(textureID);
	return node;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _tmanager : GlobalComponent
{
	TManager * manager;

	virtual void Startup (void)
	{
		TMANAGER = manager = new DAComponent<TManager>;
		AddToGlobalCleanupList(&TMANAGER);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(TMANAGER, &manager->eventHandle);
	}
};

static _tmanager tmanager;

//--------------------------------------------------------------------------//
//----------------------------End TManager.cpp-------------------------------//
//--------------------------------------------------------------------------//
