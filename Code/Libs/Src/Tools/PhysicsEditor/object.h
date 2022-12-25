//
// <object.h>
//

#ifndef OBJECT_H
#define OBJECT_H

#include "main.h"
#include "basecam.h"

struct Object
{
	INSTANCE_INDEX	index;
	Vector			center_of_mass;
	
	Mesh			*mesh;
	BaseExtent		*tree;
	SINGLE			mass;
	Matrix			Ibody;
	
	C8				name[256];
	C8				filename[256];
	BOOL32			read_only;
	SINGLE			scale;	//scale is fabs(biggest axis of box)
	
	Object()
	{
		initialize();
	}
	
	void	initialize(void);
	void	reset(void);
	void	render(BaseCamera *camera, BOOL32 render_extents);
	void	render_extents(void);
	void	select_extents(Vector &raynear);
	BOOL32	load(C8 *fn);
	int		GetNumMeshes(INSTANCE_INDEX idx);
};

#endif
