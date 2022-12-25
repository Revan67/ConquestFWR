// RPUL_Misc.h
//
//
//

#ifndef RPUL_MISC_H
#define RPUL_MISC_H

#ifndef _WINDOWS_
#error Windows.h required for this to compile!
#endif

#ifndef DACOM_H
#include "dacom.h"
#endif

#ifndef RENDPIPELINE_H
#include "rendpipeline.h"
#endif

#ifndef RESULTS_H
#include "results.h"
#endif

GENRESULT CreateDeviceDescription( const char *device_class, const char *device_id, char *out_desc );
GENRESULT GetDeviceClassAndId( const char *interface_desc, char *device_class, char *device_id );
GENRESULT GetDeviceInfoInFile( const char *ini_file, char *dclass, char *did);
GENRESULT SetDeviceInfoInFile( const char *ini_file, const char *device_class, const char *device_id );
GENRESULT GetPrimaryGUID( char *out_guid );

typedef enum {
	RPUL_DIRECT3D = 1,
	RPUL_OPENGL   = 2,
	RPUL_GLIDE    = 4,
	RPUL_RASTER   = 8
} RPDEVICECLASS;

typedef BOOL (*RPUL_ENUMERATOR_FUNC)( IDAComponent *device, void *user );

GENRESULT EnumerateRenderPipelineDevices( U32 classes, RPUL_ENUMERATOR_FUNC cb, void *user_data, U32 *out_num );


LPGUID ConvertStringToGUID( char *string, LPGUID guid );
char *ConvertGUIDToString( LPGUID guid, char *string );

HRESULT mem_bitblt( void *dst_bits, int dst_width, int dst_height, int dst_stride, PixelFormat & dst_format,
					const void *src_bits, int src_width, int src_height, int src_stride, const PixelFormat &src_format,
					const RGB *src_palette, const U8 *src_alpha );

HRESULT mem_bitblt_invert( void *dst_bits, int dst_width, int dst_height, int dst_stride, PixelFormat & dst_format,
					const void *src_bits, int src_width, int src_height, int src_stride, const PixelFormat &src_format,
					const RGB *src_palette, const U8 *src_alpha );


#endif
