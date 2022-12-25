//---------------------------------------------------------------------------
/*
	DRAW.CPP

	Copyright (C) 1997 Digital Anvil, Inc.

	Created: November 1997

	Authors: Paul Isaac & Bill Baldwin
*/
//---------------------------------------------------------------------------

#define DIRECTDRAW_VERSION 0x0500	// Ensure we can run with DDraw 5 and up

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define INITGUID
#include "draw.h"

#include <d3d.h>

#define DDRAW_DYNAMIC 1				// Non-zero to load/unload DDRAW.DLL dynamically

void DebugPrint (char *fmt, ...);
void DebugAlert (char *title, char *fmt, ...);
bool DebugAlertYesNo(char * title, char * fmt, ...);

#define RELEASE(x)			if(x) {(x)->Release();(x)=0;}

//---------------------------------------------------------------------------
// GLOBALS
//---------------------------------------------------------------------------

LPDIRECTDRAW2			lpDD = NULL;
LPDIRECT3DDEVICE2		D3DDevice = NULL;

#ifdef RSTATE_CHECK
DWORD					*render_states=NULL;    // state tracking of global d3d device
#endif

extern int Verbosity;
#define VERBOSE if(Verbosity)
//#define VERBOSE if(false)

PixelFormat screen_pixel_format;

DDCAPS DrawMgr::hw_caps;
DDCAPS DrawMgr::hel_caps;

//---------------------------------------------------------------------------
// DEBUG
//---------------------------------------------------------------------------

