//
// <extutil.h> - extent utilities
//

#ifndef EXTUTIL_H
#define EXTUTIL_H

#include "phyedit.h"

// remove a single extent from the tree
void remove_extent(BaseExtent * base, BaseExtent * remove, BOOL32 unlink_only = FALSE);         

// remove (delete) entire tree
void remove_extent_tree(BaseExtent * base);

// add extents to tree
BOOL32 add_extent_as_root(S32 index);
BOOL32 add_extent_as_next(BaseExtent * base, BaseExtent * extent);
BOOL32 add_extent_as_child(BaseExtent * base, BaseExtent * extent);

// save extents
BOOL32 write_extent_tree(C8 * filename);

// autogenerate
void autogenerate_sphere();
void autogenerate_sphere_box();
void autogenerate_inertia_tensor();

// editing

void edit_rigid_data();

enum MirrorType
{
	MIRROR_X,
	MIRROR_Y
};

void mirror_tree(BaseExtent * extent, MirrorType type, const Vector & mirror_bias);

#endif