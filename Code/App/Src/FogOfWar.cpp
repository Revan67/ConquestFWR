//--------------------------------------------------------------------------//
//                                                                          //
//                               FogOfWar.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/FogOfWar.cpp 134   5/07/01 9:22a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Resource.h"
#include "Startup.h"
#include "TCPoint.h"
#include "Objlist.h"
#include "sector.h"
#include "IObject.h"
#include "FogOfWar.h"
#include "camera.h"
#include "TDocClient.h"
#include "UserDefaults.h"
#include "menu.h"
#include <DFog.h>
#include "CQTrace.h"
#include "SysMap.h"
#include "Sector.h"
#include "MGlobals.h"
#include "Gridvector.h"
#include "TManager.h"
#include "Objwatch.h"
#include "ObjmapIterator.h"
#include "DEffectOpts.h"

//#include <ITextureLibrary.h>
#include <FileSys.h>
#include <TComponent.h>
#include <TSmartPointer.h>
#include <Heapobj.h>
#include <Viewer.h>
#include <EventSys.h>
#include <3dmath.h>
#include <Engine.h>
#include <RendPipeline.h>
//#include <RPUL\PrimitiveBuilder.h>

#include <malloc.h>
//#include <stdlib.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#define unsafe_sqr(x) ((x)*(x))
//static struct FogOfWar * FOGOFWAR; 
typedef S32 INSTANCE_INDEX;
#define BUFFER_WIDTH 2000
#define SQ_BUFFER_WIDTH (BUFFER_WIDTH*BUFFER_WIDTH)//4000000 //20000000
#define INV_BUFFER_WIDTH (0.5/BUFFER_WIDTH)//2.5e-4 //half of the inverse of buffer_width

struct IntVec2d
{
	int x,y;
};

struct Viewzone
{
	S32 rad;
	SINGLE radius;
	SINGLE magic_rad;
	S32 cloakedRadius;
	S32 cloakedRad;
	S32 bufferedRad;
	S32 valx0,valx1;
	S32 valy0,valy1;
	Vector vec;
	IntVec2d pos;

	BOOL32 ColorBit(S32 lowx,S32 lowy,COLORREF *bit,SINGLE short_rad);
};

//assumes only the alpha needs to be changed
BOOL32 Viewzone::ColorBit(S32 lowx, S32 lowy,COLORREF *bit,SINGLE short_rad)
{
	S32 xdiff = abs(pos.x - lowx);
	S32 ydiff = abs(pos.y - lowy);
	
	if (xdiff < bufferedRad && ydiff < bufferedRad)
	{
		if (xdiff < short_rad-BUFFER_WIDTH  && ydiff < short_rad-BUFFER_WIDTH )
		{
			*bit &= 0x00ffffff;
			return 0;
		}
		
		SINGLE dist = SINGLE(xdiff)*SINGLE(xdiff)+
			SINGLE(ydiff)*SINGLE(ydiff);
		if (dist < magic_rad)
		{
			dist = sqrt(dist);
			if (dist < radius - BUFFER_WIDTH)
			{
				*bit &= 0x00ffffff;
				return 0;
			}
			else
			{
				U32 alpha = *bit >> 24;
				SINGLE alpha2 = (1 - (SINGLE)(bufferedRad - dist)*INV_BUFFER_WIDTH);
				*bit &= 0x00ffffff;
				*bit |= F2LONG(alpha*alpha2) << 24;
			}
		}
	}

	return 1;
}

struct Cloudzone
{
	RECT r;
	SINGLE alpha[9];
};

#define MAX_ZONES 512
#define BLACKFOG_RES 32
#define TEXTURE_RES 32
#define SKIP 1



static char szRegKey[] = "Fog";
static S32 mheight,mwidth;
static U32 lastSystem = 0;

struct BlackFog
{
	U8 *buffer;
	U32 width,height;

	bool isFogged(int i,int j,U8 allyMask)
	{
		return ((buffer[j*width+i] & allyMask) == 0);
	}

	bool setFog(int i,int j,U8 playerMask)
	{
		buffer[j*width+i] &= ~(playerMask);
	}

	void clearFog(int i,int j,U8 playerMask)
	{
		buffer[j*width+i] |= playerMask;
	}

	void allocate(int _width,int _height)
	{
		width = _width;
		height = _height;
		buffer = new U8[width*height];
	}

	BOOL32 load(IFileSystem *file,U32 _width,U32 _height)
	{
		BOOL32 result=0;

		U32 pack_buff_size,pack_buff_pos=0;
		DWORD dwRead;
		U8 *pack_buff=0;
		U8 currentByte;
		U8 currentCount;
		U32 i;
		U32 buff_pos=0;
		U8 *byte_buff;
		
		if (_width == width && _height == height)
		{
			byte_buff = (U8 *)buffer;
		}
		else
			byte_buff = (U8 *)malloc(_width*_height*sizeof(U8));

		if (file->ReadFile(0,&pack_buff_size,sizeof(pack_buff_size),&dwRead) ==0)
			goto Done;
		pack_buff = (U8 *)malloc(pack_buff_size);
		if (file->ReadFile(0,pack_buff,pack_buff_size,&dwRead,0) ==0)
			goto Done;

		while (pack_buff_pos < pack_buff_size)
		{
			currentCount = pack_buff[pack_buff_pos];
			pack_buff_pos++;
			currentByte = pack_buff[pack_buff_pos];
			pack_buff_pos++;
			
			for (i=0;i<currentCount;i++)
			{
				byte_buff[buff_pos] = currentByte;
				buff_pos++;
			}
		}

		//convert from old fog resolutions
		if (_width != width || _height != height)
		{
			for (U32 x=0;x<width;x++)
			{
				for (U32 y=0;y<height;y++)
				{
				//	bool bit=0;
					
					S32 xx,yy;
					xx = (SINGLE)x*_width/width;
					yy = (SINGLE)y*_height/height;
					
				/*	int segment,offset;
					
					segment = yy*_width+xx/32;
					offset = xx%32;
					
					bit = (((U32 *)byte_buff)[segment] >> offset & 0x1 != 0);*/

					buffer[y*width+x] = byte_buff[yy*_width+xx];
				}
			}
			free(byte_buff);
		}

		result = 1;

		Done:

		if (pack_buff)
			free(pack_buff);

		return result;
	}

	BOOL32 save(IFileSystem *file)
	{
		BOOL32 result=0;

		U32 buff_size = sizeof(U8)*height*width;
		U32 buff_pos=0;
		U8 *byte_buff = (U8 *)buffer;

		DWORD dwWritten;
		U8 *pack_buff = (U8 *)malloc(2*buff_size);

		//make sure first byte is new
		U8 currentByte=*byte_buff;
		U8 currentCount=1;
		U32 pack_buff_pos=0;

		buff_pos++;
		while (buff_pos < buff_size)
		{
			if (currentCount < 255 && byte_buff[buff_pos] == currentByte)
			{
				currentCount++;
			}
			else
			{
				pack_buff[pack_buff_pos] = currentCount;
				pack_buff_pos++;
				pack_buff[pack_buff_pos] = currentByte;
				pack_buff_pos++;
				currentCount=1;
				currentByte = byte_buff[buff_pos];
			}
			
			buff_pos++;
		}
		
		pack_buff[pack_buff_pos] = currentCount;
		pack_buff_pos++;
		pack_buff[pack_buff_pos] = currentByte;
		pack_buff_pos++;
		

		if (file->WriteFile(0,&pack_buff_pos,sizeof(pack_buff_pos),&dwWritten) ==0)
			goto Done;
		if (file->WriteFile(0,pack_buff,pack_buff_pos,&dwWritten,0) ==0)
			goto Done;
		
		result=1;
Done:
		if (pack_buff)
			free(pack_buff);
		return result;
	}

	void clear(U8 fill)
	{
		memset(buffer,fill,sizeof(U8)*height*width);
	}

	BlackFog::~BlackFog()
	{
		delete [] buffer;
	}
};

struct CircleBits
{
	U32 *buffer;
	U32 width,height;

	bool getBit(int i,int j)
	{
		int segment,offset;

		segment = j*width+i/32;
		offset = i%32;

		return ((buffer[segment] >> offset & 0x1) != 0);
	}

	void setBit(int i,int j)
	{
		int segment,offset;

		segment = j*width+i/32;
		offset = i%32;

		buffer[segment] |= (0x1 << offset);
	}

	void clearBit(int i,int j)
	{
		int segment,offset;

		segment = j*width+i/32;
		offset = i%32;

		buffer[segment] &= ~(0x1 << offset);
	};