char *DD_message (HRESULT error)
{
   static char buff[128];

   switch (error) 
      {
      case DD_OK:
          return "No error.\0";
      case DDERR_ALREADYINITIALIZED:
          return "This object is already initialized.\0";
      case DDERR_BLTFASTCANTCLIP:
          return "Return if a clipper object is attached to the source surface passed into a BltFast call.\0";
      case DDERR_CANNOTATTACHSURFACE:
          return "This surface can not be attached to the requested surface.\0";
      case DDERR_CANNOTDETACHSURFACE:
          return "This surface can not be detached from the requested surface.\0";
      case DDERR_CANTCREATEDC:
          return "Windows can not create any more DCs.\0";
      case DDERR_CANTDUPLICATE:
          return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.\0";
      case DDERR_CLIPPERISUSINGHWND:
          return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.\0";
      case DDERR_COLORKEYNOTSET:
          return "No src color key specified for this operation.\0";
      case DDERR_CURRENTLYNOTAVAIL:
          return "Support is currently not available.\0";
      case DDERR_DIRECTDRAWALREADYCREATED:
          return "A DirectDraw object representing this driver has already been created for this process.\0";
      case DDERR_EXCEPTION:
          return "An exception was encountered while performing the requested operation.\0";
      case DDERR_EXCLUSIVEMODEALREADYSET:
          return "An attempt was made to set the cooperative level when it was already set to exclusive.\0";
      case DDERR_GENERIC:
          return "Generic failure.\0";
      case DDERR_HEIGHTALIGN:
          return "Height of rectangle provided is not a multiple of reqd alignment.\0";
      case DDERR_HWNDALREADYSET:
          return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.\0";
      case DDERR_HWNDSUBCLASSED:
          return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.\0";
      case DDERR_IMPLICITLYCREATED:
          return "This surface can not be restored because it is an implicitly created surface.\0";
      case DDERR_INCOMPATIBLEPRIMARY:
          return "Unable to match primary surface creation request with existing primary surface.\0";
      case DDERR_INVALIDCAPS:
          return "One or more of the caps bits passed to the callback are incorrect.\0";
      case DDERR_INVALIDCLIPLIST:
          return "DirectDraw does not support the provided cliplist.\0";
      case DDERR_INVALIDDIRECTDRAWGUID:
          return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.\0";
      case DDERR_INVALIDMODE:
          return "DirectDraw does not support the requested mode.\0";
      case DDERR_INVALIDOBJECT:
          return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.\0";
      case DDERR_INVALIDPARAMS:
          return "One or more of the parameters passed to the function are incorrect.\0";
      case DDERR_INVALIDPIXELFORMAT:
          return "The pixel format was invalid as specified.\0";
      case DDERR_INVALIDPOSITION:
          return "Returned when the position of the overlay on the destination is no longer legal for that destination.\0";
      case DDERR_INVALIDRECT:
          return "Rectangle provided was invalid.\0";
      case DDERR_LOCKEDSURFACES:
          return "Operation could not be carried out because one or more surfaces are locked.\0";
      case DDERR_NO3D:
          return "There is no 3D present.\0";
      case DDERR_NOALPHAHW:
          return "Operation could not be carried out because there is no alpha accleration hardware present or available.\0";
      case DDERR_NOBLTHW:
          return "No blitter hardware present.\0";
      case DDERR_NOCLIPLIST:
          return "No cliplist available.\0";
      case DDERR_NOCLIPPERATTACHED:
          return "No clipper object attached to surface object.\0";
      case DDERR_NOCOLORCONVHW:
          return "Operation could not be carried out because there is no color conversion hardware present or available.\0";
      case DDERR_NOCOLORKEY:
          return "Surface doesn't currently have a color key\0";
      case DDERR_NOCOLORKEYHW:
          return "Operation could not be carried out because there is no hardware support of the destination color key.\0";
      case DDERR_NOCOOPERATIVELEVELSET:
          return "Create function called without DirectDraw object method SetCooperativeLevel being called.\0";
      case DDERR_NODC:
          return "No DC was ever created for this surface.\0";
      case DDERR_NODDROPSHW:
          return "No DirectDraw ROP hardware.\0";
      case DDERR_NODIRECTDRAWHW:
          return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.\0";
      case DDERR_NOEMULATION:
          return "Software emulation not available.\0";
      case DDERR_NOEXCLUSIVEMODE:
          return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.\0";
      case DDERR_NOFLIPHW:
          return "Flipping visible surfaces is not supported.\0";
      case DDERR_NOGDI:
          return "There is no GDI present.\0";
      case DDERR_NOHWND:
          return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.\0";
	  case DDERR_NOMIPMAPHW:
		  return "Hardware doesn't support mipmapping.\0";
      case DDERR_NOMIRRORHW:
          return "Operation could not be carried out because there is no hardware present or available.\0";
      case DDERR_NOOVERLAYDEST:
          return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.\0";
      case DDERR_NOOVERLAYHW:
          return "Operation could not be carried out because there is no overlay hardware present or available.\0";
      case DDERR_NOPALETTEATTACHED:
          return "No palette object attached to this surface.\0";
      case DDERR_NOPALETTEHW:
          return "No hardware support for 16 or 256 color palettes.\0";
      case DDERR_NORASTEROPHW:
          return "Operation could not be carried out because there is no appropriate raster op hardware present or available.\0";
      case DDERR_NOROTATIONHW:
          return "Operation could not be carried out because there is no rotation hardware present or available.\0";
      case DDERR_NOSTRETCHHW:
          return "Operation could not be carried out because there is no hardware support for stretching.\0";
      case DDERR_NOT4BITCOLOR:
          return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.\0";
      case DDERR_NOT4BITCOLORINDEX:
          return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.\0";
      case DDERR_NOT8BITCOLOR:
          return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.\0";
      case DDERR_NOTAOVERLAYSURFACE:
          return "Returned when an overlay member is called for a non-overlay surface.\0";
      case DDERR_NOTEXTUREHW:
          return "Operation could not be carried out because there is no texture mapping hardware present or available.\0";
      case DDERR_NOTFLIPPABLE:
          return "An attempt has been made to flip a surface that is not flippable.\0";
      case DDERR_NOTFOUND:
          return "Requested item was not found.\0";
      case DDERR_NOTLOCKED:
          return "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.\0";
      case DDERR_NOTPALETTIZED:
          return "The surface being used is not a palette-based surface.\0";
      case DDERR_NOVSYNCHW:
          return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.\0";
      case DDERR_NOZBUFFERHW:
          return "Operation could not be carried out because there is no hardware support for zbuffer blitting.\0";
      case DDERR_NOZOVERLAYHW:
          return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.\0";
      case DDERR_OUTOFCAPS:
          return "The hardware needed for the requested operation has already been allocated.\0";
      case DDERR_OUTOFMEMORY:
          return "DirectDraw does not have enough memory to perform the operation.\0";
      case DDERR_OUTOFVIDEOMEMORY:
          return "DirectDraw does not have enough memory to perform the operation.\0";
      case DDERR_OVERLAYCANTCLIP:
          return "The hardware does not support clipped overlays.\0";
      case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
          return "Can only have ony color key active at one time for overlays.\0";
      case DDERR_OVERLAYNOTVISIBLE:
          return "Returned when GetOverlayPosition is called on a hidden overlay.\0";
      case DDERR_PALETTEBUSY:
          return "Access to this palette is being refused because the palette is already locked by another thread.\0";
      case DDERR_PRIMARYSURFACEALREADYEXISTS:
          return "This process already has created a primary surface.\0";
      case DDERR_REGIONTOOSMALL:
          return "Region passed to Clipper::GetClipList is too small.\0";
      case DDERR_SURFACEALREADYATTACHED:
          return "This surface is already attached to the surface it is being attached to.\0";
      case DDERR_SURFACEALREADYDEPENDENT:
          return "This surface is already a dependency of the surface it is being made a dependency of.\0";
      case DDERR_SURFACEBUSY:
          return "Access to this surface is being refused because the surface is already locked by another thread.\0";
      case DDERR_SURFACEISOBSCURED:
          return "Access to surface refused because the surface is obscured.\0";
      case DDERR_SURFACELOST:
          return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.\0";
      case DDERR_SURFACENOTATTACHED:
          return "The requested surface is not attached.\0";
      case DDERR_TOOBIGHEIGHT:
          return "Height requested by DirectDraw is too large.\0";
      case DDERR_TOOBIGSIZE:
          return "Size requested by DirectDraw is too large, but the individual height and width are OK.\0";
      case DDERR_TOOBIGWIDTH:
          return "Width requested by DirectDraw is too large.\0";
      case DDERR_UNSUPPORTED:
          return "Action not supported.\0";
      case DDERR_UNSUPPORTEDFORMAT:
          return "FOURCC format requested is unsupported by DirectDraw.\0";
      case DDERR_UNSUPPORTEDMASK:
          return "Bitmask in the pixel format requested is unsupported by DirectDraw.\0";
      case DDERR_VERTICALBLANKINPROGRESS:
          return "Vertical blank is in progress.\0";
      case DDERR_WASSTILLDRAWING:
          return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.\0";
      case DDERR_WRONGMODE:
          return "This surface can not be restored because it was created in a different mode.\0";
      case DDERR_XALIGN:
          return "Rectangle provided was not horizontally aligned on required boundary.\0";
      case D3DERR_BADMAJORVERSION:
          return "D3DERR_BADMAJORVERSION\0";
      case D3DERR_BADMINORVERSION:
          return "D3DERR_BADMINORVERSION\0";
      case D3DERR_EXECUTE_LOCKED:
          return "D3DERR_EXECUTE_LOCKED\0";
      case D3DERR_EXECUTE_NOT_LOCKED:
          return "D3DERR_EXECUTE_NOT_LOCKED\0";
      case D3DERR_EXECUTE_CREATE_FAILED:
          return "D3DERR_EXECUTE_CREATE_FAILED\0";
      case D3DERR_EXECUTE_DESTROY_FAILED:
          return "D3DERR_EXECUTE_DESTROY_FAILED\0";
      case D3DERR_EXECUTE_LOCK_FAILED:
          return "D3DERR_EXECUTE_LOCK_FAILED\0";
      case D3DERR_EXECUTE_UNLOCK_FAILED:
          return "D3DERR_EXECUTE_UNLOCK_FAILED\0";
      case D3DERR_EXECUTE_FAILED:
          return "D3DERR_EXECUTE_FAILED\0";
      case D3DERR_EXECUTE_CLIPPED_FAILED:
          return "D3DERR_EXECUTE_CLIPPED_FAILED\0";
      case D3DERR_TEXTURE_NO_SUPPORT:
          return "D3DERR_TEXTURE_NO_SUPPORT\0";
      case D3DERR_TEXTURE_NOT_LOCKED:
          return "D3DERR_TEXTURE_NOT_LOCKED\0";
      case D3DERR_TEXTURE_LOCKED:
          return "D3DERR_TEXTURELOCKED\0";
      case D3DERR_TEXTURE_CREATE_FAILED:
          return "D3DERR_TEXTURE_CREATE_FAILED\0";
      case D3DERR_TEXTURE_DESTROY_FAILED:
          return "D3DERR_TEXTURE_DESTROY_FAILED\0";
      case D3DERR_TEXTURE_LOCK_FAILED:
          return "D3DERR_TEXTURE_LOCK_FAILED\0";
      case D3DERR_TEXTURE_UNLOCK_FAILED:
          return "D3DERR_TEXTURE_UNLOCK_FAILED\0";
      case D3DERR_TEXTURE_LOAD_FAILED:
          return "D3DERR_TEXTURE_LOAD_FAILED\0";
      case D3DERR_MATRIX_CREATE_FAILED:
          return "D3DERR_MATRIX_CREATE_FAILED\0";
      case D3DERR_MATRIX_DESTROY_FAILED:
          return "D3DERR_MATRIX_DESTROY_FAILED\0";
      case D3DERR_MATRIX_SETDATA_FAILED:
          return "D3DERR_MATRIX_SETDATA_FAILED\0";
      case D3DERR_SETVIEWPORTDATA_FAILED:
          return "D3DERR_SETVIEWPORTDATA_FAILED\0";
      case D3DERR_MATERIAL_CREATE_FAILED:
          return "D3DERR_MATERIAL_CREATE_FAILED\0";
      case D3DERR_MATERIAL_DESTROY_FAILED:
          return "D3DERR_MATERIAL_DESTROY_FAILED\0";
      case D3DERR_MATERIAL_SETDATA_FAILED:
          return "D3DERR_MATERIAL_SETDATA_FAILED\0";
      case D3DERR_LIGHT_SET_FAILED:
          return "D3DERR_LIGHT_SET_FAILED\0";
      case D3DERR_ZBUFF_NEEDS_VIDEOMEMORY:
          return "D3DERR_ZBUFF_NEEDS_VIDEOMEMORY\0";
      default:
          wsprintf(buff,"Unrecognized DirectX error=%X\0",error);
          return buff;
      }
}

