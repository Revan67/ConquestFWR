#ifndef RESULTS_H
#define RESULTS_H
//----------------------------
// General result codes for functions
//----------------------------

#ifdef __cplusplus

enum GENRESULT 
{
	GR_OK						= 0x00000000,
	GR_GENERIC					= -1,
	GR_INVALID_PARMS			= -2,
	GR_INTERFACE_UNSUPPORTED	= -3,
	GR_OUT_OF_MEMORY			= -4,
	GR_OUT_OF_SPACE				= -5,
	GR_FILE_ERROR				= -6,
	GR_NOT_IMPLEMENTED			= -7,
	GR_DATA_NOT_FOUND			= -8
};

#else
				  
typedef S32 GENRESULT;

#define GR_OK                    0x00000000
#define GR_GENERIC               (GENRESULT(-1))
#define GR_INVALID_PARMS         (GENRESULT(-2))
#define GR_INTERFACE_UNSUPPORTED (GENRESULT(-3))
#define GR_OUT_OF_MEMORY         (GENRESULT(-4))
#define GR_OUT_OF_SPACE          (GENRESULT(-5))
#define GR_FILE_ERROR            (GENRESULT(-6))
#define GR_NOT_IMPLEMENTED       (GENRESULT(-7))
#define GR_DATA_NOT_FOUND        (GENRESULT(-8))

#endif

#endif