	void allocate(int _width,int _height)
	{
		width = ceil(_width/32.0);
		height = _height;
		int size=height*width;
		buffer = new U32[size];
	};

/*	BOOL32 load(IFileSystem *file,U32 _width,U32 _height)
	{
		BOOL32 result=0;
		U32 pack_buff_size,pack_buff_pos=0;
		DWORD dwRead;
		U8 *pack_buff=0;
		U8 currentByte;
		U8 currentCount;
		U32 i;
		U32 buff_pos=0;
		U8 *byte_buff;
		
		U32 oldWidth = ceil(_width/32.0);
		if (oldWidth == width && _height == height)
		{
			byte_buff = (U8 *)buffer;
		}
		else
			byte_buff = (U8 *)malloc(ceil(_width/32.0)*_height*sizeof(U32));

		if (file->ReadFile(0,&pack_buff_size,sizeof(pack_buff_size),&dwRead) ==0)
			goto Done;
		pack_buff = (U8 *)malloc(pack_buff_size);
		if (file->ReadFile(0,pack_buff,pack_buff_size,&dwRead,0) ==0)
			goto Done;

		while (pack_buff_pos < pack_buff_size)
		{
			currentCount = pack_buff[pack_buff_pos];
			pack_buff_pos++;
			currentByte = pack_buff[pack_buff_pos];
			pack_buff_pos++;
			
			for (i=0;i<currentCount;i++)
			{
				byte_buff[buff_pos] = currentByte;
				buff_pos++;
			}
		}

		//convert from old fog resolutions
		if (oldWidth != width || _height != height)
		{
			for (U32 x=0;x<BLACKFOG_RES;x++)
			{
				for (U32 y=0;y<height;y++)
				{
					bool bit=0;
					
					S32 xx,yy;
					xx = (SINGLE)x*_width/BLACKFOG_RES;
					yy = (SINGLE)y*_height/height;
					
					int segment,offset;
					
					segment = yy*oldWidth+xx/32;
					offset = xx%32;
					
					bit = (((U32 *)byte_buff)[segment] >> offset & 0x1 != 0);

					if (bit)
						setBit(x,y);
					else
						clearBit(x,y);
				}
			}
			free(byte_buff);
		}

		result = 1;

		Done:

		if (pack_buff)
			free(pack_buff);

		return result;
	}

	BOOL32 save(IFileSystem *file)
	{
		BOOL32 result=0;

		U32 buff_size = sizeof(U32)*height*width;
		U32 buff_pos=0;
		U8 *byte_buff = (U8 *)buffer;

		DWORD dwWritten;
		U8 *pack_buff = (U8 *)malloc(2*buff_size);

		//make sure first byte is new
		U8 currentByte=*byte_buff;
		U8 currentCount=1;
		U32 pack_buff_pos=0;

		buff_pos++;
		while (buff_pos < buff_size)
		{
			if (currentCount < 255 && byte_buff[buff_pos] == currentByte)
			{
				currentCount++;
			}
			else
			{
				pack_buff[pack_buff_pos] = currentCount;
				pack_buff_pos++;
				pack_buff[pack_buff_pos] = currentByte;
				pack_buff_pos++;
				currentCount=1;
				currentByte = byte_buff[buff_pos];
			}
			
			buff_pos++;
		}
		
		pack_buff[pack_buff_pos] = currentCount;
		pack_buff_pos++;
		pack_buff[pack_buff_pos] = currentByte;
		pack_buff_pos++;
		

		if (file->WriteFile(0,&pack_buff_pos,sizeof(pack_buff_pos),&dwWritten) ==0)
			goto Done;
		if (file->WriteFile(0,pack_buff,pack_buff_pos,&dwWritten,0) ==0)
			goto Done;
		
		result=1;
Done:
		if (pack_buff)
			free(pack_buff);
		return result;
	}*/

	void clear(U8 fill)
	{
		memset(buffer,fill,sizeof(U32)*height*width);
	}

	CircleBits::~CircleBits()
	{
		delete [] buffer;
	}
};
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct DACOM_NO_VTABLE FogOfWar : IFogOfWar, IEventCallback, DocumentClient
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(FogOfWar)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	END_DACOM_MAP()

	// infrastructure
	COMPTR<IViewer> viewer;
	COMPTR<IDocument> doc;

	U32 eventHandle;
	U32 menuID;
	FOG_DATA fogData;
	SINGLE fogAlpha;

	// functionality
	U32 numZones;
	Viewzone zones[MAX_ZONES];
	U32 textureID;

//	U32 numNebZones;
//	Cloudzone nebZones[MAX_ZONES];
//	RECT quickZone;
	BOOL *mapGrid;
	S32 mapX,mapY;

	U32 mapTexID[MAX_SYSTEMS];
	COLORREF bits_array[MAX_SYSTEMS][TEXTURE_RES*TEXTURE_RES];
	U32 update_step[MAX_SYSTEMS];

	//COLORREF *fogTexPixels;
	//U32 fogTexSize;

	int sys_count;

	//BLACK FOG
	//what point on the map does 0,0 represent?  Actual map origin?
	BlackFog blackFog[MAX_SYSTEMS];
	CircleBits circleBits;
	CircleBits mapRimBits;
	// methods

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	FogOfWar (void)
	{
		int i,j;

		for (j=0;j<MAX_SYSTEMS;j++)
		{
			blackFog[j].allocate(BLACKFOG_RES,BLACKFOG_RES);
			blackFog[j].clear(0x00);
		}

		circleBits.allocate(TEXTURE_RES,TEXTURE_RES);
		circleBits.clear(0x00);
		for (i=0;i<TEXTURE_RES;i++)
		{
			for (j=0;j<TEXTURE_RES;j++)
			{
				SINGLE xdiff = TEXTURE_RES*0.5-(i+0.5);
				SINGLE ydiff = TEXTURE_RES*0.5-(j+0.5);

				if (xdiff*xdiff+ydiff*ydiff > (TEXTURE_RES*0.5*TEXTURE_RES*0.5)/((1+2*TEX_PAD_RATIO)*(1+2*TEX_PAD_RATIO)))
				{
					circleBits.setBit(i,j);
				}
			}
		}

		mapRimBits.allocate(TEXTURE_RES,TEXTURE_RES);
		mapRimBits.clear(0x00);
		for (i=0;i<TEXTURE_RES;i++)
		{
			for (j=0;j<TEXTURE_RES;j++)
			{
				SINGLE xdiff = TEXTURE_RES*0.5-(i+0.5);
				SINGLE ydiff = TEXTURE_RES*0.5-(j+0.5);

				if (xdiff*xdiff+ydiff*ydiff > (TEXTURE_RES*0.5)*(TEXTURE_RES*0.5))
				{
						mapRimBits.setBit(i,j);
				}
			}
		}
	}

	~FogOfWar (void);
	
	IDAComponent * GetBase (void)
	{
		return (IEventCallback *) this;
	}

	void Init(void);

	U8 InNeb(S32 x,S32 y,U8 fog);

    /* IFogOfWar methods */

    virtual BOOL32 CheckVisiblePosition (const class Vector & pos);

	virtual BOOL32 CheckHardFogVisibility (U32 playerID,U32 systemID,const class Vector & pos,SINGLE radius=0);

	virtual BOOL32 CheckHardFogVisibilityForThisPlayer (U32 systemID,const class Vector & pos, SINGLE radius=0);

	virtual BOOL32 CheckVisiblePositionCloaked (const class Vector & pos);

 	virtual Vector FindHardFog (U32 playerID, U32 systemID, const class Vector & pos, Vector * avoidList, U32 numAvoidPoints);

	virtual Vector FindHardFogFiltered (U32 playerID, U32 systemID, const class Vector & pos, Vector * avoidList, U32 numAvoidPoints, const class Vector & filterPos,SINGLE fiterRad);

	virtual void Update (void);

    virtual void Render (void);

	virtual void UpdateFog (int sys);

	virtual void MapRender (int sys,int x,int y,int sizex,int sizey);

//	virtual void AddNebulae (struct Cloudzone *zones,U32 numZones);

	// object has already verified that its side == global side
	virtual void RevealZone (struct IBaseObject * obj, SINGLE radius, SINGLE cloakedRadius);

	virtual void RevealBlackZone (U32 playerID,U32 systemID,const Vector &pos, SINGLE radius);

	virtual void ClearAllHardFog();

	virtual SINGLE GetPercentageFogCleared(U32 playerID,U32 systemID);

	BOOL32 CreateViewer (void);
	
	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message = 0, void *parm = 0);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param);

	/* FogOfWar methods */

	void loadTextures (bool bLoad);

	virtual void MakeMapTextures(int numSystems);

	void UpdateMapTexture(int tex);

	virtual BOOL32 New();

	virtual	BOOL32 __stdcall Save (struct IFileSystem * outFile);

	virtual	BOOL32 __stdcall Load (struct IFileSystem * inFile);
};
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//
void FogOfWar::loadTextures (bool bLoad)
{
	if (bLoad)
	{
		CQASSERT(textureID==0);
		textureID = TMANAGER->CreateTextureFromFile("fog_tile.tga", TEXTURESDIR,DA::TGA,PF_4CC_DAOT);

		//copy fog texture
	/*	RPLOCKDATA rpLockData;

		PIPE->lock_texture(textureID,0,&rpLockData);
		CQASSERT(rpLockData.pf.num_bits() == 16);
		fogTexPixels = (COLORREF *)malloc(rpLockData.width*rpLockData.height*4);
		fogTexSize = rpLockData.width;
		COLORREF *dst = fogTexPixels;
		U16 *src = (U16 *)rpLockData.pixels;
		U8 r,g,b,a;
		int rr = rpLockData.pf.rr;
		int gr = rpLockData.pf.gr;
		int br = rpLockData.pf.br;
		int ar = rpLockData.pf.ar;
		int rl = rpLockData.pf.rl;
		int gl = rpLockData.pf.gl;
		int bl = rpLockData.pf.bl;
		int al = rpLockData.pf.al;

		for (unsigned int j=0;j<fogTexSize;j++)
		{
			src = (U16 *)((U8 *)rpLockData.pixels+rpLockData.pitch*j);
			for (unsigned int i=0;i<rpLockData.width;i++)
			{
				a = 0xff;
				r = ((*src) >> rl) << rr;
				g = ((*src) >> gl) << gr;
				b = ((*src) >> bl) << br;
				if (rpLockData.pf.awidth)
				{
					a = ((*src) >> al) << ar;
				}

				*dst = RGB(r,g,b) | a<<24;
				dst++;
				src++;
			}
		}
		PIPE->unlock_texture(textureID,0);*/
	}
	else
	{
		TMANAGER->ReleaseTextureRef(textureID);
		textureID = 0;
//		free(fogTexPixels);
//		fogTexPixels =0;
	}
}
//------------------------------------------------------------------------
//
FogOfWar::~FogOfWar (void)
{
	if (GS)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}

	DEFAULTS->SetDataInRegistry(szRegKey, &fogData, sizeof(fogData));