//---------------------------------------------------------------------------
// DrawMgr
//---------------------------------------------------------------------------

DrawMgr::DrawMgr(void)
{
	active_context = 0;

	DDraw_active = false;
	DDraw_lib_handle = 0;
	lock_flags = 0;

	fullscreen = false;

	desktop_wnd = 0;
	desktop_w = desktop_h = desktop_bpp = 0;

	num_formats = 0;
	current_format = 0;
}

//****************************************************************************
//*                                                                          *
//*  Explicitly load / unload DDRAW.DLL                                      *
//*                                                                          *
//****************************************************************************

#if DDRAW_DYNAMIC

static HRESULT (WINAPI *_DirectDrawCreate) (GUID         FAR *lpGUID, 
                                            LPDIRECTDRAW FAR *lplpDD, 
                                            IUnknown     FAR *pUnkOuter) = 0;

static void UnloadDDrawLibrary (HINSTANCE hDDLibrary)
{
	if (hDDLibrary)
	{
		_DirectDrawCreate = NULL;
		FreeLibrary(hDDLibrary);
	}
}

static BOOL LoadDDrawLibrary (HINSTANCE *hDDLibrary)
{
	HMODULE hModule;
	char      szLibName[]= "ddraw.dll";

	if ((*hDDLibrary = LoadLibrary(szLibName)) != NULL)
	{
		if ((hModule = GetModuleHandle(szLibName)) != NULL)
		{
			_DirectDrawCreate = (long (__stdcall *)(struct _GUID *,struct IDirectDraw ** ,struct IUnknown *))
			GetProcAddress(hModule, "DirectDrawCreate");

			return (_DirectDrawCreate != NULL);
		}
	}

	return FALSE;
}

#endif


HRESULT CALLBACK TF_enumerate(DDSURFACEDESC *DeviceFmt, LPVOID lParam)
{
	DDPIXELFORMAT ddpf = DeviceFmt->ddpfPixelFormat;

	VERBOSE DebugPrint("   %2dbpp %s%s%s %08X %08X %08X %08X\n",
		ddpf.dwRGBBitCount,
		(ddpf.dwFlags & (DDPF_RGB)) ? "RGB" : "   ",
		(ddpf.dwFlags & (DDPF_ALPHAPIXELS)) ? "A " : "  ",
		(ddpf.dwFlags & (DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXED4)) ? "PAL " : "    ",
		ddpf.dwRBitMask,
		ddpf.dwGBitMask,
		ddpf.dwBBitMask,
		ddpf.dwRGBAlphaBitMask);

//
// Add this format's pixel descriptor to the list
// 
	texture_formats[texture_format_cnt++].init(ddpf);

//
// Signal request for next format
//
	return DDENUMRET_OK;
}

//****************************************************************************
//*                                                                          *
//*  DirectDraw device enumeration callbacks attempt to locate a             *
//*  device which supports 3D acceleration                                   *
//*                                                                          *
//*  DD1_enumerate() attempts to find a DirectDraw 3D device which is not    *
//*  the same as the default GDI card.  This handles the case of an add-in   *
//*  3D card (e.g., 3Dfx-based) coupled with a non-game-compatible 3D        *
//*  device such as the Matrox Millennium.                                   *
//*                                                                          *
//*  DD2_enumerate() is called if Phase1_enumerate() fails, and accepts      *
//*  DirectDraw drivers associated with the primary display device.  This    *
//*  handles Voodoo Rush and similar cards.                                  *
//*                                                                          *
//*  If both enumeration functions fail, the system contains no 3D hardware  *
//*  at all.                                                                 *
//*                                                                          *
//*  TBD: Allow driver selection based on name stored in registry as         *
//*  user preference!                                                        *
//*                                                                          *
//****************************************************************************

BOOL DD_enumerate (GUID *lpGUID, LPSTR szName, LPSTR szDevice, LPVOID lParam)
{
   HRESULT ddrval;

//
// Create an instance of the driver
//
   LPDIRECTDRAW DD;

#if DDRAW_DYNAMIC
	if (_DirectDrawCreate)   
	{
		ddrval = _DirectDrawCreate(lpGUID, &DD, NULL);
	}
	else
		return DDENUMRET_OK;
#else
	ddrval = DirectDrawCreate(lpGUID, &DD, NULL);
#endif

	if (ddrval != DD_OK) 
	{
		DD = NULL;
		return DDENUMRET_OK;
	}

	if (DD->QueryInterface(IID_IDirectDraw2, (void **) &lpDD) != DD_OK)
	{
		RELEASE(DD);
		lpDD = NULL;
		return DDENUMRET_OK;
	}

//
// See if it supports hardware rendering
//
	memset(&DrawMgr::hw_caps, 0, sizeof(DDCAPS));
	DrawMgr::hw_caps.dwSize = sizeof(DDCAPS);

	memset(&DrawMgr::hel_caps, 0, sizeof(DDCAPS));
	DrawMgr::hel_caps.dwSize = sizeof(DDCAPS);

	if (lpDD->GetCaps(&DrawMgr::hw_caps, &DrawMgr::hel_caps) != DD_OK)
	{
		RELEASE(lpDD);
		return DDENUMRET_OK;
	}

	if (DrawMgr::hw_caps.dwCaps & DDCAPS_3D)
	{
		VERBOSE DebugPrint("DD <= %X '%s' '%s'\n", lpGUID, szName, szDevice);
		return DDENUMRET_CANCEL;
	}

//
// Driver does not support 3D rendering -- release it and continue
// with enumeration process
//
	RELEASE(lpDD);
	return DDENUMRET_OK;
}

BOOL CALLBACK DD1_enumerate(GUID *lpGUID, LPSTR szName, LPSTR szDevice, LPVOID lParam)
{
	VERBOSE DebugPrint("DD1 %X '%s' '%s'\n", lpGUID, szName, szDevice);   

	if (lpGUID == NULL)
	{
		return DDENUMRET_OK;	// Reject drivers based on the primary display device
	}

	return DD_enumerate(lpGUID,szName,szDevice,lParam);
}


