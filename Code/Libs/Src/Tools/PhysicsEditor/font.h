//
// <font.h> - ick
//

#ifndef FONT_H
#define FONT_H

#include "filesys.h"
#include "ITextureLibrary.h"

struct Font
{
	U32	RPTextureID;
	
    BOOL32 load(C8 *font)
    {
        COMPTR <IFileSystem> fs;
		ITL_TEXTURE_ID			index;
		ITL_TEXTUREFRAME_IRP	frame;

        if (ENGINE->create_file_system(font, fs) == GR_OK)
        {
            TEXTURELIB->load_library(fs, NULL);
			TEXTURELIB->get_texture_id( "font", &index );
			if(TEXTURELIB->get_texture_frame(index, 0, &frame) == GR_OK)
			{
				RPTextureID	=frame.rp_texture_id;
			}
        }
        else
        {
            return FALSE;
        }
    }

    BOOL32 load(IFileSystem *fs)
    {
		ITL_TEXTURE_ID			index;
		ITL_TEXTUREFRAME_IRP	frame;

        TEXTURELIB->load_library(fs, NULL);
		TEXTURELIB->get_texture_id( "font", &index );
		if(TEXTURELIB->get_texture_frame(index, 0, &frame) == GR_OK)
		{
			RPTextureID	=frame.rp_texture_id;
		}
		
		return TRUE;
	}

    SINGLE get_height( void ) const
    {
    	return 16.0;
    }

    SINGLE get_width( void ) const
    {
    	return 8.0;
    }

    void begin()
    {
        Transform t;
        t.set_identity();
        RP->set_modelview(t);

        RP->set_viewport(0, 0, 640, 480);
        RP->set_ortho(0, 640, 480, 0);
		RP->set_texture_stage_texture(0, RPTextureID);
        
        // glBlend(GL_SRC_ALPHA, GL_ONE);
        RP->set_render_state(D3DRS_ALPHABLENDENABLE, TRUE );
        RP->set_render_state(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        RP->set_render_state(D3DRS_DESTBLEND, D3DBLEND_ONE);
        
        RP->set_render_state(D3DRS_ZFUNC, D3DCMP_ALWAYS);
        RP->set_render_state(D3DRS_CULLMODE, D3DCULL_NONE);
    }

    void draw_text(S32 x, S32 y, char *text, U8 r = 255, U8 g = 255, U8 b = 255)
    {
        // requires you to bind texture

        C8      cval;
        SINGLE  char_width      = get_width();
        SINGLE  char_height     = get_height();
        SINGLE  scale_factor    = 1.0;
        SINGLE  u, v;
        SINGLE  du, dv;

        RPVertex vlist[3];

        for (U32 i = 0; i < strlen(text); i++)
        {
            cval = text[i] - 32;

            if (cval >= 96) return;

            u = (SINGLE) ((cval % 12) * char_width) / 256.0;
            v = (SINGLE) ((cval / 12) * char_height) / 256.0;
           
            u += 0.0045;
            v -= 0.005;

            du = char_width / 256.0;
            dv = char_height / 256.0;

            vlist[0].pos.set(x + (scale_factor * (SINGLE) char_width * i), y, 0.0);
            vlist[0].u = u; vlist[0].v = v;
            vlist[0].r = r; vlist[0].g = g; vlist[0].b = b; vlist[0].a = 255;

            vlist[1].pos.set(x + (scale_factor * (SINGLE) char_width * (i + 1)), y, 0.0);
            vlist[1].u = u + du; vlist[1].v = v;
            vlist[1].r = r; vlist[1].g = g; vlist[1].b = b; vlist[1].a = 255;

            vlist[2].pos.set(x + (scale_factor * (SINGLE) char_width * (i + 1)), y + scale_factor * (SINGLE) char_height, 0.0);
            vlist[2].u = u + du; vlist[2].v = v + dv;
            vlist[2].r = r; vlist[2].g = g; vlist[2].b = b; vlist[2].a = 255;

            RP->draw_primitive(D3DPT_TRIANGLELIST, D3DFVF_RPVERTEX, vlist, 3, 0);
            
            vlist[0].pos.set(x + (scale_factor * (SINGLE) char_width * i), y, 0.0);
            vlist[0].u = u; vlist[0].v = v;
            vlist[0].r = r; vlist[0].g = g; vlist[0].b = b; vlist[0].a = 255;

            vlist[1].pos.set(x + (scale_factor * (SINGLE) char_width * (i + 1)), y + scale_factor * (SINGLE) char_height, 0.0);
            vlist[1].u = u + du; vlist[1].v = v + dv;
            vlist[1].r = r; vlist[1].g = g; vlist[1].b = b; vlist[1].a = 255;

            vlist[2].pos.set(x + (scale_factor * (SINGLE) char_width * i), y + scale_factor * (SINGLE) char_height, 0.0);
            vlist[2].u = u; vlist[2].v = v + dv;
            vlist[2].r = r; vlist[2].g = g; vlist[2].b = b; vlist[2].a = 255;
            
            RP->draw_primitive(D3DPT_TRIANGLELIST, D3DFVF_RPVERTEX, vlist, 3, 0);
                                
        }
        
    }

    inline void print_var(S32 x, S32 y, C8 * header, C8 * message, ...)
    {
	    static C8 buffer[1024];

        draw_text(x, y, header, 240, 240, 240);

        va_list arglist;
	    va_start(arglist, message);
	    vsprintf(buffer, message, arglist);
	    va_end(arglist);

        draw_text(x + ((strlen(header) + 1) * 8), y, buffer, 120, 120, 200);
    }

    void end()
    {
        RP->set_render_state(D3DRS_ZFUNC, D3DCMP_LESS);
    }
};

extern Font theFont;

#endif
