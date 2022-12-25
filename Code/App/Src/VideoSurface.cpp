//--------------------------------------------------------------------------//
//                                                                          //
//                            VideoSurface.cpp                              //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/VideoSurface.cpp 8     11/09/00 11:17a Jasony $
*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include "globals.h"

#include "VideoSurface.h"
#include "CQTrace.h"
#include "Startup.h"

#include <HeapObj.h>
#include <TSmartPointer.h>
#include <TComponent.h>
#include <RendPipeline.h>
#include <IDDBackDoor.h>
#include <WindowManager.h>


//--------------------------------------------------------------------------//
//
static U32 bitmaskR,  bitmaskG,  bitmaskB;
static U32 bitshiftR, bitshiftG, bitshiftB;
static U32 bitwidthR, bitwidthG, bitwidthB;

//--------------------------------------------------------------------------//
//
struct VideoSurface : public IVideoSurface
{
	BEGIN_DACOM_MAP_INBOUND(VideoSurface)
	DACOM_INTERFACE_ENTRY(IVideoSurface)
	END_DACOM_MAP()

	// pixel format 
	RPLOCKDATA lockData;
	
	//
	// VFX_WINDOW representing display surface
	//
	
	VFX_WINDOW window;
	
	//
	// VFX pane representing entire display surface
	//
	
	PANE pane;
	
	//
	// 'invalidAddress' points to a region of address space which is 
	// guaranteed to cause an exception when either a read or write access is attempted.
	// (This is good for debugging.)
	//
	void * invalidAddress;
	S32 savedWindowPitch;

	int frameLockCount;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	
	VideoSurface (void)
	{
	}
	
    ~VideoSurface (void)
    {
		if (invalidAddress)
		{
			VirtualFree(invalidAddress, 0, MEM_RELEASE);
			invalidAddress = 0;
		}
	}
	
	void init (void);

	/* IVideoSurface methods */

	virtual const VFX_WINDOW & GetWindow (void);

	virtual const PANE & GetPane (void);

	virtual bool  Lock (void);
	
	virtual void  Unlock (void);

	virtual bool LockFrontBuffer (void);

	virtual void UnlockFrontBuffer (void);

	virtual bool IsLocked (void)
	{
		return (frameLockCount>0);
	}

	/* VideoSurface methods */

	void invalidateWindowBuffer (void);

	static GENRESULT getPrimarySurface (IDirectDrawSurface7 ** ppSurface);