BOOL CALLBACK DD2_enumerate(GUID *lpGUID, LPSTR szName, LPSTR szDevice, LPVOID lParam)
{
	VERBOSE DebugPrint("DD2 %X '%s' '%s'\n", lpGUID, szName, szDevice);

	if (lpGUID != NULL)
	{
		return DDENUMRET_OK;	// Reject drivers NOT based on the primary display device
	}

	return DD_enumerate(lpGUID,szName,szDevice,lParam);
}

//---------------------------------------------------------------------------

HRESULT CALLBACK DM_enumerate (DDSURFACEDESC *DeviceFmt, LPVOID lpContext)
{
	DDPIXELFORMAT ddpf = DeviceFmt->ddpfPixelFormat;

	VERBOSE DebugPrint("   %4dx%4d %2dbpp",
		DeviceFmt->dwWidth,
		DeviceFmt->dwHeight,
		ddpf.dwRGBBitCount);

	VERBOSE DebugPrint("  %s%s%s %08X %08X %08X %08X\n",
		(ddpf.dwFlags & (DDPF_RGB)) ? "RGB" : "   ",
		(ddpf.dwFlags & (DDPF_ALPHAPIXELS)) ? "A " : "  ",
		(ddpf.dwFlags & (DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXED4)) ? "PAL " : "    ",
		ddpf.dwRBitMask,
		ddpf.dwGBitMask,
		ddpf.dwBBitMask,
		ddpf.dwRGBAlphaBitMask);

	return DDENUMRET_OK;
}

//---------------------------------------------------------------------------
//
// STARTUP/SHUTDOWN
//
//---------------------------------------------------------------------------

bool DrawMgr::startup (void)
{
	if (DDraw_active)
		return false; // must shutdown before re-starting?

#if DDRAW_DYNAMIC
	if (DDraw_lib_handle == NULL)
	{
		if (LoadDDrawLibrary(&DDraw_lib_handle) == FALSE)
		{
			goto done;
		}
	}
#endif

//
// Attempt to create DirectDraw 3D object based on non-primary display
// device
//
	DirectDrawEnumerate(DD1_enumerate, NULL);

	if (lpDD == NULL)
	{
	//
	// 1st-pass enumeration failed: before giving up, try to create a 
	// DirectDraw 3D object based on the primary display device
	//
		DirectDrawEnumerate(DD2_enumerate, NULL);
	}
done:
	if (lpDD == NULL)
	{
		DebugAlert(NULL,"No DirectDraw-compatible 3D accelerators found!\n"); // DirectDraw 5?
		return false;
	}

	DDSCAPS caps;
	caps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
	DWORD vtotal, vfree;
	lpDD->GetAvailableVidMem(&caps, &vtotal, &vfree);

	caps.dwCaps |= DDSCAPS_LOCALVIDMEM;
	lpDD->GetAvailableVidMem(&caps, &vtotal, &vfree);

//
// Enumerate Display Modes
//
	VERBOSE DebugPrint("Display Modes:\n");
	HRESULT result = lpDD->EnumDisplayModes(DDEDM_REFRESHRATES,NULL,0,DM_enumerate);

//
// Try to avoid taking Win16 lock when locking surface
//
// 3DFx guys claim that specifying WRITEONLY can speed up performance
// on Voodoo cards. Of course if you need to read the buffer you're screweed.
//
	lock_flags = DDLOCK_NOSYSLOCK | DDLOCK_WRITEONLY;

//
// Set global flag to indicate DirectDraw is active
// 
	DDraw_active = true;

	result = lpDD->SetCooperativeLevel(NULL, DDSCL_NORMAL);

	if (result != DD_OK)
	{
		DebugAlert(NULL,"DD->SetCooperativeLevel() failed, code %X\n",result);
		return false;
	}

// FUTURE: enumerate display modes?

	num_formats = 0;

	PIXELFORMATDESCRIPTOR *pf = pixel_formats;

	if (pf)
	{
		num_formats++;

		memset(pf,0,sizeof(PIXELFORMATDESCRIPTOR));

		pf->nSize			= sizeof(PIXELFORMATDESCRIPTOR);
		pf->nVersion		= 1;
		pf->dwFlags			= PFD_SUPPORT_OPENGL; 
		pf->iPixelType		= PFD_TYPE_RGBA; 

		pf->cColorBits		= 16; 
		pf->cRedBits		= 5;	pf->cRedShift	=  0; 
		pf->cGreenBits		= 6;	pf->cGreenShift	=  0; 
        pf->cBlueBits		= 5;	pf->cBlueShift	=  0; 
        pf->cAlphaBits		= 0;	pf->cAlphaShift =  0; 

        pf->cAccumBits		= 0;
        pf->cDepthBits		= 16; 
        pf->cStencilBits	= 0; 
	}

	return true; // Success!
}

//---------------------------------------------------------------------------

void DrawMgr::shutdown (void)
{
	if (DDraw_active)
	{
/*
	//
	// Free all DirectDraw surfaces
	//
		while (num_contexts > 0)
		{
			DrawContext *ctx = context[0];
			free_surfaces(ctx);
			remove_context(ctx);
		}
		num_contexts = 0;
*/
	//
	// Restore Window to normal and free DirectDraw object
	//
		if (lpDD)
		{
			RELEASE(lpDD);
		}

	#if DDRAW_DYNAMIC
		if (DDraw_lib_handle)
		{
			UnloadDDrawLibrary(DDraw_lib_handle);
			DDraw_lib_handle = NULL;
		}
	#endif

	//
	// Clear DirectDraw status flag
	//
		DDraw_active = 0;
	}
}

void DrawMgr::set_fullscreen(bool full)
{
	if (!active_context) return;

	if (full && !fullscreen)
	{
	// switching to fullscreen.
		free_surfaces(active_context);
		fullscreen = true;
		alloc_surfaces(active_context);
	}
	else if (!full && fullscreen)
	{
	// switching to windowed...
		free_surfaces(active_context);
		fullscreen = false;
		alloc_surfaces(active_context);
	}
}

//---------------------------------------------------------------------------
//
// ALLOC/FREE SURFACES
//
//---------------------------------------------------------------------------

