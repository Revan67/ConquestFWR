//
// <submesh.h> - mesh editing and related functions
//

#ifndef SUBMESHEDIT_H
#define SUBMESHEDIT_H

#include "main.h"
#include "input.h"

struct SMFace
{
	S32		v[3];
	Vector	normal;
};

struct SubMeshEditor
{
    BOOL32  active;
    BOOL32  vertex_selection_only;
    
    S32     mouse_x, mouse_y;

    S32     selected_face_count;
    S32     selected_faces[4096 * 16];                        // indices into object face list

    BOOL32  creating_box;
    S32     box_start_x, box_start_y;

	// pseudo-mesh vars
	
	Vector	vlist[8192 * 16];
	S32		vcount;
	
	SMFace	flist[4096 * 16];
	S32		fcount;

    SubMeshEditor()
    {
        active = TRUE;
        vertex_selection_only = TRUE;

        mouse_x = 0; 
        mouse_y = 0;
        
        selected_face_count = 0;

        creating_box = FALSE;
    }

    void reset(INSTANCE_INDEX idx)
    {
        selected_face_count = 0;
        creating_box = FALSE;

		vcount = 0;
		fcount = 0;
		fill_vlist(idx);

    }

    void process_keyboard(KeyboardMessage * message);
    void process_mouse(MouseMessage * message);

    void calculate_selected_submesh();
    void toggle_selected_face(S32 mouse_x, S32 mouse_y);
    
	void get_closest_vertex(S32 target_x, S32 target_y, INSTANCE_INDEX index, Vector * vertex);
	void fill_vlist(INSTANCE_INDEX index);

    void render();

};

#endif