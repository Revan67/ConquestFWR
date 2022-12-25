
#ifndef PHYEDIT_H
#define PHYEDIT_H

#include "main.h"
#include "basecam.h"
#include "font.h"
#include "object.h"
#include "editmode.h"
#include "submesh.h"

#define PEC_SOLID_EXTENTS       0x01
#define PEC_DRAW_TEXTURES       0x02
#define PEC_COLLIDABLE_ONLY     0x04

class	PhysicsEditor
{
public:
	BOOL32		cmp_edit;
	BOOL32		bHelp;
	Object		object;
	Object		*ChildObjects;
	Object		*CurObject;
	BaseCamera	*camera;
	Font		*font;
	BaseExtent	*selected_extent;			// selected extent in main tree
	SINGLE		keyboard_dt;
	int			NumChildObjects;
	
	// Free extent list:
	S32			free_extent_count;			// free extent count
	BaseExtent	*free_extent_list[256];		// list of free extents
    EditMode	*free_extent_editors[256];	// list of editors associated with them
    S32			selected_free_extent;		// index of selected free extent
	
	// Free extent editor list:
	
	EditMode	*current_editor;
	
	U32			flags;

	PhysicsEditor(void)
	{
		NumChildObjects		=0;
		cmp_edit			=FALSE;
		camera				=NULL;
		font				=NULL;
		flags				=0;
		selected_extent		=NULL;
		current_editor		=NULL;
		free_extent_count	=0;
		ChildObjects		=NULL;
		bHelp				=FALSE;
		
		memset(free_extent_list, NULL, sizeof(free_extent_list));
		selected_free_extent=-1;
	}
	
	void	initialize_matrix(void);
	Object	*FindExtentOwner(BaseExtent *bx);
	void	init(void);
	void	update(void);
	void	uninit(void);
	void	handle_keyboard(SINGLE dt);
	void	set_flag(U32 flag)	{	flags	|=flag;					}
	void	clr_flag(U32 flag)	{	flags	&=~flag;				}
	BOOL32	is_flag(U32 flag)	{	return((flags & flag) == flag);	}
	BOOL32	ToggleHelp(void)	{	bHelp	=!bHelp; return	bHelp;	}
};

extern PhysicsEditor    physicsEditor;
extern SubMeshEditor    subMeshEditor;
extern SphereEditor     sphereEditor;
extern BoxEditor        boxEditor;
extern TubeEditor		tubeEditor;
extern ConvexMeshEditor convexMeshEditor;

#endif