HRESULT DrawMgr::alloc_surfaces (DrawContext *window)
//
// Allocate DirectDraw surfaces (Primary, Back and Z-buffer)
//
// ie. create_context_surfaces
{
	HRESULT result;

// SETUP CONTEXT SURFACE INFO

	HWND hWnd = window->hWnd;

	int width = window->width;
	int height = window->height;
	int bpp = window->bpp;

	window->lpDD = lpDD;

// Set up requested fullscreen display mode

	bool flipping = fullscreen && window->flip && (window->pages > 1);
	bool depth = (window->zbits > 0);

	window->width	= width;
	window->height	= height;
	window->bpp		= bpp;

// OPTIONAL - CREATE Z-BUFFER (16 BIT)

	if (depth)
	{
		DDSURFACEDESC ddsd;
		memset(&ddsd, 0, sizeof(DDSURFACEDESC));
		ddsd.dwSize = sizeof(ddsd);

		ddsd.dwFlags =	DDSD_WIDTH  | 
						DDSD_HEIGHT | 
						DDSD_CAPS   | 
						DDSD_ZBUFFERBITDEPTH;

		ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;

		ddsd.dwWidth           = width;
		ddsd.dwHeight          = height;
		ddsd.dwZBufferBitDepth = 16;

		LPDIRECTDRAWSURFACE Z;

		result = lpDD->CreateSurface(&ddsd, &Z, NULL);
		if (result == DD_OK)
		{
			VERBOSE DebugPrint("GLD3D: Z-buffer created.\n");
		}
		else
		{
			DebugAlert(NULL, "GLD3D: Z-buffer creation failed: %s (%d x %d x %d)",DD_message(result), width, height, bpp);
			return result;
		}

		result = Z->QueryInterface(IID_IDirectDrawSurface3, (void **) &window->lpZBuffer);
		if (result != DD_OK)
		{
			DebugAlert(NULL, "GLD3D: Z-buffer query failed: %s",DD_message(result));
			return result;
		}
		Z->Release();
	}

// CREATE PRIMARY SURFACE (POSSIBLY WITH ATTACHED BACK BUFFERS)

	DDSURFACEDESC ddsd;
	memset(&ddsd, 0, sizeof(ddsd));

	LPDIRECTDRAWSURFACE DDSPrimary;
	LPDIRECTDRAWSURFACE DDSBack;

	ddsd.dwSize			= sizeof(ddsd);
	ddsd.ddsCaps.dwCaps	= DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY;
	ddsd.dwFlags		= DDSD_CAPS;

	if (flipping)
	{
		result = lpDD->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT | DDSCL_FULLSCREEN);

		ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_3DDEVICE;
		ddsd.dwBackBufferCount = 1;
	}

	bool complex = false;

	VERBOSE DebugPrint("GLD3D: Creating primary surface:\n");
	result = lpDD->CreateSurface(&ddsd, &DDSPrimary, NULL);
	if (result == DD_OK)
	{
		VERBOSE DebugPrint("GLD3D: Created primary surface%s\n", (flipping) ? " with 1 back buffer." : ".");
		if (flipping)
		{
			complex = true;
		}
	}
	else // CREATE NON-FLIP SURFACE
	{
		VERBOSE DebugPrint("GLD3D: Complex surface creation failed: %s\n", DD_message(result));
		VERBOSE DebugPrint("GLD3D: Unable to create primary surface%s\n", (flipping) ? ": trying non-flipping surface." : ". Aborting.");

		if (flipping)
		{
			flipping = false;

		// Couldn't create complex (flipping) surface, try simple surface with separate back buffer.
			memset(&ddsd, 0, sizeof(ddsd));
			ddsd.dwSize			= sizeof(ddsd);
			ddsd.ddsCaps.dwCaps	= DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY;// | DDSCAPS_3DDEVICE;
			ddsd.dwFlags		= DDSD_CAPS;

			result = lpDD->CreateSurface(&ddsd, &DDSPrimary, NULL);
			if (result == DD_OK)
			{
				VERBOSE DebugPrint("GLD3D: Created simple (non-flipping) primary surface.\n");
				complex = false;
			}
			else
			{
				DebugAlert("GLD3D: Unable to create simple (non-flipping) surface: %s.\n", DD_message(result));
				return result;
			}
		}
		else
		{
			return result;
		}
	}

	result = DDSPrimary->QueryInterface(IID_IDirectDrawSurface3, (void **) &window->lpDDSPrimary);
	RELEASE(DDSPrimary);
	if (result != DD_OK)
	{
		DebugAlert(NULL,"GLD3D: This application requires DirectX 5 or later\n");
		return result;
	}

// CREATE BACK BUFFER (OR GET ATTACHED BACK BUFFER)

	if (flipping && complex)
	{
		DDSCAPS ddscaps;
		ddscaps.dwCaps = DDSCAPS_BACKBUFFER;

		result = window->lpDDSPrimary->GetAttachedSurface(&ddscaps, &window->lpDDSBack);
		if (result == DD_OK)
		{
			VERBOSE DebugPrint("GLD3D: Got pointer to attached back buffer.\n");
		}
		else
		{
			DebugAlert(NULL, "GLD3D: DD->GetAttachedSurface() failed: %s.\n", DD_message(result));
			return result;
		}
	}
	else
	{
	//
	// Allocate single back buffer surface from video memory.
	//
		DDSURFACEDESC ddsd;
		memset(&ddsd, 0, sizeof(ddsd));

		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.dwWidth  = width;
		ddsd.dwHeight = height;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;

		VERBOSE DebugPrint("GLD3D: Creating back buffer: %d, %d, %d\n", width, height, bpp);

		result = lpDD->CreateSurface(&ddsd, &DDSBack, NULL);
		if (result == DD_OK)
		{
			VERBOSE DebugPrint("GLD3D: Created back buffer.\n");
		}
		else
		{
			DebugAlert("Error", "GLD3D: Unable to create back buffer (%d x %d x %d): %s.\n", width, height, bpp, DD_message(result));
			return result;
		}

		result = DDSBack->QueryInterface(IID_IDirectDrawSurface3, (void **) &window->lpDDSBack);
		RELEASE(DDSBack);
		if (result != DD_OK)
		{
			DebugAlert(NULL,"GLD3D: This application requires DirectX 5 or later\n");
			return result;
		}
	}

//
// Set up pixel description.
//
	ddsd.dwSize = sizeof(ddsd);
	window->lpDDSBack->GetSurfaceDesc(&ddsd);
	if (ddsd.dwFlags & DDSD_PIXELFORMAT)
	{
		screen_pixel_format.init(ddsd.ddpfPixelFormat);

		DebugPrint("GLD3D: screen pixel format = RGBA(%d%d%d%d)\n", screen_pixel_format.rwidth,
			screen_pixel_format.gwidth, screen_pixel_format.bwidth, screen_pixel_format.awidth);
	}
	else
	{
		DebugAlert(NULL, "GLD3D: Unable to get pixel format for back buffer.\n");
	}

