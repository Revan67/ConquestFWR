#ifndef RENDERER_PROP_H
#define RENDERER_PROP_H

// used by IRenderer get/set_render_property()
typedef enum
{
	SUB_PIXEL_THRESHOLD,	// polymesh sub pixel polygon threshold
	NURB_PIXEL_ERROR,		// nurbmesh maximum tessellation error
	BEZIER_SUBDIV_CNT
} RenderProp;

#endif