	IDAComponent * getBase (void)
	{
		return static_cast<IVideoSurface *> (this);
	}
	
};
//---------------------------------------------------------------------
//
const VFX_WINDOW & VideoSurface::GetWindow (void)
{
	return window;
}
//---------------------------------------------------------------------
//
const PANE & VideoSurface::GetPane (void)
{
	return pane;
}
//---------------------------------------------------------------------
//
bool VideoSurface::Lock (void)
{
	//
	// Increment lock count and return if already locked
	//
	
	if (frameLockCount++)
	{
		return true;
	}

	if (PIPE->lock_buffer(&lockData) == GR_OK)
	{
		window.buffer = lockData.pixels;
		window.x_max = (lockData.pitch / 2) - 1;
		window.y_max = lockData.height - 1;

		window.R_mask = bitmaskR = lockData.pf.ddpf.dwRBitMask;
		window.G_mask = bitmaskG = lockData.pf.ddpf.dwGBitMask;
		window.B_mask = bitmaskB = lockData.pf.ddpf.dwBBitMask;
  
		bitshiftR = lockData.pf.rl;
		bitshiftG = lockData.pf.gl;
		bitshiftB = lockData.pf.bl;

		bitwidthR = lockData.pf.rwidth;
		bitwidthG = lockData.pf.gwidth;
		bitwidthB = lockData.pf.bwidth;

		pane.x0 = pane.y0 = 0;
		pane.x1 = SCREENRESX-1;
		pane.y1 = SCREENRESY-1;

		return true;
	}
	else
	{
		frameLockCount--;		// undo increment above
		return false;
	}
}
//---------------------------------------------------------------------
//
void VideoSurface::Unlock (void)
{
	//
	// Decrement lock count and return if still locked
	//
	
	if (--frameLockCount > 0)
	{
		return;
	}
	
	PIPE->unlock_buffer();

	invalidateWindowBuffer();
}
//-------------------------------------------------
//
GENRESULT VideoSurface::getPrimarySurface (IDirectDrawSurface7 ** ppSurface)
{
	COMPTR<IDDBackDoor> pBackDoor;
	COMPTR<IUnknown> pDD1;
	HRESULT hr;
	GENRESULT result = GR_GENERIC;

	*ppSurface = 0;

	hr = PIPE->QueryInterface(IID_IDDBackDoor, pBackDoor);
	if (hr != GR_OK)
		goto Done;
	hr = pBackDoor->get_dd_provider(DDBD_P_PRIMARYSURFACE, pDD1);
	if (hr != GR_OK)
		goto Done;
	hr = pDD1->QueryInterface(IID_IDirectDrawSurface7, (void **) ppSurface);
	if (hr != GR_OK)
		goto Done;
	
	result = GR_OK;

Done:
	return result;
}
//-------------------------------------------------
//
bool VideoSurface::LockFrontBuffer (void)
{
	COMPTR<IDirectDrawSurface7> pPrimary;
	bool result = false;
	WM_WINAREA area;
	WM->GetClientArea(area);

	CQASSERT(frameLockCount==0);	// can't lock both at once

	if (getPrimarySurface(pPrimary) == GR_OK)
	{
		DDSURFACEDESC2 ddsd2;

		memset( &ddsd2, 0, sizeof(ddsd2) );
		ddsd2.dwSize = sizeof(ddsd2);

		if( SUCCEEDED( pPrimary->Lock( NULL, &ddsd2, DDLOCK_WAIT, 0 ) ) ) 
		{
			lockData.pixels = ddsd2.lpSurface;
			lockData.width = ddsd2.dwWidth;
			lockData.pitch = ddsd2.lPitch;
			lockData.height = ddsd2.dwHeight;
			lockData.pf.init( ddsd2.ddpfPixelFormat );

			window.R_mask = bitmaskR = lockData.pf.ddpf.dwRBitMask;
			window.G_mask = bitmaskG = lockData.pf.ddpf.dwGBitMask;
			window.B_mask = bitmaskB = lockData.pf.ddpf.dwBBitMask;
			window.pixel_pitch = window.bytes_per_pixel = (lockData.pf.ddpf.dwRGBBitCount + 7) / 8;
  
			window.buffer = lockData.pixels;
			window.x_max = (lockData.pitch / window.pixel_pitch) - 1;
			window.y_max = lockData.height - 1;

			bitshiftR = lockData.pf.rl;
			bitshiftG = lockData.pf.gl;
			bitshiftB = lockData.pf.bl;

			bitwidthR = lockData.pf.rwidth;
			bitwidthG = lockData.pf.gwidth;
			bitwidthB = lockData.pf.bwidth;

			pane.x0 = area.x;
			pane.x1 = area.x + area.w - 1;
			pane.y0 = area.y;
			pane.y1 = area.y + area.h - 1;

			result = true;
		}
	}

	return result;
}
//-------------------------------------------------
//
void VideoSurface::UnlockFrontBuffer (void)
{
	COMPTR<IDirectDrawSurface7> pPrimary;

	CQASSERT(frameLockCount==0);	// can't lock both at once

	if (getPrimarySurface(pPrimary) == GR_OK)
	{
		pPrimary->Unlock( NULL );

		pane.x0 = pane.y0 = 0;		// set these back to default for safety
		pane.x1 = SCREENRESX-1;
		pane.y1 = SCREENRESY-1;
		window.pixel_pitch = window.bytes_per_pixel = 2;

		invalidateWindowBuffer();
	}
}
//-------------------------------------------------
//
void VideoSurface::invalidateWindowBuffer (void)
{
	//
	// make sure our invalid buffer is large enough
	//
	
	if (savedWindowPitch != window.x_max+1)
	{
		if (invalidAddress)
			VirtualFree(invalidAddress, 0, MEM_RELEASE);
		invalidAddress = VirtualAlloc(0, (window.x_max+1)*(window.y_max+1)*window.bytes_per_pixel, MEM_RESERVE, PAGE_NOACCESS); 
		savedWindowPitch = window.x_max+1;
	}
	
	//
	// Invalidate window buffer pointer to help diagnose 
	// write-without-lock errors
	//
	
	window.buffer = invalidAddress;
}
//-------------------------------------------------
//
static S32 getDesktopFormat (void)
{
	HDC      hdc;
	COLORREF color,save;
	S32      bpp;
	S32		 i;

	
	hdc = GetDC(0);
	
	//
	// Get # of bits per pixel
	//
	
	bpp = GetDeviceCaps(hdc, BITSPIXEL) * 
		GetDeviceCaps(hdc, PLANES);
	
	//
	// Preserve upper-left pixel value
	//
	
	save = GetPixel(hdc,0,0);
	
	//
	// Write pixel value in 8-8-8 format with 5th bit from MSB = 1
	// >5-bit RGB components will read back identically
	//  5-bit RGB components will read back 0
	//
	// (JM: This would seem to be off by one, in that a 5-bit color
	// component should preserve the 5th-order bit, but Windows does not
	// appear to work that way...)
	//
	
	SetPixel(hdc,0,0,RGB(0x08,0x08,0x08));
	
	//
	// Retrieve pixel in 8-8-8 format, and look for position of '1' 
	// bit in component fields
	//
	
	color = GetPixel(hdc,0,0) & 0xffffff;
	
	switch (color)
	{
		//
		// 0x000000 = 5-5-5
		//
		
	case 0x000000:
		bitmaskR = 0x007c00;
		bitmaskG = 0x0003e0;
		bitmaskB = 0x00001f;
		break;
		
		//
		// 0x000800 = 5-6-5
		//
		
	case 0x000800:
		bitmaskR = 0x00f800;
		bitmaskG = 0x0007e0;
		bitmaskB = 0x00001f;
		break;
		
		//
		// 0x080808 = 8-8-8
		//
		
	case 0x080808:
		bitmaskR = 0xff0000;
		bitmaskG = 0x00ff00;
		bitmaskB = 0x0000ff;
		break;
		
	default:
		if ((bpp == 15) || (bpp == 16))
		{
            bitmaskR = 0x00f800;
            bitmaskG = 0x0007e0;
            bitmaskB = 0x00001f;
		}
		else
		{
            bitmaskR = 0xff0000;
            bitmaskG = 0x00ff00;
            bitmaskB = 0x0000ff;
		}
		break;
	}
	
	//
	// Restore initial pixel value
	//
	
	SetPixel(hdc,0,0,save);
	ReleaseDC(0, hdc);


	//
	// Derive shift, width values from masks
	//
	
	for (i=31; i >= 0; i--)
	{
		if (bitmaskR & (1 << i))
		{
			bitshiftR = i;
		}
		
		if (bitmaskG & (1 << i))
		{
			bitshiftG = i;
		}
		
		if (bitmaskB & (1 << i))
		{
			bitshiftB = i;
		}
	}
	
	for (i=0; i <= 31; i++)
	{
		if (bitmaskR & (1 << i))
		{
			bitwidthR = i - bitshiftR + 1;
		}
		
		if (bitmaskG & (1 << i))
		{
			bitwidthG = i - bitshiftG + 1;
		}
		
		if (bitmaskB & (1 << i))
		{
			bitwidthB = i - bitshiftB + 1;
		}
	}

	return bpp;
}
//---------------------------------------------------------------------
//
void VideoSurface::init (void)
{
	//
	// initialize pane and window structures
	// 

	window.x_max = SCREEN_WIDTH-1;
	window.y_max = SCREEN_HEIGHT-1;
	window.pixel_pitch = 2;           // # of bytes between adjacent pixels
	window.bytes_per_pixel = 2;       // # of bytes to write per pixel

	pane.window = &window;
	pane.x1 = SCREEN_WIDTH-1;
	pane.y1 = SCREEN_HEIGHT-1;

	//
	// set up an invalid region of memory to catch invalid writes to the window
	//
	if (invalidAddress)
	{
		VirtualFree(invalidAddress, 0, MEM_RELEASE);
		invalidAddress = 0;
	}
	window.buffer = invalidAddress = VirtualAlloc(0, SCREEN_WIDTH*SCREEN_HEIGHT*2, MEM_RESERVE, PAGE_NOACCESS); 
	savedWindowPitch = SCREEN_WIDTH;

	getDesktopFormat();


}
//---------------------------------------------------------------------
//
COLORREF __fastcall PixelToColorRef (U32 pixel)
{
	COLORREF result;

	result = (((pixel & bitmaskR) >> bitshiftR) << (8-bitwidthR)) |
			 (((pixel & bitmaskG) >> bitshiftG) << (16-bitwidthG)) |
			 (((pixel & bitmaskB) >> bitshiftB) << (24-bitwidthB));

	return result;
}
//---------------------------------------------------------------------
//												 
U32 __fastcall ColorRefToPixel (COLORREF colorref)
{
	U32 result;

	result = (((colorref & 0xFF)     >> (8-bitwidthR))  << bitshiftR) |
			 (((colorref & 0xFF00)   >> (16-bitwidthG)) << bitshiftG) |
			 (((colorref & 0xFF0000) >> (24-bitwidthB)) << bitshiftB);

	return result;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _video : GlobalComponent
{

	virtual void Startup (void)
	{
		VideoSurface * video;

		SURFACE = video = new DAComponent<VideoSurface>;
		AddToGlobalCleanupList((IDAComponent **) &SURFACE);

		video->init();

	}

	virtual void Initialize (void)
	{
	}
};

static _video surface;

//--------------------------------------------------------------------------//
//-----------------------------End VideoSurface.cpp-------------------------//
//--------------------------------------------------------------------------//