// SUPPORT DescribePixelFormat()

	PIXELFORMATDESCRIPTOR *pf = get_format(current_format);
	if (pf)
	{
		pf->nSize			= sizeof(PIXELFORMATDESCRIPTOR);
		pf->iPixelType		= screen_pixel_format.is_indexed() ? PFD_TYPE_COLORINDEX : PFD_TYPE_RGBA;
		pf->cColorBits		= (BYTE)screen_pixel_format.ddpf.dwRGBBitCount;
		pf->cDepthBits		= window->zbits;
		pf->cRedBits		= screen_pixel_format.rwidth;
		pf->cRedShift		= screen_pixel_format.rl;
		pf->cGreenBits		= screen_pixel_format.gwidth;
		pf->cGreenShift		= screen_pixel_format.gl;
		pf->cBlueBits		= screen_pixel_format.bwidth;
		pf->cBlueShift		= screen_pixel_format.bl;
		pf->cAlphaBits		= screen_pixel_format.awidth;
		pf->cAlphaShift		= screen_pixel_format.al;
		//pages?
	}

// CHECK NOSYSLOCK ONCE (not sure what this really is?)

	if (lock_flags & DDLOCK_NOSYSLOCK)
	{
		result = window->lpDDSBack->Lock(NULL,&ddsd,lock_flags|DDLOCK_WAIT,NULL);
		if (result == DD_OK)
		{
			window->lpDDSBack->Unlock(NULL);
		}

		if (result == DDERR_INVALIDPARAMS)
		{
			DebugPrint("GLD3D: DDLOCK_NOSYSLOCK not supported?\n");
			lock_flags &= ~DDLOCK_NOSYSLOCK;
		}
	}

// Attach Z-buffer to back buffer

	if (window->lpZBuffer)
	{
	if (window->lpZBuffer->IsLost())
	{
		window->lpZBuffer->Restore();
	}
	if (window->lpDDSBack->IsLost())
	{
		window->lpDDSBack->Restore();
	}

		result = window->lpDDSBack->AddAttachedSurface(window->lpZBuffer);
		if (result != DD_OK)
		{
			DebugAlert(NULL, "GLD3D: Z-buffer attach failed: %s", DD_message(result));
			return result;
		}
	}

//
// If blitting as opposed to flipping, create a clipper.
//
	if (!complex)
	{
		IDirectDrawClipper  * lpDDClipper;
		result = lpDD->CreateClipper(0, &lpDDClipper, NULL);
		if (result != DD_OK)
		{
			DebugAlert(NULL,"GLD3D: DD->CreateClipper() failed, code %X\n",result);
			return result;
		}

		result = lpDDClipper->SetHWnd(0, hWnd);
		if (result != DD_OK)
		{
			DebugAlert(NULL,"GLD3D: Clipper->SetHWnd() failed, code %X\n",result);
			return result;
		}

		result = window->lpDDSPrimary->SetClipper(lpDDClipper);
		if (result != DD_OK)
		{
			DebugAlert(NULL,"GLD3D: Surface->SetClipper failed, code %X\n",result);
			return result;
		}
		lpDDClipper->Release();
	}

// Get Direct3D interface.
	result = lpDD->QueryInterface(IID_IDirect3D2, (void **) &window->lpD3D);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "GLD3D: This application requires DirectX 5 or greater\n");
		return result;
	}

//
// Create Direct3D HAL device
//
// If native hardware support not available from this driver, abort
//
	result = window->lpD3D->CreateDevice(IID_IDirect3DHALDevice, (LPDIRECTDRAWSURFACE) window->lpDDSBack, &window->lpD3DDevice);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "GLD3D: D3D device creation failed: %s",DD_message(result));
		return result;
	}

// SET GLOBAL POINTER FOR EVERYONE TO USE.
	D3DDevice = window->lpD3DDevice;

#ifdef RSTATE_CHECK
	render_states= window->render_states;
#endif

//
// Create Direct3D viewport object and associate it with the device 
// just created
//
	result = window->lpD3D->CreateViewport(&window->lpD3DViewport, NULL);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "GLD3D: CreateViewport() failed: %s",DD_message(result));
		return result;
	}

	result = window->lpD3DDevice->AddViewport(window->lpD3DViewport);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "GLD3D: AddViewport() failed: %s",DD_message(result));
		return result;
	}

	D3DVIEWPORT2 viewData;

	memset(&viewData, 0, sizeof(D3DVIEWPORT));
	viewData.dwSize         =   sizeof(D3DVIEWPORT);
	viewData.dwX            =   0;
	viewData.dwY            =   0;
	viewData.dwWidth        =   width;
	viewData.dwHeight       =   height;
	viewData.dvClipX        =   -1;//0;
	viewData.dvClipY        =   1;//0;
	viewData.dvClipWidth    =   2;//float(width);
	viewData.dvClipHeight   =   2;//float(height);
	viewData.dvMinZ         =   0.0f;
	viewData.dvMaxZ         =   1.0f;

	result = window->lpD3DViewport->SetViewport2(&viewData);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "GLD3D: SetViewport2() failed: %s",DD_message(result));
		return result;
	}

	result = window->lpD3DDevice->SetCurrentViewport(window->lpD3DViewport);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "GLD3D: SetCurrentViewport() failed: %s",DD_message(result));
		return result;
	}

	texture_format_cnt = 0;

	VERBOSE DebugPrint("Texture Formats:\n");
	result = window->lpD3DDevice->EnumTextureFormats(TF_enumerate, NULL);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "GLD3D: EnumTextureFormats() failed: %s",DD_message(result));
		return result;
	}

	if (texture_format_cnt == 0)
	{
		DebugAlert(NULL, "GLD3D: No texture formats supported\n");
		return DDERR_GENERIC;
	}

// Check some goddamn caps bits.
	result = check_D3D_caps();
	if (result != DD_OK)
	{
		return result;
	}

// SET DEFAULT RENDER STATES
#ifdef RSTATE_CHECK
	memset(window->render_states, -1, sizeof(window->render_states));
