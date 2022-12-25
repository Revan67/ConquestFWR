//
// <treebar.h> - extent tree sidebar
//

#ifndef TREEBAR_H
#define TREEBAR_H

#include "main.h"
#include "extent.h"

struct TreeBar
{
    static HWND         handle;
    static HFONT        font;

    static BOOL32 create();
    static BOOL32 destroy();

    static void refresh();
    static void refresh_extent_list();
    static void refresh_all_windows();
    
    static void display_master_tree();
        
};

#endif