//
// <editmode.h> - virtual editing modes
//

#ifndef EDITMODE_H
#define EDITMODE_H

#include "main.h"
#include "input.h"
#include "font.h"

enum EditModeType
{
    EM_SPHERE,
    EM_BOX,
	EM_TUBE,
    EM_CONVEX_MESH
};

class EditMode
{
    public:

    S32         type;
    char        mode_string[64];
    
    virtual void init(BaseExtent * extent, BOOL32 init_extent) = NULL;
    virtual void keyboard_handler(const KeyboardMessage & kb) = NULL;
    virtual void mouse_handler(const MouseMessage & mouse) = NULL;
	virtual void reset_selection(void) = NULL;
    
    virtual void post_render() = NULL;
    virtual void font_render(Font * font) = NULL;
    virtual void help_print(Font * font) = NULL;

    virtual BaseExtent * get_edit_extent(void) = NULL;

    EditMode()
    {        
    }

};

class SphereEditor : public EditMode
{
    public:

    SphereExtent * edit_extent;

    virtual void init(BaseExtent * extent, BOOL32 init_extent);
    virtual void keyboard_handler(const KeyboardMessage & kb);
    
    virtual void mouse_handler(const MouseMessage & m)
    {
    }
	virtual void reset_selection(void){};

    virtual void post_render();
    virtual void font_render(Font * font) {};
    virtual BaseExtent * get_edit_extent(void) { return (BaseExtent *) edit_extent; }
    virtual void help_print(Font * font);
    
    SphereEditor()
    {
        type = EM_SPHERE;
        edit_extent = NULL;
        strcpy(mode_string, "Sphere Editor");
    }
    
};

class BoxEditor : public EditMode
{
    public:

    BoxExtent * edit_extent;

    virtual void init(BaseExtent * extent, BOOL32 init_extent);
    virtual void keyboard_handler(const KeyboardMessage & kb);
    virtual void mouse_handler(const MouseMessage & m)
    {
    }
	virtual void reset_selection(void){};
    virtual void post_render();
    virtual void font_render(Font * font) {};    
    virtual BaseExtent * get_edit_extent(void) { return (BaseExtent *) edit_extent; }
    virtual void help_print(Font * font);
    
    BoxEditor()
    {
        type = EM_BOX;
        edit_extent = NULL;
        strcpy(mode_string, "Box Editor");
    }
    
};

class TubeEditor : public EditMode
{
	public:

	TubeExtent * edit_extent;

	virtual void init(BaseExtent * extent, BOOL32 init_extent);
    virtual void keyboard_handler(const KeyboardMessage & kb);
    virtual void mouse_handler(const MouseMessage & m)
    {
    }
	virtual void reset_selection(void){};
    virtual void post_render();
    virtual void font_render(Font * font) {};    
    virtual BaseExtent * get_edit_extent(void) { return (BaseExtent *) edit_extent; }
    virtual void help_print(Font * font);
    
	TubeEditor()
	{
		type = EM_TUBE;
		edit_extent = NULL;
		strcpy(mode_string, "Tube Editor");
	}
};

class ConvexMeshEditor : public EditMode
{
    public:

    S32 mouse_x, mouse_y;

    S32 selected_vertex_count;
    S32 selected_vertex_list[4096];

    ConvexMeshExtent * edit_extent;

    virtual void init(BaseExtent * extent, BOOL32 init_extent);
    virtual void keyboard_handler(const KeyboardMessage & kb);
    virtual void mouse_handler(const MouseMessage & m);
    virtual void post_render();
    virtual void font_render(Font * font);
    virtual void help_print(Font * font);
	virtual void reset_selection(void)
	{
		selected_vertex_count	=0;
	}
    
    virtual BaseExtent * get_edit_extent(void) { return (BaseExtent *) edit_extent; }
    
    virtual void add_selected_vertex(S32 sx, S32 sy);
	void	select_verts_by_face(void);

    ConvexMeshEditor()
    {
        type = EM_CONVEX_MESH;
        edit_extent = NULL;
        strcpy(mode_string, "Convex Mesh Editor");

        selected_vertex_count = 0;
        memset(selected_vertex_list, 0, sizeof(selected_vertex_list));
    }
};

#endif