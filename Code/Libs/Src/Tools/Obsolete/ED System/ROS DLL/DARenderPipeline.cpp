// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "DARenderPipeline.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
IRenderPipeline*	PIPE = NULL;
// --------------------------------------------------------------------------
#ifdef __cplusplus 
extern "C" { 
#endif
// --------------------------------------------------------------------------
bool __cdecl RPStartup(unsigned int colorBpp, unsigned int depthBpp)
{
	ASSERT(PIPE != NULL);

	if(PIPE->startup() == GR_OK)
	{	PIPE->set_pipeline_state(RP_BUFFERS_COLOR_BPP, colorBpp);
		PIPE->set_pipeline_state(RP_BUFFERS_DEPTH_BPP, depthBpp);
		PIPE->set_pipeline_state(RP_BUFFERS_COUNT, 2);

		return true;
	}
	else
	{	return false;
	}
}
// --------------------------------------------------------------------------
void __cdecl RPShutDown()
{
	ASSERT(PIPE != NULL);

	GENRESULT genResult = PIPE->shutdown();

	ASSERT(genResult == GR_OK);
}
// --------------------------------------------------------------------------
#ifdef __cplusplus 
}
#endif
// --------------------------------------------------------------------------
