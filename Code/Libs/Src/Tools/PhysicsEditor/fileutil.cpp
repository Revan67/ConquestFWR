//
// <fileutil.cpp>
//

#include "fileutil.h"
#include <commdlg.h>

C8   copy_path[256];
C8   copy_file_list[256][256];
S32  copy_file_count;

BOOL32 open_file_dialog(C8 *full_path_buffer)
{
    char *filter = "3DB Files (*.3db)\0*.3db\0Compound Files (*.cmp)\0*.cmp\0All files (*.*)\0*.*\0\0";

    full_path_buffer[0] = NULL;
    
    OPENFILENAME ofn;

	ofn.lStructSize         = sizeof(OPENFILENAME);
	ofn.hwndOwner           = windowHandle;
	ofn.hInstance           = NULL;
	ofn.lpstrFilter         = filter;
	ofn.lpstrCustomFilter   = NULL;
	ofn.nMaxCustFilter      = 0;
	ofn.nFilterIndex        = 0;
	ofn.lpstrFile           = full_path_buffer;
	ofn.nMaxFile            = _MAX_PATH;
	ofn.lpstrFileTitle      = NULL;
	ofn.nMaxFileTitle       =_MAX_FNAME+_MAX_EXT;
	ofn.lpstrInitialDir     = NULL;
	ofn.lpstrTitle          = NULL;
	ofn.Flags               = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
	ofn.nFileOffset         = 0;
	ofn.nFileExtension      = 0;
	ofn.lpstrDefExt         = "3db";
	ofn.lCustData           = 0L;
	ofn.lpfnHook            = NULL;
	ofn.lpTemplateName      = NULL;

    if (GetOpenFileName(&ofn))
    {
        return TRUE;
    }
    else
    {
        // error
        return FALSE;
    }

}


BOOL32 save_as_file_dialog(C8 *full_path_buffer)
{
    char *filter = "3DB Files (*.3db)\0*.3db\0Compound Files (*.cmp)\0*.cmp\0All files (*.*)\0*.*\0\0";

    full_path_buffer[0] = NULL;
    
    OPENFILENAME ofn;

	ofn.lStructSize         = sizeof(OPENFILENAME);
	ofn.hwndOwner           = windowHandle;
	ofn.hInstance           = NULL;
	ofn.lpstrFilter         = filter;
	ofn.lpstrCustomFilter   = NULL;
	ofn.nMaxCustFilter      = 0;
	ofn.nFilterIndex        = 0;
	ofn.lpstrFile           = full_path_buffer;
	ofn.nMaxFile            = _MAX_PATH;
	ofn.lpstrFileTitle      = NULL;
	ofn.nMaxFileTitle       =_MAX_FNAME+_MAX_EXT;
	ofn.lpstrInitialDir     = NULL;
	ofn.lpstrTitle          = NULL;
	ofn.Flags               = OFN_HIDEREADONLY;
	ofn.nFileOffset         = 0;
	ofn.nFileExtension      = 0;
	ofn.lpstrDefExt         = "3db";
	ofn.lCustData           = 0L;
	ofn.lpfnHook            = NULL;
	ofn.lpTemplateName      = NULL;

    if (GetSaveFileName(&ofn))
    {
        return TRUE;
    }
    else
    {
        // error
        return FALSE;
    }

}

BOOL32 copy_file_dialog()
{
    char * filter = "3DB Files (*.3db)\0*.3db\0Compound Files (*.cmp)\0*.cmp\0All files (*.*)\0*.*\0\0";

    OPENFILENAME ofn;
    char outbuffer[32768];

	ofn.lStructSize         = sizeof(OPENFILENAME);
	ofn.hwndOwner           = windowHandle;
	ofn.hInstance           = NULL;
	ofn.lpstrFilter         = filter;
	ofn.lpstrCustomFilter   = NULL;
	ofn.nMaxCustFilter      = 0;
	ofn.nFilterIndex        = 0;
	ofn.lpstrFile           = outbuffer;
	ofn.nMaxFile            = sizeof(outbuffer);
	ofn.lpstrFileTitle      = NULL;
	ofn.nMaxFileTitle       =_MAX_FNAME+_MAX_EXT;
	ofn.lpstrInitialDir     = NULL;
	ofn.lpstrTitle          = "Copy (export)";
	ofn.Flags               = OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_EXPLORER;
	ofn.nFileOffset         = 0;
	ofn.nFileExtension      = 0;
	ofn.lpstrDefExt         = "3db";
	ofn.lCustData           = 0L;
	ofn.lpfnHook            = NULL;
	ofn.lpTemplateName      = NULL;

    if (GetOpenFileName(&ofn))
    {
        // lpstrfile contains filenames
        copy_file_count = 0;
        
        char * ofs = ofn.lpstrFile + ofn.nFileOffset;

        strncpy(copy_path, ofn.lpstrFile, ofn.nFileOffset - 1);

        S32 sx = strlen(ofs);

        for (int i = 0; i < sx; i++)
        {
            if (ofs[i] == ' ') ofs[i] = 0;
        }

        while (*ofs != 0)
        {
            strcpy(copy_file_list[copy_file_count], ofs);
            ofs += strlen(ofs) + 1;

            copy_file_count++;
        }
         
        return TRUE;
    }
    else
    {
        // cancel
        return FALSE;
    }

}

void delete_fs_tree(IFileSystem * fs)
{
	WIN32_FIND_DATA wfd;
	HANDLE handle = fs->FindFirstFile("*.*", &wfd);
	
    if (handle != INVALID_HANDLE_VALUE)
    {
		do
        {
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
				fs->SetCurrentDirectory(wfd.cFileName);
				delete_fs_tree(fs);
				fs->SetCurrentDirectory("..");
				fs->RemoveDirectory(wfd.cFileName);
			}
			else
            {
				fs->DeleteFile(wfd.cFileName);
			}
		}
    	while (fs->FindNextFile(handle, &wfd));
    }
	fs->FindClose(handle);

}

void DeleteDirectory(IFileSystem * fs, const char * name)
{
    if (fs->SetCurrentDirectory(name))
    {
        delete_fs_tree(fs);
        fs->SetCurrentDirectory("..");
        fs->RemoveDirectory(name);
    }
}