//	delete [] mapGrid;
//	if (bits)
//		free(bits);
}

void FogOfWar::Init()
{
	COMPTR<IDAConnectionPoint> connection;
	
	if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
	{
		CQASSERT(eventHandle==0);
		connection->Advise(GetBase(), &eventHandle);
	}
}
/*
void FogOfWar::AddNebulae (Cloudzone *zones,U32 numZones)
{
	U32 i;

	for (i=0;i<numZones;i++)
	{
		nebZones[numNebZones+i] = zones[i];
		if (zones[i].r.left < quickZone.left)
			quickZone.left = zones[i].r.left;
		if (zones[i].r.right > quickZone.right)
			quickZone.right = zones[i].r.right;
		if (zones[i].r.top < quickZone.top)
			quickZone.top = zones[i].r.top;
		if (zones[i].r.bottom > quickZone.bottom)
			quickZone.bottom = zones[i].r.bottom;
	}

	numNebZones += numZones;
}*/
//--------------------------------------------------------------------------//
// Poll objects for visibility
//
void FogOfWar::Update (void)
{
	// can't short circuit this because won't show up, or they won't go away
	//	if (DEFAULTS->GetDefaults()->fogMode != FOGOWAR_NONE)// && (rand() & 7) == 0)
	{
		if (DEFAULTS->GetDefaults()->bVisibilityRulesOff)
			fogData.mapHardFog.a = 80;
		else
			fogData.mapHardFog.a = 255;


		if (CQFLAGS.bFullScreenMap)
		{
			for (int s=1;s<sys_count+1;s++)
			{
				UpdateFog(s);
			}
		}
		else
			UpdateFog(SECTOR->GetCurrentSystem());
	}
}
//--------------------------------------------------------------------------//
// object has already verified that its side == global side
//
void FogOfWar::RevealZone (struct IBaseObject * object, SINGLE radius, SINGLE cloakedRadius)
{
	if (DEFAULTS->GetDefaults()->fogMode == FOGOWAR_NONE)
		radius = 5000;	// a large number

	if (radius)
	{
		if (object->GetSystemID() != lastSystem)
		{
			lastSystem = object->GetSystemID();
			RECT sysMapRect;
			SYSMAP->GetSysMapRect(lastSystem,&sysMapRect);
			
			mwidth = sysMapRect.right - sysMapRect.left;
			mheight = sysMapRect.top - sysMapRect.bottom;
		}

		if(numZones < MAX_ZONES)
		{
			CQASSERT(numZones < MAX_ZONES);
			radius *= GRIDSIZE;		// convert to world coordinates
			cloakedRadius *= GRIDSIZE;
			zones[numZones].radius = radius;
			zones[numZones].rad = radius*radius;
			zones[numZones].magic_rad = (radius+BUFFER_WIDTH)*(radius+BUFFER_WIDTH);
			zones[numZones].cloakedRadius = cloakedRadius;
			zones[numZones].cloakedRad = cloakedRadius*cloakedRadius;
			zones[numZones].bufferedRad = radius+BUFFER_WIDTH;
			zones[numZones].vec = object->GetPosition();
			zones[numZones].pos.x = zones[numZones].vec.x;
			zones[numZones].pos.y = zones[numZones].vec.y;
			//add extra for overlay to avoid disallowing any revealing that should happen for small radii
			int gbuffer = (mwidth/mapX);
			zones[numZones].valx0 = zones[numZones].pos.x-zones[numZones].bufferedRad-gbuffer;
			zones[numZones].valx1 = zones[numZones].pos.x+zones[numZones].bufferedRad+gbuffer;
			zones[numZones].valy0 = zones[numZones].pos.y-zones[numZones].bufferedRad-gbuffer;
			zones[numZones].valy1 = zones[numZones].pos.y+zones[numZones].bufferedRad+gbuffer;
			numZones++;
		}
	}
}
//--------------------------------------------------------------------------//
// object has already verified that its side == global side
//
void FogOfWar::RevealBlackZone (U32 playerID,U32 systemID,const Vector &pos,SINGLE radius)
{
	CQASSERT(playerID && systemID);
	CQASSERT(systemID <= MAX_SYSTEMS);
	
	if (radius && DEFAULTS->GetDefaults()->fogMode == FOGOWAR_NORMAL)
	{
		U8 playerMask = 0x1 << (playerID-1);
		S32 minwX,maxwX,minwY,maxwY;
		S32 minX,maxX,minY,maxY;

		RECT sysRect;
		SECTOR->GetSystemRect(systemID,&sysRect);

		radius *= GRIDSIZE;			// convert to world coordinates
		minwX = pos.x-radius;
		maxwX = pos.x+radius;
		minwY = pos.y-radius;
		maxwY = pos.y+radius;

		S32 width = sysRect.right-sysRect.left;
		S32 height = sysRect.top-sysRect.bottom;

		//ftols here
		S32 posX=F2LONG(pos.x),posY=F2LONG(pos.y);

		S32 incX = width/BLACKFOG_RES;
		S32 incY = height/BLACKFOG_RES;
	
		minX = __max((sysRect.left+BLACKFOG_RES*minwX)/width,0);
		maxX = __min((sysRect.left+BLACKFOG_RES*maxwX)/width,BLACKFOG_RES-1);
		minY = __max((sysRect.bottom+BLACKFOG_RES*minwY)/height,0);
		maxY = __min((sysRect.bottom+BLACKFOG_RES*maxwY)/height,BLACKFOG_RES-1);

		for (int i=minX;i<=maxX;i++)
		{
			for (int j=minY;j<=maxY;j++)
			{
				S32 closeX,closeY;
				closeX = sysRect.left+(i*incX);
				if (abs(closeX-posX) > incX)
				{
					if (closeX > posX)
						closeX -= incX;
					else
						closeX += incX;
				}
				else if (posX > closeX)
					closeX = posX;

				closeY = sysRect.bottom+(j*incY);
				if (abs(closeY-posY) > incY)
				{
					if (closeY > posY)
						closeY -= incY;
					else
						closeY += incY;
				}
				else if (posY > closeY)
					closeY = posY;

				//reveals at center
			//	closeX = sysRect.left+(i+0.5)*incX;
			//	closeY = sysRect.bottom+(j+0.5)*incY;

				//blackFog is mapped to 0 base...?
				if (unsafe_sqr(closeX-pos.x)+unsafe_sqr(closeY-pos.y) < radius*radius)
					blackFog[systemID-1].clearFog(i,j,playerMask);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void FogOfWar::ClearAllHardFog()
{
	for (int s=0;s<SECTOR->GetNumSystems();s++)
	{
		blackFog[s].clear(0xff);
	}
}
//--------------------------------------------------------------------------//
//
SINGLE FogOfWar::GetPercentageFogCleared(U32 playerID,U32 systemID)
{
	SINGLE total=0;
	SINGLE cleared=0;

	U8 playerMask = 0x1 << (playerID-1);

	for (int i=0;i<BLACKFOG_RES;i++)
	{
		for (int j=0;j<BLACKFOG_RES;j++)
		{
			//blackFog is mapped to 0 base...?
			if (mapRimBits.getBit(i,j) == 0)
			{
				total++;
				if (blackFog[systemID-1].isFogged(i,j,playerMask) == 0)
					cleared++;
			}
		}
	}

	return cleared/total;
}
//--------------------------------------------------------------------------//
// Find if an object is not fogged
//
BOOL32 FogOfWar::CheckVisiblePosition (const class Vector & pos)
{
	U32 c;
	Vector loc = pos;

	for (c=0;c<numZones;c++)
	{
		SINGLE dist = (zones[c].vec.x - (loc.x))*(zones[c].vec.x - (loc.x))+
			(zones[c].vec.y - (loc.y))*(zones[c].vec.y - loc.y);
				
		if (dist < zones[c].rad)
		{
			return 1;
		}
	}

	return 0;
}

BOOL32 FogOfWar::CheckHardFogVisibility (U32 playerID,U32 systemID,const class Vector & pos, SINGLE radius)
{
	CQASSERT(playerID && systemID);
	if (DEFAULTS->GetDefaults()->fogMode != FOGOWAR_NORMAL)
		return 1;
	
	RECT sysRect;
	if (SECTOR->GetSystemRect(systemID,&sysRect) == FALSE)
		return 0;
	
	S32 width = sysRect.right-sysRect.left;
	S32 height = sysRect.top-sysRect.bottom;
	radius *= GRIDSIZE;

	U8 playerMask = MGlobals::GetAllyMask(playerID);

	if (radius == 0)
	{
		S32 cx,cy;
		cx = F2LONG(BLACKFOG_RES*pos.x/width);
		cy = F2LONG(BLACKFOG_RES*pos.y/height);
		if (cx < 0 || cy < 0 || cx >= BLACKFOG_RES || cy >= BLACKFOG_RES)
			return FALSE;
		if (blackFog[systemID-1].isFogged(cx,cy,playerMask) == 0)
			return TRUE;
	}
	else
	{
		S32 minwX,maxwX,minwY,maxwY;
		S32 minX,maxX,minY,maxY;

		RECT sysRect;
		SECTOR->GetSystemRect(systemID,&sysRect);

		minwX = pos.x-radius;
		maxwX = pos.x+radius;
		minwY = pos.y-radius;
		maxwY = pos.y+radius;

		S32 width = sysRect.right-sysRect.left;
		S32 height = sysRect.top-sysRect.bottom;

		//ftols here
		S32 posX=F2LONG(pos.x),posY=F2LONG(pos.y);

		S32 incX = width/BLACKFOG_RES;
		S32 incY = height/BLACKFOG_RES;
	
		minX = __max((sysRect.left+BLACKFOG_RES*minwX)/width,0);
		maxX = __min((sysRect.left+BLACKFOG_RES*maxwX)/width,BLACKFOG_RES-1);
		minY = __max((sysRect.bottom+BLACKFOG_RES*minwY)/height,0);
		maxY = __min((sysRect.bottom+BLACKFOG_RES*maxwY)/height,BLACKFOG_RES-1);

		for (int i=minX;i<=maxX;i++)
		{
			for (int j=minY;j<=maxY;j++)
			{
				S32 closeX,closeY;
				closeX = sysRect.left+(i*incX);
				if (abs(closeX-posX) > incX)
				{
					if (closeX > posX)
						closeX -= incX;
					else
						closeX += incX;
				}
				else if (posX > closeX)
					closeX = posX;

				closeY = sysRect.bottom+(j*incY);
				if (abs(closeY-posY) > incY)
				{
					if (closeY > posY)
						closeY -= incY;
					else
						closeY += incY;
				}
				else if (posY > closeY)
					closeY = posY;

				if (unsafe_sqr(closeX-pos.x)+unsafe_sqr(closeY-pos.y) < radius*radius)
				{
					if (blackFog[systemID-1].isFogged(BLACKFOG_RES*closeX/width,BLACKFOG_RES*closeY/height,playerMask) == 0)
						return TRUE;
				}
			}
		}
	}

	return FALSE;
}
//--------------------------------------------------------------------------//
///
BOOL32 FogOfWar::CheckHardFogVisibilityForThisPlayer (U32 systemID,const class Vector & pos, SINGLE radius)
{
	return CheckHardFogVisibility(MGlobals::GetThisPlayer(), systemID, pos, radius);
}
//--------------------------------------------------------------------------//
// Find if an object is not cloaked
//
BOOL32 FogOfWar::CheckVisiblePositionCloaked (const class Vector & pos)
{
	U32 c;
	Vector loc = pos;

	for (c=0;c<numZones;c++)
	{
		S32 rad = zones[c].cloakedRad;//zones[c].radius*zones[c].radius;
				
		SINGLE dist = (zones[c].vec.x - (loc.x))*(zones[c].vec.x - (loc.x))+
			(zones[c].vec.y - (loc.y))*(zones[c].vec.y - loc.y);
				
		if (dist < rad)
		{
			return 1;
		}
	}

	return 0;
}
//--------------------------------------------------------------------------//
//
Vector FogOfWar::FindHardFog (U32 playerID, U32 systemID, const class Vector & pos, Vector * avoidList, U32 numAvoidPoints)
{
	U8 playerMask = 0x1 << (playerID-1);

	RECT sysRect;
	if (SECTOR->GetSystemRect(systemID,&sysRect) == FALSE)
		return Vector(0,0,0);

	S32 width = sysRect.right-sysRect.left;
	S32 height = sysRect.top-sysRect.bottom;
	Vector nearest = Vector(0,0,0);
	SINGLE bestDist = 0;
	bool bFound = false;

	for (int i=0;i<BLACKFOG_RES;i++)
	{
		for (int j=0;j<BLACKFOG_RES;j++)
		{
			//blackFog is mapped to 0 base...?
			if (mapRimBits.getBit(i,j) == 0)
			{
				if (blackFog[systemID-1].isFogged(i,j,playerMask))
				{
					if(bFound)
					{
						Vector newPos(((SINGLE)i)*width/BLACKFOG_RES,((SINGLE)j)*height/BLACKFOG_RES,0);
						SINGLE newDist = (newPos-pos).fast_magnitude();
						SINGLE avoidWeight = 0;
						for(U32 av = 0; av < numAvoidPoints; ++av)
						{
							SINGLE mag = (avoidList[av]-newPos).magnitude_squared();
							if(mag < 4*GRIDSIZE*GRIDSIZE)
								avoidWeight += (4*GRIDSIZE*GRIDSIZE)-mag;
						}
						newDist += avoidWeight;
						if(newDist < bestDist)
						{
							bestDist = newDist;
							nearest = newPos;
						}
					}
					else
					{
						nearest = Vector(((SINGLE)i)*width/BLACKFOG_RES,((SINGLE)j)*height/BLACKFOG_RES,0);
						bFound = true;
						bestDist = (nearest-pos).fast_magnitude();
					}
				}
			}
		}
	}

	return nearest;
}
//--------------------------------------------------------------------------//
//
Vector FogOfWar::FindHardFogFiltered (U32 playerID, U32 systemID, const class Vector & pos, Vector * avoidList, U32 numAvoidPoints,
									  const class Vector & filterPos,SINGLE fiterRad)
{
	U8 playerMask = 0x1 << (playerID-1);

	RECT sysRect;
	if (SECTOR->GetSystemRect(systemID,&sysRect) == FALSE)
		return Vector(0,0,0);

	fiterRad *= fiterRad;

	S32 width = sysRect.right-sysRect.left;
	S32 height = sysRect.top-sysRect.bottom;
	Vector nearest = Vector(0,0,0);
	SINGLE bestDist = 0;
	bool bFound = false;

	for (int i=0;i<BLACKFOG_RES;i++)
	{
		for (int j=0;j<BLACKFOG_RES;j++)
		{
			//blackFog is mapped to 0 base...?
			if (mapRimBits.getBit(i,j) == 0)
			{
				if (blackFog[systemID-1].isFogged(i,j,playerMask))
				{
					Vector newPos(((SINGLE)i)*width/BLACKFOG_RES,((SINGLE)j)*height/BLACKFOG_RES,0);
					SINGLE filterCheckDist = (newPos-filterPos).magnitude_squared();
					if(filterCheckDist <= fiterRad)
					{
						if(bFound)
						{
							SINGLE newDist = (newPos-pos).fast_magnitude();
							SINGLE avoidWeight = 0;
							for(U32 av = 0; av < numAvoidPoints; ++av)
							{
								SINGLE mag = (avoidList[av]-newPos).magnitude_squared();
								if(mag < 4*GRIDSIZE*GRIDSIZE)
									avoidWeight += (4*GRIDSIZE*GRIDSIZE)-mag;
							}
							newDist += avoidWeight;
							if(newDist < bestDist)
							{
								bestDist = newDist;
								nearest = newPos;
							}
						}
						else
						{
							nearest = newPos;
							bFound = true;
							bestDist = (nearest-pos).fast_magnitude();
						}
					}
				}
			}
		}
	}

	return nearest;}
//--------------------------------------------------------------------------//
//
/*U8 FogOfWar::InNeb(S32 x,S32 y,U8 fog)
{
#define DIV 0.00025

	if (x<quickZone.left || x > quickZone.right || y<quickZone.top || y>quickZone.bottom)
		return fog;

	U32 c,i,j;
	SINGLE xDiff,yDiff;
	for (c=0;c<numNebZones;c++)
	{
		if ((x>nebZones[c].r.left && x<nebZones[c].r.right) && (y>nebZones[c].r.top && y<nebZones[c].r.bottom))
		{
			xDiff = (x-(nebZones[c].r.left+4000))*DIV;
			yDiff = (y-(nebZones[c].r.top+4000))*DIV;
			if (xDiff < 0)
			{
				xDiff = -xDiff;
				i=0;
			}
			else
				i=2;
			if (yDiff <0)
			{
				yDiff = -yDiff;
				j=0;
			}
			else
				j=2;

			if (nebZones[c].alpha[i*3+j] != 0)
			{
				if (nebZones[c].alpha[i*3+1] != 0)
					xDiff = 0.1-xDiff;
				if (nebZones[c].alpha[3+j] != 0)
					yDiff = 0.1-yDiff;
			}
			else
			{
				if (nebZones[c].alpha[3+j] != 0)
				{
					yDiff =xDiff*0.3;
				//	xDiff *= 0.1;
				}
				if (nebZones[c].alpha[i*3+1] != 0)
				{
					xDiff =yDiff*0.3;
				//	yDiff *= 0.1;
				}
				xDiff *=xDiff*1.4;
				yDiff *=yDiff*1.4;
				if (xDiff > 1)
					xDiff = 1;
				if (yDiff > 1)
					yDiff = 1;
			}

			
			U32 temp = 0;//fog+ fogData.alpha*((1-xDiff)+(1-yDiff))*0.8;
		//	if (temp > fogData.alpha)
		//		return fogData.alpha;
			
			return temp;
		}
	}

	return fog;
}*/

#define MAX_SQUARES 100
#define SQUARES fogData.resolution


//--------------------------------------------------------------------------//
// Put actual misty stuff on map
//
void FogOfWar::Render()
{
	if (CQEFFECTS.bExpensiveTerrain==0)
		return;

	if (DEFAULTS->GetDefaults()->bSpectatorModeOn)
	{
		// don't render any fog if we are a spectator
		return;
	}

	Vector p[4];
//	SINGLE tempx,tempy;
//	SINGLE grid[MAX_SQUARES+1][MAX_SQUARES];
//	BOOL gridTab[MAX_SQUARES];
	S32 height,width;
//	SINGLE inset,offsetX,offsetY;
	//	SINGLE xStep, yStep;
	//	S32 i,j;
	//	U32 c;
	Vector low,high,count,iStep,jStep;
	COLORREF *bits = bits_array[SECTOR->GetCurrentSystem()-1];
	
	static int counter=0;
	counter++;
	
	if (CAMERA && DEFAULTS->GetDefaults()->fogMode != FOGOWAR_NONE)
	{
		const Transform & worldTrans = *CAMERA->GetWorldTransform();
		
		
		CAMERA->PaneToPoints(p[0], p[1], p[2], p[3]);
		
		height = p[3].y-p[0].y;
		width = p[1].x-p[0].x;

	//	xStep = width / (SQUARES-1);
	//	yStep = height / (SQUARES-1);

		S32 sysWidth,sysHeight;
		RECT sysMapRect;
		SYSMAP->GetSysMapRect(SECTOR->GetCurrentSystem(),&sysMapRect);

		sysWidth = sysMapRect.right-sysMapRect.left;
		sysHeight = sysMapRect.top-sysMapRect.bottom;


		p[0] = worldTrans.rotate_translate(p[0]);
		p[1] = worldTrans.rotate_translate(p[1]);
		p[2] = worldTrans.rotate_translate(p[2]);
		p[3] = worldTrans.rotate_translate(p[3]);

		S32 minX,minY,maxX,maxY;
		minX = min(p[0].x,min(p[1].x,min(p[2].x,p[3].x)));
		maxX = max(p[0].x,max(p[1].x,max(p[2].x,p[3].x)));
		minY = min(p[0].y,min(p[1].y,min(p[2].y,p[3].y)));
		maxY = max(p[0].y,max(p[1].y,max(p[2].y,p[3].y)));


		BATCH->set_state(RPR_BATCH,false);
		CAMERA->SetPerspective();
		CAMERA->SetModelView();

	//	DisableTextures();
		SetupDiffuseBlend(textureID,FALSE);
		BATCH->set_render_state(D3DRS_CLIPPING,TRUE);
		PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		PIPE->set_render_state(D3DRS_ALPHATESTENABLE,FALSE);
		PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
		PIPE->set_render_state(D3DRS_ZENABLE,FALSE);
		PIPE->set_render_state(D3DRS_DITHERENABLE,TRUE);
		PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);


		PB.Color4ub(255,255,255,255);
		if (CQRENDERFLAGS.bNoPerVertexAlpha)
			PB.Begin(PB_TRIANGLES);
		else
			PB.Begin(PB_QUADS);
		
#define FOG_RES (TEXTURE_RES/SKIP)
		S32 incX,incY;
		incX = sysWidth/TEXTURE_RES;
		incY = sysHeight/TEXTURE_RES;
		minX = SKIP*floor((float)(((minX-sysMapRect.left)*FOG_RES))/sysWidth)-1;
		minY = SKIP*floor((float)(((minY-sysMapRect.bottom)*FOG_RES))/sysHeight)-1;
		maxX = SKIP*floor((float)(((maxX-sysMapRect.left)*FOG_RES))/sysWidth)+1;
		maxY = SKIP*floor((float)(((maxY-sysMapRect.bottom)*FOG_RES))/sysHeight)+1;

		minX = min(max(minX,-1),FOG_RES);
		maxX = min(max(maxX,-1),FOG_RES);
		minY = min(max(minY,-1),FOG_RES);
		maxY = min(max(maxY,-1),FOG_RES);
		for (int cx=minX;cx<maxX;cx+=SKIP)
		{
			for (int cy=minY;cy<maxY;cy+=SKIP)
			{
				COLORREF bit[4];
				S32 posX,posY;
				
				posX = cx*incX+incX/2+sysMapRect.left;
				posY = cy*incY+incY/2+sysMapRect.bottom;
				
				//COLORREF hf_color=RGB(fogData.mapHardFog.r,fogData.mapHardFog.g,fogData.mapHardFog.b) | fogData.mapHardFog.a<<24;
				COLORREF hf_color=RGB(120,18,15) | fogData.mapHardFog.a<<24;
				bit[0]=bit[1]=bit[2]=bit[3]=hf_color;
				
				//circleBits is supposed to be over the padded map
				//mapRimBits is aligned to the other one
			//	S32 circx = cx;//F2LONG(posX/SINGLE(sWidth)*TEXTURE_RES);
			//	S32 circy = cy;//F2LONG(posY/SINGLE(sHeight)*TEXTURE_RES);
				
				if(cx == -1 || cy == -1)
					bit[0] = 0x00000000;
				else if (cx > -1 && cy > -1)
					if (circleBits.getBit(cx,cy) == 0)
						bit[0]=bits[cy*TEXTURE_RES+cx];

				if(cx == TEXTURE_RES-1 || cy == -1)
					bit[1] = 0x00000000;
				else if (cx < TEXTURE_RES-1 && cy > -1)
					if (circleBits.getBit(cx+SKIP,cy) == 0)
						bit[1]=bits[cy*TEXTURE_RES+cx+SKIP];
				
				if(cx == TEXTURE_RES-1 || cy == TEXTURE_RES-1)
					bit[2] = 0x00000000;
				else if (cx < TEXTURE_RES-1 && cy < TEXTURE_RES-1)
					if (circleBits.getBit(cx+SKIP,cy+SKIP) == 0)
						bit[2]=bits[(cy+SKIP)*TEXTURE_RES+cx+SKIP];
					
				if(cx == -1 || cy == TEXTURE_RES-1)
					bit[3] = 0x00000000;
				else if (cx > -1 && cy < TEXTURE_RES-1)
					if (circleBits.getBit(cx,cy+SKIP) == 0)
						bit[3]=bits[(cy+SKIP)*TEXTURE_RES+cx];
						
				if (bit[0] || bit[1] || bit[2] || bit[3])
				{
					BOOL32 flag=0;
					
					//choose winding order based on alpha values
					if (bit[0] <= bit[2] && bit[0] <= bit[3] && bit[0] <= bit[1])
					{
						flag = TRUE;
					}
					else if (bit[2] <= bit[0] && bit[2] <= bit[3] && bit[2] <= bit[1])
						flag = TRUE;
					
#define FTTILE 0.0003


					struct TexCoord{
						SINGLE u,v;
					}t;
					t.u = 0;//posX*FTTILE+0.5*((counter+posX)%2);
					t.v = 0;//posY*FTTILE+0.5*((counter+posY)%4<2);
					
					
					if (CQRENDERFLAGS.bNoPerVertexAlpha)
					{
						static U8 wrap[2][6] = {{3,0,1,3,1,2},{0,1,2,0,2,3}};
						U8 wrap_choice=1;
						if (flag)
							wrap_choice=0;
						
						U32 color[6];
						
						for (int w=0;w<2;w++)
						{
							U32 maxAlpha=0;
							U32 minAlpha = 255;
							U32 drawAlpha=0;
							int c;
							for (c=0;c<3;c++)
							{
								U32 & bbit = bit[wrap[wrap_choice][w*3+c]];
								if (((bbit>>24)&0xff) > maxAlpha)
									maxAlpha = (bbit>>24)&0xff;
								if (((bbit>>24)&0xff) < minAlpha)
									minAlpha = (bbit>>24)&0xff;
							//	drawAlpha += (bbit>>24)&0xff;
							}
							//drawAlpha /= 3;
							drawAlpha = (minAlpha+maxAlpha)/2;
							for (c=0;c<3;c++)
							{
								color[w*3+c] = 0;
								U32 & bbit = bit[wrap[wrap_choice][w*3+c]];
								if (drawAlpha)
								{
									U32 alpha = ((bbit&0xff000000)/drawAlpha);
									alpha = alpha>>16;  //magic shift to take alpha down to 0..256 - higher with average alpha

									//capping on next 3 lines
									color[w*3+c] |= ((min(alpha*(bbit&0xff),0xff00))>>8)<<16;
									color[w*3+c] |= ((min(alpha*(bbit&0xff00),0xff0000))>>16)<<8;
									//these calculations depend on overflow, so this one must be divided down before being multiplied
									color[w*3+c] |= ((min(alpha*((bbit&0xff0000)>>16),0xff00))>>8);
									//different than below cause alpha is not coming out of bbit
								//	color[w*3+c] = (bbit&0x0000ff00) | ((bbit&0xff)<<16) | ((bbit>>16)&0xff);
									color[w*3+c] |= drawAlpha<<24;
								}
							}
						}

						//set up winding order for prettiness
						if (flag)
						{
							PB.TexCoord2f(t.u,t.v+(incY*SKIP)*FTTILE);
							PB.Color(color[0]);
							PB.Vertex3f(posX,posY+incY*SKIP,0);
							
							PB.TexCoord2f(t.u,t.v);
							PB.Color(color[1]);
							PB.Vertex3f(posX,posY,0);
							
							PB.TexCoord2f(t.u+(incX*SKIP)*FTTILE,t.v);
							PB.Color(color[2]);
							PB.Vertex3f(posX+incX*SKIP,posY,0);

							PB.TexCoord2f(t.u,t.v+(incY*SKIP)*FTTILE);
							PB.Color(color[3]);
							PB.Vertex3f(posX,posY+incY*SKIP,0);

							PB.TexCoord2f(t.u+(incX*SKIP)*FTTILE,t.v);
							PB.Color(color[4]);
							PB.Vertex3f(posX+incX*SKIP,posY,0);

							PB.TexCoord2f(t.u+(incX*SKIP)*FTTILE,t.v+(incY*SKIP)*FTTILE);
							PB.Color(color[5]);
							PB.Vertex3f(posX+incX*SKIP,posY+incY*SKIP,0);
						}
						else
						{
							PB.TexCoord2f(t.u,t.v);
							PB.Color(color[0]);
							PB.Vertex3f(posX,posY,0);
							
							PB.TexCoord2f(t.u+(incX*SKIP)*FTTILE,t.v);
							PB.Color(color[1]);
							PB.Vertex3f(posX+incX*SKIP,posY,0);

							PB.TexCoord2f(t.u+(incX*SKIP)*FTTILE,t.v+(incY*SKIP)*FTTILE);
							PB.Color(color[2]);
							PB.Vertex3f(posX+incX*SKIP,posY+incY*SKIP,0);

							PB.TexCoord2f(t.u,t.v);
							PB.Color(color[3]);
							PB.Vertex3f(posX,posY,0);

							PB.TexCoord2f(t.u+(incX*SKIP)*FTTILE,t.v+(incY*SKIP)*FTTILE);
							PB.Color(color[4]);
							PB.Vertex3f(posX+incX*SKIP,posY+incY*SKIP,0);
						
							PB.TexCoord2f(t.u,t.v+(incY*SKIP)*FTTILE);
							PB.Color(color[5]);
							PB.Vertex3f(posX,posY+incY*SKIP,0);
						}
					}
					else
					{
						U32 color[4];
						for (int c=0;c<4;c++)
							color[c] = (bit[c]&0xff00ff00) | ((bit[c]&0xff)<<16) | ((bit[c]>>16)&0xff);
						
						//set up winding order for prettiness
						if (flag)
						{
							PB.TexCoord2f(t.u,t.v+(incY*SKIP)*FTTILE);
							PB.Color(color[3]);
							PB.Vertex3f(posX,posY+incY*SKIP,0);
						}
						
						PB.TexCoord2f(t.u,t.v);
						PB.Color(color[0]);
						PB.Vertex3f(posX,posY,0);
						
						PB.TexCoord2f(t.u+(incX*SKIP)*FTTILE,t.v);
						PB.Color(color[1]);
						PB.Vertex3f(posX+incX*SKIP,posY,0);
						
						PB.TexCoord2f(t.u+(incX*SKIP)*FTTILE,t.v+(incY*SKIP)*FTTILE);
						PB.Color(color[2]);
						PB.Vertex3f(posX+incX*SKIP,posY+incY*SKIP,0);
						
						if (!flag)
						{
							PB.TexCoord2f(t.u,t.v+(incY*SKIP)*FTTILE);
							PB.Color(color[3]);
							PB.Vertex3f(posX,posY+incY*SKIP,0);
						}
					}
					
				}
			}
		}
		PB.End();
/*		DisableTextures();
		PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
		PB.Begin(PB_QUADS);

		//rather than introduce ftol's, I will put in a cheezy rounding error fudge.

		SINGLE alph=fogData.mapHardFog.a/255.0;
		//PB.Color4ub(fogData.mapHardFog.r,fogData.mapHardFog.g,fogData.mapHardFog.b,fogData.mapHardFog.a);
		PB.Color3ub(alph*fogData.mapHardFog.r,alph*fogData.mapHardFog.g,alph*fogData.mapHardFog.b);
		PB.Vertex3f(-99999,-99999,0);
		PB.Vertex3f(-99999,sysMapRect.bottom-incY/2,0);
		PB.Vertex3f(sysMapRect.right+99999,sysMapRect.bottom-incY/2,0);
		PB.Vertex3f(sysMapRect.right+99999,-99999,0);

		// -24 is the fudge
		PB.Vertex3f(-99999,sysMapRect.top+99999,0);
		PB.Vertex3f(-99999,sysMapRect.top+incY/2-24,0);
		PB.Vertex3f(sysMapRect.right+99999,sysMapRect.top+incY/2-24,0);
		PB.Vertex3f(sysMapRect.right+99999,99999,0);

		// -24 is the fudge
		PB.Vertex3f(sysMapRect.right+incX/2-24,sysMapRect.top+incY/2,0);
		PB.Vertex3f(sysMapRect.right+incX/2-24,sysMapRect.bottom-incY/2,0);
		PB.Vertex3f(sysMapRect.right+99999,sysMapRect.bottom-incY/2,0);
		PB.Vertex3f(sysMapRect.right+99999,sysMapRect.top+incY/2,0);

		PB.Vertex3f(-99999,sysMapRect.top+incY/2,0);
		PB.Vertex3f(-99999,sysMapRect.bottom-incY/2,0);
		PB.Vertex3f(sysMapRect.left-incY/2,sysMapRect.bottom-incY/2,0);
		PB.Vertex3f(sysMapRect.left-incY/2,sysMapRect.top+incY/2,0);

		PB.End();
*/		return;

	}
}

static bool bForceUpdate = false;
void FogOfWar::UpdateFog (int sys)
{
	numZones = 0;

	if (sys)
	{
		const U32 playerID = MGlobals::GetThisPlayer();
		const U32 mask = MGlobals::GetAllyMask(playerID) << 1;
		ObjMapIterator it(sys, Vector(0,0,0), 500000, playerID);

		while (it)
		{
			if ((mask & (1 << (it->dwMissionID&PLAYERID_MASK))) != 0)
			{
				it->obj->RevealFog(sys);
			}

			++it;
		}
	}

	COLORREF *bits = bits_array[sys-1];

#define TOP_RES 8
#define ZONE_EMPTY    -1
#define ZONE_OVERFLOW -2
#define ZONE_REVEALED -3
#define ZONE_DEPTH 16
#define MAP_STEP 4
	
	//	static SINGLE zorbx[MAX_ZONES];
	//	static SINGLE zorby[MAX_ZONES];
	static SINGLE short_rad[MAX_ZONES];
	static S32 topGrid[TOP_RES][TOP_RES][ZONE_DEPTH];
	static S32 topPts[TOP_RES+1][TOP_RES+1][ZONE_DEPTH];
	
	//	COLORREF cref = RGB(fogData.map_red,fogData.map_green,fogData.map_blue) | fogData.alpha<<24;
	
	if (DEFAULTS->GetDefaults()->fogMode != FOGOWAR_NONE)
	{
		//U32 i;
		Vector p[4];

		
		S32 i,j;
		U32 c;
		
	//	Vector low,base;
		S32 lowx,lowy,basex,basey;
		S32 iStep,jStep;
		
		U32 currentPlayer = MGlobals::GetThisPlayer();
		RECT sysMapRect;
		SYSMAP->GetSysMapRect(sys,&sysMapRect);
		RECT sysRect;
		SECTOR->GetSystemRect(sys,&sysRect);
		
		mwidth = sysMapRect.right - sysMapRect.left;
		mheight = sysMapRect.top - sysMapRect.bottom;
		
		iStep = mwidth/(mapX);//+0.5);
		jStep = mheight/(mapY);//+0.5);
		
#undef SQUARES
		
		S32 pix = mapX/TOP_RES;
		
		lowx = sysMapRect.left+iStep/2;//thumbRect.left-trans->translation.x;
		lowy = sysMapRect.bottom+jStep/2;//thumbRect.top-trans->translation.y;
//		low.z = 0;
//		base = low;
		basex = lowx;
		basey = lowy;
		
		U32 *step = &update_step[sys-1];
		S32 limit = *step+MAP_STEP;
		if (limit >= mapX)
		{
			limit = mapX;
		}
		if (bForceUpdate)
		{
			*step = 0;
			limit = mapX;
			bForceUpdate = false;
		}
//		low = base;
		lowx = basex;
		lowy = basey;
		
		//might like to make this update corresponding to stagger
		if (*step == 0)
		{
			//sample points on map
			memset(topPts,0xff, sizeof(topPts));
		}
		for (i=(*step*TOP_RES)/mapX;i<=(limit*TOP_RES)/mapX;i++)
		{
			lowx = basex+pix*i*iStep;
			for (j=0;j<=TOP_RES;j++)
			{
				int numHits=0;
				c=0;
				while (c<numZones && numHits < ZONE_DEPTH+1)
				{
					if (lowx > zones[c].valx0 && lowy > zones[c].valy0)
					{
						if (lowx<zones[c].valx1 && lowy < zones[c].valy1)
						{
							if (numHits < ZONE_DEPTH)
								topPts[i][j][numHits++] = c;
							else
							{
								numHits++;
								topPts[i][j][0] = ZONE_OVERFLOW;
							}
						}
					}
					
					c++;
				}
				lowy += pix*jStep;
			}
			//lowx = basex+pix*(i+1)*iStep;
			lowy = basey;
		}
		
		//consolidate points for regions
		memset(topGrid,0xff, sizeof(topGrid));
		for (i=(*step*TOP_RES)/mapX;i<(limit*TOP_RES)/mapX;i++)
		{
			for (j=0;j<TOP_RES;j++)
			{
				int numHits = 0;
				for (int a=0;a<2;a++)
				{
					for (int b=0;b<2;b++)
					{
						c=0;
						while (c<ZONE_DEPTH && topGrid[i][j][0] != ZONE_OVERFLOW)
						{
							if (topPts[i+a][j+b][c] == ZONE_OVERFLOW)
							{
								numHits = ZONE_DEPTH+1;
								topGrid[i][j][0] = ZONE_OVERFLOW;
							}
							
							if (numHits < ZONE_DEPTH+1 && topPts[i+a][j+b][c] != -1)
							{
								bool bRecorded = FALSE;
								for (int k=0;k<numHits;k++)
								{
									if (topGrid[i][j][k] == topPts[i+a][j+b][c])
									{
										bRecorded = TRUE;
									}
								}
								
								if (bRecorded == FALSE)
								{
									if (numHits < ZONE_DEPTH)
										topGrid[i][j][numHits++] = topPts[i+a][j+b][c];
									else
									{
										topGrid[i][j][0] = ZONE_OVERFLOW;
										numHits = ZONE_DEPTH+1;
									}
								}
							}
							else
								break;
							c++;
						}
					}
				}
			}
		}
		//	}

		//make sure that each unit reveals at least the square it is in
	/*	for (c=0;c<numZones;c++)
		{
			int numHits=0;
			bool bRecorded = FALSE;
			i = (zones[c].pos.x-basex) / iStep;
			j = (zones[c].pos.y-basey) / jStep;

			if (topGrid[i][j][0] != ZONE_OVERFLOW)
			{
				for (int k=0;k<ZONE_DEPTH;k++)
				{
					if (topGrid[i][j][k] == (S32)c)
					{
						bRecorded = TRUE;
					}
					if (topGrid[i][j][k] != 0xff)
					{
						numHits++;
					}
				}
			}
			
			if (bRecorded == FALSE)
			{
				if (numHits < ZONE_DEPTH)
					topGrid[i][j][numHits++] = c;
				else
				{
					topGrid[i][j][0] = ZONE_OVERFLOW;
					numHits = ZONE_DEPTH+1;
				}
			}
		}*/
		
		//this is the quick dropout case when objects fall inside the square revealed by the object
		for (c=0;c<numZones;c++)
		{
			short_rad[c] = zones[c].radius*0.707;
		}
		
		//setup the variables that keep track of where we are in the overlay grid
		int top_pos_i=(*step/pix),top_pos_j=0;
		int pix_i = (top_pos_i+1)*pix,pix_j=pix;
		
		BOOL *gridPos;
	//	low = base;
		lowx = basex+(*step)*iStep;
		lowy = basey;
		S32 cx,cy;
			
		SINGLE base_cy = BLACKFOG_RES*(sysMapRect.bottom+(mheight>>1)/mapY)/(SINGLE)sysRect.top;
		SINGLE cy_inc = BLACKFOG_RES*((SINGLE)mheight/mapY)/(SINGLE)sysRect.top;
		for (i=*step;i<limit;i++)
		{
			//update position in overlay grid
			if (i >= pix_i)
			{
				pix_i += pix;
				top_pos_i++;
			}
			pix_j = pix;
			top_pos_j = 0;
			
			//pos in main grid
			gridPos = mapGrid+i*mapY;
			
			//cx = floor(BLACKFOG_RES*(sysMapRect.left+mwidth*(i+0.5)/mapX)/(SINGLE)sysRect.right);
			//SINGLE base_cy = BLACKFOG_RES*(sysMapRect.bottom+mheight*(0.5)/mapY)/(SINGLE)sysRect.top;
			//SINGLE cy_inc = BLACKFOG_RES*((SINGLE)mheight/mapY)/(SINGLE)sysRect.top;

			cx = floor(BLACKFOG_RES*(sysMapRect.left+(mwidth*i+(mwidth>>1))/mapX)/(SINGLE)sysRect.right);
		
			//	BOOL32 foggedLine = TRUE;
			for (j=0;j<mapY;j++)
			{
				S32 xx,yy;
				
				cy = floor(base_cy+cy_inc*j);
				
				int hits=0;
				

				
				U32 r,g,b,a;
				//map texturing code, no go in low-res
			/*	U8 texr,texg,texb,texa;
				int y = (j)%fogTexSize;
				int x = (i)%fogTexSize;

				texr = GetRValue(fogTexPixels[y*fogTexSize+x]);
				texg = GetGValue(fogTexPixels[y*fogTexSize+x]);
				texb = GetBValue(fogTexPixels[y*fogTexSize+x]);
				texa = fogTexPixels[y*fogTexSize+x] >> 24;*/

				if (DEFAULTS->GetDefaults()->bEditorMode==0)
				{
					U8 playerMask = MGlobals::GetAllyMask(currentPlayer);
					if (0)//blackFog[currentPlayer-1][currentSysID-1].getBit(cx,cy))
					{
						r = fogData.mapHardFog.r;
						g = fogData.mapHardFog.g;
						b = fogData.mapHardFog.b;
						a = fogData.mapHardFog.a;
					}
					else
					{
						for (xx=cx-1;xx<cx+2;xx++)
						{
							for (yy=cy-1;yy<cy+2;yy++)
							{
								if (xx < 0 || xx > BLACKFOG_RES-1 || yy<0 || yy>BLACKFOG_RES-1 || blackFog[sys-1].isFogged(xx,yy,playerMask))
								{
									hits++;
									if (xx==cx && yy==cy)
										hits += 3;
								}
							}
						}
						
						
#define TOTAL_WEIGHTS 12
#define BIG_TOTAL_WEIGHTS (12<<8)
//#define TOTAL_WEIGHTS 12.0
						
						hits = hits << 8;
						S32 quot1,quot2;
						quot1 = (hits/TOTAL_WEIGHTS);
						quot2 = 256-quot1;//(BIG_TOTAL_WEIGHTS-hits)/TOTAL_WEIGHTS;
						
						r = (quot1*fogData.mapHardFog.r+(quot2)*fogData.mapSoftFog.r)>>8;
						g = (quot1*fogData.mapHardFog.g+(quot2)*fogData.mapSoftFog.g)>>8;
						b = (quot1*fogData.mapHardFog.b+(quot2)*fogData.mapSoftFog.b)>>8;
						a = (quot1*fogData.mapHardFog.a+(quot2)*fogData.mapSoftFog.a)>>8;
					}
				}
				else
				{
					r = fogData.mapSoftFog.r;
					g = fogData.mapSoftFog.g;
					b = fogData.mapSoftFog.b;
					a = fogData.mapSoftFog.a;
				}

				//more map texturing code
			/*	r = (r*texr) >> 8;
				g = (g*texg) >> 8;
				b = (b*texb) >> 8;
				a = (a*texa) >> 8;*/
				
				//assume alpha will be filled out below
				bits[j*mapX+i] = RGB(r,g,b);// | a<<24;

				if (mapRimBits.getBit(i,j) == 0)
				{
					bits[j*mapX+i] |= a << 24;
					//update position in overlay grid
					if (j >= pix_j)
					{
						pix_j += pix;
						top_pos_j++;
					}
					
					if (topGrid[top_pos_i][top_pos_j][0] != ZONE_EMPTY)
					{
						if (topGrid[top_pos_i][top_pos_j][0] == ZONE_OVERFLOW)
						{
							for (c=0;c<numZones;c++)
							{
								COLORREF *bit = &bits[j*mapX+i];
								if (zones[c].ColorBit(lowx,lowy,bit,short_rad[c]) == 0)
									break;
							}
						}
						else
						{
							for (int k=0;k<ZONE_DEPTH;k++)
							{
								c = topGrid[top_pos_i][top_pos_j][k];
								if (c != -1)
								{
									COLORREF *bit = &bits[j*mapX+i];
									if (zones[c].ColorBit(lowx,lowy,bit,short_rad[c]) == 0)
										break;
								}
								else
									break;
							}
						}
					}
					
				}
			//	else if (mapRimBits.getBit(i,j) == 0)  //works well for ugly red border
			//		bits[j*mapX+i] = RGB(120,18,15) | fogData.mapHardFog.a<<24;

				lowy += jStep;
				gridPos++;
			}
			lowx = basex + (i+1)*iStep;
			lowy = basey;
			//gridTab[i] = foggedLine;
		}

		if (limit == mapX)
		{
			*step = 0;
			UpdateMapTexture(sys);
		}
		else
			*step += MAP_STEP;

	}
}

void FogOfWar::MapRender (int sys,int x,int y,int sizex,int sizey)
{
	if (DEFAULTS->GetDefaults()->fogMode != FOGOWAR_NONE || DEFAULTS->GetDefaults()->bSpectatorModeOn)
	{
		SetupDiffuseBlend(mapTexID[sys-1],TRUE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	//	BATCH->set_texture_stage_state(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);

		SINGLE rot = CAMERA->GetWorldRotation();
		SINGLE cosR = cos(PI*(rot)/180.0);
		SINGLE sinR = sin(PI*(rot)/180.0);

		SINGLE offs=0;
		if (CQFLAGS.bTextureBias)
			offs = 1.0f/(2*TEXTURE_RES);

		PB.Begin(PB_QUADS);
		PB.Color3ub(255,255,255);

		PB.TexCoord2f(offs,offs);  PB.Vertex3f(x+sizex*(-cosR+sinR),y+sizey*(cosR+sinR),0);
		PB.TexCoord2f(1+offs,offs);  PB.Vertex3f(x+sizex*(cosR+sinR),y+sizey*(cosR-sinR),0);
		PB.TexCoord2f(1+offs,1+offs);  PB.Vertex3f(x+sizex*(cosR-sinR),y+sizey*(-cosR-sinR),0);
		PB.TexCoord2f(offs,1+offs);  PB.Vertex3f(x+sizex*(-cosR-sinR),y+sizey*(-cosR+sinR),0);

		PB.End();
	}
}
//--------------------------------------------------------------------------//
//
GENRESULT FogOfWar::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &fogData, sizeof(fogData), &dwRead, 0);

/*	if (fogData.resolution <2)
		fogData.resolution = 2;
	if (fogData.resolution > MAX_SQUARES)
		fogData.resolution = MAX_SQUARES;*/

	fogAlpha = fogData.softFog.a/255.0;


	return GR_OK;
}
//--------------------------------------------------------------------------//
//
BOOL32 FogOfWar::CreateViewer (void)
{
	HMENU hMenu = MENU->GetSubMenu(MENUPOS_VIEW);
	MENUITEMINFO minfo;

	memset(&minfo, 0, sizeof(minfo));
	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIIM_ID | MIIM_TYPE;
	minfo.fType = MFT_STRING;
	minfo.wID = IDS_VIEWFOG;
	minfo.dwTypeData = "Fog of War";
	minfo.cch = strlen(minfo.dwTypeData);
		
	if (InsertMenuItem(hMenu, 0x7FFE, 1, &minfo))
		menuID = IDS_VIEWFOG;

	if (DEFAULTS->GetDataFromRegistry(szRegKey, &fogData, sizeof(fogData)) != sizeof(fogData) || fogData.version != FOGDATA_VERSION)
	{
		fogData.version = FOGDATA_VERSION;
	/*	fogData.resolution = 15;
		fogData.red = fogData.green = 75;
		fogData.blue = 100;
		fogData.alpha = 100;
		fogData.map_red = fogData.map_green = 120;
		fogData.map_blue = 150;*/
		
		fogData.hardFog.r = 170;
		fogData.hardFog.g = 150;
		fogData.hardFog.b = 150;
		fogData.hardFog.a = 150;
		fogData.softFog.r = 100;
		fogData.softFog.g = 100;
		fogData.softFog.b = 100;
		fogData.softFog.a = 100;
		fogData.mapHardFog.r = 0;
		fogData.mapHardFog.g = 0;
		fogData.mapHardFog.b = 0;
		fogData.mapHardFog.a = 255;
		fogData.mapSoftFog.r = 100;
		fogData.mapSoftFog.g = 140;
		fogData.mapSoftFog.b = 255;
		fogData.mapSoftFog.a = 65;

	}
	fogAlpha = fogData.softFog.a/255.0;

	if (CQFLAGS.bNoGDI)
		return 1;

	DOCDESC ddesc;

	ddesc.memory = &fogData;
	ddesc.memoryLength = sizeof(fogData);

	if (DACOM->CreateInstance(&ddesc, doc) == GR_OK)
	{
		VIEWDESC vdesc;
		HWND hwnd;

		vdesc.className = "FOG_DATA";
		vdesc.doc = doc;
		vdesc.hOwnerWindow = hMainWindow;
		
		if (PARSER->CreateInstance(&vdesc, viewer) == GR_OK)
		{
			COMPTR<IDAConnectionPoint> connection;

			viewer->get_main_window((void **)&hwnd);
			MoveWindow(hwnd, 140, 140, 300, 150, 1);

			viewer->set_instance_name("Foggy fog");

			MakeConnection(doc);

			OnUpdate(doc,NULL,NULL);
		}
	}

	return (viewer != 0);
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT FogOfWar::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_SYSTEM_CHANGED:
		numZones = 0;
		bForceUpdate = true;
		break;

	case CQE_ENTERING_INGAMEMODE:
		loadTextures(true);
		RECT thumbRect;
		thumbRect.left = IDEAL2REALX(134);  
		thumbRect.right = IDEAL2REALX(213);
		thumbRect.top = IDEAL2REALY(390);
		thumbRect.bottom = IDEAL2REALY(470);
		mapX = TEXTURE_RES;//thumbRect.right-thumbRect.left;
		mapY = TEXTURE_RES;//thumbRect.bottom-thumbRect.top;
		//	mapGrid = new BOOL[mapX*mapY];
		MakeMapTextures(1);
		break;
	case CQE_LEAVING_INGAMEMODE:
		loadTextures(false);
	//	delete [] mapGrid;
	//	mapGrid = 0;
	//	free(bits);
	//	bits=0;
		for (int s=0;s<sys_count;s++)
		{
			PIPE->destroy_texture(mapTexID[s]);
			mapTexID[s] = 0;
		}
		sys_count = 0;
		
		break;
		
	case WM_COMMAND:
		if (LOWORD(msg->wParam) == menuID)
		{
			if (viewer)
				viewer->set_display_state(1);
		}
		else if (LOWORD(msg->wParam) == IDM_RESET_VISIBILITY)
		{
			for (int j=0;j<MAX_SYSTEMS;j++)
				blackFog[j].clear(0x00);
		}
		break;

	case WM_CLOSE:
		loadTextures(false);
		break;
	}

	return GR_OK;
}
//----------------------------------------------------------------------------------------------
//
void FogOfWar::MakeMapTextures(int numSystems)
{
	S32 width=TEXTURE_RES,height=TEXTURE_RES;
	
	//bits = (COLORREF *) malloc(width * height * sizeof(COLORREF));
	for (int s=sys_count;s<numSystems;s++)
	{
		COLORREF *bits = bits_array[s];
		for (int x=0;x<width;x++)
		{
			for (int y=0;y<height;y++)
			{
				bits[y*width+x] = 0xff000000;//RGB(8*x,8*y,129) | 0xff000000;
			}
		}
		
		
		if (PIPE->create_texture(width,height,PF_4CC_DAA8,1,0,mapTexID[s]) != GR_OK)
			CQBOMB2("create_texture() failed. Requested = %dx%d", width, height);
		
		PIPE->set_texture_level_data(mapTexID[s], 0, width, height, width*sizeof(COLORREF), 
			PixelFormat(PF_RGBA),
			bits, 0, 0);
	}
	
	sys_count = numSystems;

}
//----------------------------------------------------------------------------------------------
//
void FogOfWar::UpdateMapTexture(int tex)
{
	COLORREF *bits = bits_array[tex-1];
	CQASSERT(bits);
	//if (bits)
//	{
		S32 width=TEXTURE_RES,height=TEXTURE_RES;
		
		PIPE->set_texture_level_data(mapTexID[tex-1], 0, width, height, width*sizeof(COLORREF), 
			PixelFormat(PF_RGBA),
			bits, 0, 0);
//	}
}
//----------------------------------------------------------------------------------------------
//
BOOL32 FogOfWar::New()
{
	if (DEFAULTS->GetDefaults()->fogMode != FOGOWAR_NORMAL)
	{
		// set whole map to be explored
		for (int j=0;j<MAX_SYSTEMS;j++)
		{
			blackFog[j].clear(0xFF);
		}
	}
	else
	{
		for (int j=0;j<MAX_SYSTEMS;j++)
		{
			blackFog[j].clear(0x00);
		}
	}
	return 1;
}

BOOL32 __stdcall FogOfWar::Save (struct IFileSystem * outFile)
{
	COMPTR<IFileSystem> file;
	OBJPTR<ISaveLoad> pSaveLoad;
	DAFILEDESC fdesc = "\\FogOfWar";

	DWORD dwWritten;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	
	if (outFile->CreateInstance(&fdesc,file) == GR_OK)
	{
		U32 crazyPacking = MAX_PLAYERS<<24|MAX_SYSTEMS<<16|BLACKFOG_RES;
		file->WriteFile(0,&crazyPacking,sizeof(crazyPacking),&dwWritten);
		for (int s=0;s<MAX_SYSTEMS;s++)
		{
			if (blackFog[s].save(file) == 0)
				return FALSE;
		}
	}

	return TRUE;
}
//----------------------------------------------------------------------------------------------
//
BOOL32 __stdcall FogOfWar::Load (struct IFileSystem * inFile)
{
	New();

	COMPTR<IFileSystem> file;
	OBJPTR<ISaveLoad> pSaveLoad;
	DAFILEDESC fdesc = "\\FogOfWar";

	DWORD dwRead;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = OPEN_EXISTING;

	if (inFile->CreateInstance(&fdesc,file) == GR_OK)
	{
		U32 crazyPacking;
		U32 oldPlayers;
		U32 oldSystems;
		U32 oldRes;
		file->ReadFile(0,&crazyPacking,sizeof(crazyPacking),&dwRead);
		oldPlayers = crazyPacking >> 24;
		oldSystems = (crazyPacking >> 16) & 0xff;
		oldRes = crazyPacking & 0xffff;
		//	void *buffer;
		//size of a bit-packed fog buffer 
		//	S32 buff_size = ceil(oldRes/32.0)*oldRes*sizeof(U32);
		//	buffer = (void *)malloc(buff_size);
		
		if (DEFAULTS->GetDefaults()->fogMode == FOGOWAR_NORMAL)
		{
			for (U32 s=0;s<oldSystems;s++)
			{
				//	if (file->ReadFile(0,buffer,buff_size,&dwRead) == 0)
				//		return FALSE;
				
				if (s<MAX_SYSTEMS)
				{
					blackFog[s].load(file,oldRes,oldRes);
				}
			}
		}
	}

	int new_num_sys = SECTOR->GetNumSystems();
	MakeMapTextures(new_num_sys);

	
	return 1;
}
//----------------------------------------------------------------------------------------------
//
struct _warfog : GlobalComponent
{
	FogOfWar * fog;

	virtual void Startup (void)
	{
		FOGOFWAR = fog = new DAComponent<FogOfWar>;
		AddToGlobalCleanupList((IDAComponent **) &FOGOFWAR);
	}
	
	virtual void Initialize (void)
	{
		fog->Init();

		if (fog->CreateViewer() == 0)
			CQBOMB0("Viewer could not be created.");
	}
};

static _warfog warfog;

//--------------------------------------------------------------------------//
//----------------------------END FogOfWar.cpp------------------------------//
//--------------------------------------------------------------------------//
