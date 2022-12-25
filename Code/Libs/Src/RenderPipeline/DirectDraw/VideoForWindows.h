// VideoForWindows.h
//
// VideoForWindows Utility functions
// 
//


#ifndef DA_VideoForWindows_H
#define DA_VideoForWindows_H

//

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
//#include <ddraw.h>
#include <d3d9.h>

#include "typedefs.h"
#include "pixel.h"
#include "rendpipeline.h"
#include "rpul.h"
#include "IVideoStreamControl.h"

#include "DirectDraw.h"
#include "DirectDrawTexture.h"

//

#define RPVS_F_SILENT	(1<<0)

//
// DirectShow Related Stuff
//

HRESULT rp_vfw_init( LPGUID lpGuid, LPDIRECTDRAW7 *lpDD );

HRESULT rp_vfw_vstream_control_create( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 target_surface, U32 flags, IVideoStreamControl **out_ivsc );

#endif // EOF