#endif

	window->set_render_state(D3DRS_SPECULARENABLE, FALSE);

	window->set_render_state(D3DRS_TEXTUREMAPBLEND, D3DTBLEND_MODULATE);
	window->set_render_state(D3DRS_TEXTUREPERSPECTIVE,TRUE);
	window->set_render_state(D3DRS_TEXTUREMIN, D3DFILTER_LINEAR);
	window->set_render_state(D3DRS_TEXTUREMAG, D3DFILTER_LINEAR);

	window->set_render_state(D3DRS_TEXTUREADDRESSU, D3DTADDRESS_WRAP);
	window->set_render_state(D3DRS_TEXTUREADDRESSV, D3DTADDRESS_WRAP);

	window->set_render_state(D3DRS_WRAPU, 0);
	window->set_render_state(D3DRS_WRAPV, 0);

	window->set_render_state(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	window->set_render_state(D3DRS_CULLMODE, D3DCULL_NONE);

	window->set_render_state(D3DRS_ALPHABLENDENABLE, TRUE);

	window->set_render_state(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	window->set_render_state(D3DRS_DESTBLEND, D3DBLEND_ONE);

	window->set_render_state(D3DRS_COLORKEYENABLE, FALSE);

	window->set_render_state(D3DRS_ANTIALIAS, D3DANTIALIAS_NONE);
	window->set_render_state(D3DRS_FOGENABLE, FALSE);

	window->set_render_state(D3DRS_ZENABLE, 1);
	window->set_render_state(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
	window->set_render_state(D3DRS_ZWRITEENABLE, 1);

	return DD_OK; // success
}

//

HRESULT DrawMgr::check_D3D_caps(void)
{
	HRESULT result = DD_OK;

	D3DDEVICEDESC d3dhwcaps, d3dhelcaps;
	d3dhwcaps.dwSize = d3dhelcaps.dwSize = sizeof(D3DDEVICEDESC);
	result = D3DDevice->GetCaps(&d3dhwcaps, &d3dhelcaps);
	if (result == DD_OK)
	{
	// Check for basic capabilities.
	   if (!(d3dhwcaps.dwDevCaps & D3DDEVCAPS_DRAWPRIMTLVERTEX))
	   {
			DebugPrint("GLD3D: Direct3D driver doesn't claim to support DrawPrimitive() with TLVERTEX. This is probably a stupid oversight in the driver. Attempting to continue.\n");
		/*
			if (!DebugAlertYesNo(NULL, "GLD3D: Direct3D driver doesn't claim to support DrawPrimitive() with TLVERTEX. This is probably a stupid oversight in the driver. Attempt to continue?\n"))
			{
				result = DDERR_GENERIC;
			}
		*/
	   }

	   if (!(d3dhwcaps.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEAR))
	   {
		   DebugAlert(NULL, "GLD3D: Hardware doesn't support bilinear texture filtering.\n");
		   result = DDERR_GENERIC;
	   }

	// Check fog capabilities.
		if (d3dhwcaps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGTABLE)
		{
			use_vertex_fog = false;
			DebugPrint("GLD3D: Hardware supports table fog.\n");
		}
		else if (d3dhwcaps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGVERTEX)
		{
			use_vertex_fog = true;
			DebugPrint("GLD3D: Hardware doesn't support table fog. Using vertex fog.\n");
		}
		else
		{
			use_vertex_fog = false;
			DebugAlert(NULL, "GLD3D: Hardware doesn't support fog.\n");
		}
	}

	return result;
}

//---------------------------------------------------------------------------

void DrawMgr::free_surfaces (DrawContext *ctx)
//
// Free all DirectDraw surface components
//
// ie. delete_context_surfaces
{
	if (lpDD != NULL)
	{
		if (fullscreen)
		if (lpDD->SetCooperativeLevel(NULL, DDSCL_NORMAL) != DD_OK)
		{
			DebugPrint("GLD3D: SetCooperativeLevel() failed.\n");
		}

		RELEASE(ctx->lpD3DViewport);

		RELEASE(ctx->lpD3DDevice);

		RELEASE(ctx->lpD3D);

		RELEASE(ctx->lpZBuffer);

		RELEASE(ctx->lpDDSBack);

		RELEASE(ctx->lpDDSPrimary);
	}
}

//---------------------------------------------------------------------------
//
// CREATE/DESTROY CONTEXT
//
//---------------------------------------------------------------------------

bool DrawMgr::create_context (DrawContext *ctx)
{
	if (!DDraw_active)
		return 0;

	if (ctx)
	{
		PIXELFORMATDESCRIPTOR *pf = get_format(ctx->pixel_format);
		if (pf == 0)
			return false;

		ctx->bpp = pf->cColorBits;
		ctx->pages = (pf->dwFlags & PFD_DOUBLEBUFFER) ? 2 : 1;
		ctx->zbits = pf->cDepthBits;

		ctx->flip = 0;
		if (pf->dwFlags & PFD_SWAP_EXCHANGE)
			ctx->flip = 1;
		if (pf->dwFlags & PFD_SWAP_COPY)
			ctx->flip = 0;
		if (pf->dwFlags & PFD_SUPPORT_GDI)
			{ ctx->flip = 0; ctx->pages = 2; }

		HRESULT r = alloc_surfaces(ctx);

		if (r != DD_OK)
		{
			free_surfaces(ctx);
			ctx = NULL;
		}
	}

	return (ctx != 0);
}

//---------------------------------------------------------------------------

void DrawMgr::destroy_context (DrawContext *ctx)
{
	if (active_context == ctx)
		active_context = 0;

	free_surfaces(ctx);
}

//---------------------------------------------------------------------------
//
// RESTORE SURFACE
//
//---------------------------------------------------------------------------

HRESULT DrawMgr::restore_surface (LPDIRECTDRAWSURFACE3 surface)
{
	HRESULT result = DD_OK;

//	if (surface->IsLost() == DDERR_SURFACELOST)
	{
		DebugPrint("restore_surface()\n");

		DDSCAPS caps;
		LPDIRECTDRAWSURFACE3 front = active_context->lpDDSPrimary;
		front->GetCaps(&caps);

		// Avoid if (result == DDERR_IMPLICITLYCREATED)
		if (caps.dwCaps & DDSCAPS_FLIP)
			result = front->Restore();	// restore attached surfaces through primary
		else
			result = surface->Restore();

		if (result == DDERR_WRONGMODE)
		{
			DebugPrint("GL: RECREATE SURFACES\n");
			free_surfaces(active_context);
			result = alloc_surfaces(active_context);
		}

		if (result != DD_OK)
		{
			DebugPrint("Restore() failed.\n=>%s\n",DD_message(result));
			return result;
		}
	}
	return result;
}

//---------------------------------------------------------------------------
//
// LOCK/UNLOCK SURFACE
//
//---------------------------------------------------------------------------

LPDIRECTDRAWSURFACE3 DrawMgr::get_surface (int index)
{
	LPDIRECTDRAWSURFACE3 surface = 0;

	if (active_context)
	{
		if (index == 0)
		{
			surface = active_context->lpDDSPrimary;
		}
		else if (index == 1)
		{
			surface = active_context->lpDDSBack;
		}
	}

	if (surface == 0)
	{
		DebugPrint("Invalid surface index(%d)\n",index);
	}

	return (surface);
}

//---------------------------------------------------------------------------

void DrawMgr::lock_surface (int index, void **ptr, int *pitch)
// Return pointer to surface memory
{
	LPDIRECTDRAWSURFACE3 surface;
	HRESULT              result;
	DDSURFACEDESC        ddsd;

	//DEFAULT
	*ptr = 0;
	*pitch = 0;

	surface = get_surface(index);

	if (surface)
	{

		surface = active_context->lpDDSBack;

		if (surface->IsLost() == DDERR_SURFACELOST)
		{
			if (restore_surface(surface) != DD_OK)
				return;
			surface = get_surface(index); // did pointer change?
		}

		ddsd.dwSize = sizeof(ddsd);

		result = surface->Lock(NULL, &ddsd, lock_flags|DDLOCK_WAIT, NULL);
		if (result != DD_OK)
		{
			DebugPrint("Lock() failed.\n=>%s\n",DD_message(result));
			return;
		}

		if (ptr)   
		{
			*ptr = ddsd.lpSurface;
		}

		if (pitch != NULL) 
		{
			if (ddsd.dwFlags & DDSD_PITCH)
			{
				*pitch = ddsd.lPitch;
			}
			else
			{
				DebugPrint("GLD3D: no pitch?\n");
				*pitch = active_context->calculate_pitch();
			}
		}
	}
}

//---------------------------------------------------------------------------

void DrawMgr::unlock_surface (int index)
// Release surface memory pointer
{
	LPDIRECTDRAWSURFACE3 surface;
	HRESULT              result;

	surface = get_surface(index);

	if (surface)
	{
		if (surface->IsLost() == DDERR_SURFACELOST)
		{
			if (restore_surface(surface) != DD_OK)
				return;
			surface = get_surface(index); // did pointer change?
		}

		result = surface->Unlock(NULL);
		if (result != DD_OK && result != DDERR_NOTLOCKED)
		{
			DebugPrint("Unlock() failed.\n=>%s\n",DD_message(result));
		}
	}
}

//****************************************************************************
//*                                                                          *
//*  Return rectangle containing client-area boundaries in screenspace       *
//*                                                                          *
//****************************************************************************

RECT client_screen_rect (HWND hWnd)
{
   RECT rect;
   POINT ul,lr;

   GetClientRect(hWnd, &rect);

   ul.x = rect.left; 
   ul.y = rect.top; 
   lr.x = rect.right; 
   lr.y = rect.bottom; 

   ClientToScreen(hWnd, &ul); 
   ClientToScreen(hWnd, &lr); 

   SetRect(&rect, ul.x, ul.y, lr.x-1, lr.y-1); 

   return rect;
}

//---------------------------------------------------------------------------
//
// DISPLAY HIDDEN SURFACE (FLIP/BLIT)
//
//---------------------------------------------------------------------------

void DrawMgr::flip_surface (void)
//
// If surface is not flippable, simulate the flip by blitting
//
{
	HRESULT result;

	LPDIRECTDRAWSURFACE3 front = active_context->lpDDSPrimary;

	if (front->IsLost() == DDERR_SURFACELOST)
	{
		if (restore_surface(front) != DD_OK)
			return;
		front = active_context->lpDDSPrimary;	// did pointer change?
	}

	LPDIRECTDRAWSURFACE3 back = active_context->lpDDSBack;

	DDSCAPS caps;
	front->GetCaps(&caps);

	if (caps.dwCaps & DDSCAPS_FLIP)
	{
//DebugPrint("FLIP\n");
	// FLIP BETWEEN FRONT AND BACK SURFACES
		result = front->Flip(NULL, DDFLIP_WAIT);
		if (result != DD_OK)
		{
			DebugPrint("GLD3D: flip failed?\n=>%s\n",DD_message(result));
		}
	}
	else
	{
//DebugPrint("BLIT\n");
	// BACK SURFACE IS NOT ATTACHED SO IT MAY NEED TO BE RESTORED

		if (back->IsLost() == DDERR_SURFACELOST)
		{
			if (restore_surface(back) != DD_OK)
				return;
		}

	// BLIT BACK SURFACE TO FRONT

//		if (fullscreen)
		// weak attempt to determine if in full screen mode
		if (GetWindowLong(active_context->hWnd, GWL_STYLE) & WS_POPUP)
		{
		// In fullscreen mode, use the BltFast() function for maximum
		// speed on unclipped surface
// FUTURE: if (clipper) Blt() else BltFast();
/*
			result = front->BltFast(0, 0, back, 
				NULL, (DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT));
*/
// FUTURE: DDBLT_ASYNC?
			result = front->Blt(NULL, back, NULL, DDBLT_WAIT, NULL);
		}
		else
		{
		// In windowed mode, use the Blt() function for proper
		// clipping (e.g., if the window is moved partially offscreen)
		// 
		// Destination rectangle must be explicitly specified -- note 
		// extra pixel/column in rectangle for Blt()'s benefit
			RECT dest_rect = client_screen_rect(active_context->hWnd);

			++dest_rect.right;
			++dest_rect.bottom;

			result = front->Blt(&dest_rect, back, NULL, DDBLT_WAIT, NULL);
		}

		if (result != DD_OK)
		{
			DebugPrint("GLD3D: blit failed?\n=>%s\n",DD_message(result));
		}
	}
}

//---------------------------------------------------------------------------
//
// CLEAR SURFACE TO COLOR
//
//---------------------------------------------------------------------------

void DrawMgr::clear_color (int index, U32 color, RECT *box)
{
	HRESULT             result;
	LPDIRECTDRAWSURFACE3 surface;

	surface = get_surface(index);
	if (surface)
	{
		DDBLTFX             ddbltfx;
		ddbltfx.dwSize      = sizeof(ddbltfx);
		ddbltfx.dwFillColor = color;

		if (surface->IsLost() == DDERR_SURFACELOST)
		{
			if (restore_surface(surface) != DD_OK)
				return;
			surface = get_surface(index); // did pointer change?
		}

		result = surface->Blt(box, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT, &ddbltfx);
		if (result != DD_OK)
		{
			DebugPrint("clear_color Blt() failed.\n=>%s\n",DD_message(result));
		}
	}
}

//---------------------------------------------------------------------------

void DrawMgr::clear_zbuffer (U32 depth, RECT *box)
{
	HRESULT				result;
	LPDIRECTDRAWSURFACE3 zbuff = active_context->lpZBuffer;
	if (zbuff)
	{
		if (zbuff->IsLost() == DDERR_SURFACELOST)
		{
			// EXPLICIT SURFACE MUST BE RESTORED
			result = zbuff->Restore();
			if (result != DD_OK)
			{
				DebugPrint("Can't restore z-buffer.\n=>%s\n",DD_message(result));
				return;
			}
		}

		DDBLTFX ddbltfx;
		ddbltfx.dwSize = sizeof(ddbltfx);
		// Note: 0.0 (znear) to 1.0 (far)
		ddbltfx.dwFillDepth = depth;

		result = zbuff->Blt(box, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_DDFX | DDBLT_WAIT, &ddbltfx);
		if (result != DD_OK)
		{
			DebugPrint("Can't clear z-buffer.\n=>%s\n",DD_message(result));
		}
	}
}

//---------------------------------------------------------------------------

void hackSetlpDD(LPDIRECTDRAW dd)
{
	if (dd->QueryInterface(IID_IDirectDraw2, (void **) &lpDD) != DD_OK)
	{

	}
}

//
