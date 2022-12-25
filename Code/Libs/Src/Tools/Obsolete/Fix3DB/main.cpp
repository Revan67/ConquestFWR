//
// <fix3db> - 3DB/UTF attribute utility
//

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>

#include "dacom.h"
#include "filesys.h"
#include "tsmartpointer.h"
#include "vector.h"

#include "fileutil.h"
#include "faceprop.h"

ICOManager *			DACOM;

void fatal(char *exit_string, ...)
{
	char message[256];

	va_list arglist;
	va_start(arglist, exit_string);
	vsprintf(message, exit_string, arglist);
	va_end(arglist);
	
	printf(message);

	DACOM->ShutDown();
	DACOM = NULL;

	exit(1);
}

inline HANDLE CreateTempFile (char tempname[MAX_PATH+4])
{
	char path[MAX_PATH+4];
	DWORD dwPrefix = '\0ert';

	GetTempPath(MAX_PATH, path);	

	if (GetTempFileName(path, (const char *)&dwPrefix, 0, tempname) == 0)
		return 0;

 	return ::CreateFile(tempname, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
}

BOOL32 file_exists(char *filename)
{
	_finddata_t find;
	
	S32 handle = _findfirst(filename,&find);
	
	if (handle == -1)
	{
		return FALSE;
	}
	else
	{
		_findclose(handle);
		return TRUE;
	}
}

char szINIData[] = "[Libraries]\r\nDOSFILE.dll";

void start_dacom (void)
{
	HANDLE hFile;
	char tempname[MAX_PATH+4];
	DWORD dwWritten;

	hFile = CreateTempFile(tempname);
	WriteFile(hFile, szINIData, sizeof(szINIData), &dwWritten, 0); 
	CloseHandle(hFile);

	if (DACOM->SetINIConfig(tempname, 0) != GR_OK)
        fatal("FATAL: couldn't init DACOM - check DACOM environment variable");

	DeleteFile(tempname);
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

void strip_objects(char * file)
{
	WIN32_FIND_DATA wfd;
    HANDLE h = FindFirstFile(file, &wfd);

	WIN32_FIND_DATA fd;
	HANDLE ht;
	
	WIN32_FIND_DATA md;
	HANDLE mtest;

	BOOL32 delete_it = FALSE;

	if (h != INVALID_HANDLE_VALUE)
	{
		do
		{
			printf("Stripping...");

			if (IFileSystem * fmain = OpenDirectory(wfd.cFileName, NULL))
			{
				if (IFileSystem * fmesh = OpenDirectory("openFLAME 3D N-mesh", fmain))
				{
					if (IFileSystem * ftxt = OpenDirectory("Texture library", fmesh))
					{
						ht = ftxt->FindFirstFile("*.*", &fd);
						
						delete_it = TRUE;
								
						if (ht != INVALID_HANDLE_VALUE)
						{
							do
							{
								strupr(fd.cFileName);
								
								if (strstr(fd.cFileName, "ALPHA"))
								{
									delete_it = FALSE;
								}
								else
								{
									// test for alpha bits
									if (IFileSystem * ftex = OpenDirectory(fd.cFileName, ftxt))
									{
										if (IFileSystem * mip0 = OpenDirectory("MIP0", ftex))
										{
											if (IFileSystem * pal = OpenDirectory("Palette 8 bit", mip0))
											{
												mtest = pal->FindFirstFile("Alpha 8 bit", &md);

												if (mtest != INVALID_HANDLE_VALUE)
												{
													delete_it = FALSE;
												}
			
												pal->Release();
											}

											mip0->Release();
										}

										ftex->Release();
									}
								}
									
							}
							while (ftxt->FindNextFile(ht, &fd));

							if (delete_it)
							{
								printf("OK\n");
								ftxt->Release();

								DeleteDirectory(fmesh, "Texture library");
								
							}
							else
							{
								printf("SKIPPED (found alpha)\n");
							}
							
						}
						else
						{
							printf("SKIPPED (already stripped)\n");
						}
							
						fmesh->Release();
							
					}
					else
					{
						printf("SKIPPED (already stripped)\n");
					}
				
				}
				
				fmain->Release();
			}
		}
		while (FindNextFile(h, &wfd));
	}

}

void spin_object(char * file)
{
    if (IFileSystem * fmain = OpenDirectory(file, NULL))
    {
        if (IFileSystem * fmesh = OpenDirectory("openFLAME 3D N-mesh", fmain))
        {
            if (IFileSystem * fvert = OpenDirectory("Vertices", fmesh))
            {
                S32 vcount;
                LoadFile("Object vertex count", &vcount, sizeof(S32), fvert);

                Vector * vlist = new Vector[vcount];
                LoadFile("Object vertex list", vlist, sizeof(Vector) * vcount, fvert);

                for (int i = 0; i < vcount; i++)
                {
                    vlist[i] *= -1.0;
                }

                SaveFile("Object vertex list", vlist, sizeof(Vector) * vcount, fvert);

                delete vlist;
                fvert->Release();
            }

            if (IFileSystem * fvert = OpenDirectory("Vertices", fmesh))
            {
                S32 vcount;
                LoadFile("Object vertex count", &vcount, sizeof(S32), fvert);

                Vector * vlist = new Vector[vcount];
                LoadFile("Object vertex list", vlist, sizeof(Vector) * vcount, fvert);

                for (int i = 0; i < vcount; i++)
                {
                    vlist[i] *= -1.0;
                }

                SaveFile("Object vertex list", vlist, sizeof(Vector) * vcount, fvert);

                delete vlist;
                fvert->Release();
            }




            fmesh->Release();
        }

        fmain->Release();
    }
}

void clamp_textures(char * file)
{
	static U32 clamp = 1;

	if (IFileSystem * fmain = OpenDirectory(file, NULL))
	{
		if (IFileSystem * ftex = OpenDirectory("Texture library", fmain))
		{
			WIN32_FIND_DATA wfd;
			HANDLE h = ftex->FindFirstFile("*.*", &wfd);

			if (h != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (IFileSystem * fsub = OpenDirectory(wfd.cFileName, ftex))
					{
						SaveFile("U wrap mode", &clamp, 4, fsub);
						SaveFile("V wrap mode", &clamp, 4, fsub);		// damn ms spelling bug

						fsub->Release();

						printf("Processed.");
					}
				}
				while (ftex->FindNextFile(h, &wfd));
			}

			ftex->Release();
		}

		fmain->Release();
	}
}

S32 lowercase_materials(char * file)
{
    S32 mcount = 0;

    char  texture_name[15];

    if (IFileSystem * fmain = OpenDirectory(file, NULL))
    {
        if (IFileSystem * fmesh = OpenDirectory("openFLAME 3D N-mesh", fmain))
        {
            if (IFileSystem * fmat = OpenDirectory("Material library", fmesh))
            {
                WIN32_FIND_DATA wfd;
                HANDLE h = fmat->FindFirstFile("*.*", &wfd);

                while (fmat->FindNextFile(h, &wfd))
                {
                    if (IFileSystem * mat = OpenDirectory(wfd.cFileName, fmat))
                    {
                        sprintf(texture_name, "");
                        LoadFile("Texture name", &texture_name, 11, mat);
						strlwr(texture_name);
						SaveFile("Texture name", &texture_name, 11, mat);
					}
                }

                fmat->Release();
            }

            fmesh->Release();
        }
        
        fmain->Release();
    }

	printf("converted.");
    return mcount;
}

S32 list_materials(char * file)
{
    S32 mcount = 0;

    U32   identifier;
    float ambient[3], diffuse[3], specular[3], emission[3];
    float shininess, transparency;
    char  texture_name[15];

    if (IFileSystem * fmain = OpenDirectory(file, NULL))
    {
        if (IFileSystem * fmesh = OpenDirectory("openFLAME 3D N-mesh", fmain))
        {
            if (IFileSystem * fmat = OpenDirectory("Material library", fmesh))
            {
                WIN32_FIND_DATA wfd;
                HANDLE h = fmat->FindFirstFile("*.*", &wfd);

                while (fmat->FindNextFile(h, &wfd))
                {
                    if (IFileSystem * mat = OpenDirectory(wfd.cFileName, fmat))
                    {
                        sprintf(texture_name, "none");
                        
                        LoadFile("Material identifier", &identifier, 4, mat);
                        LoadFile("Ambient", &ambient, 12, mat);
                        LoadFile("Diffuse", &diffuse, 12, mat);
                        LoadFile("Specular", &specular, 12, mat);
                        LoadFile("Emission", &emission, 12, mat);
                        LoadFile("Shininess", &shininess, 4, mat);
                        LoadFile("Transparency", &transparency, 4, mat);
                        LoadFile("Texture name", &texture_name, 11, mat);
                        
                        printf("\nMaterial: %d    (texture: %s)\n", identifier, texture_name);
                        printf("    ambient:  (%1.3f %1.3f %1.3f)     diffuse:  (%1.3f %1.3f %1.3f)\n", ambient[0], ambient[1], ambient[2], diffuse[0], diffuse[1], diffuse[2]);
                        printf("    specular: (%1.3f %1.3f %1.3f)     emission: (%1.3f %1.3f %1.3f)\n", specular[0], specular[1], specular[2], emission[0], emission[1], emission[2]);
                        printf("    shininess: %1.3f     transparency: %1.3f\n", shininess, transparency);
                        mcount++;
                    }
                }

                fmat->Release();
            }

            fmesh->Release();
        }
        
        fmain->Release();
    }

    return mcount;
}


S32 set_default_materials(char * file)
{
    static float one = 1.0;
    static float default_one[3] = { 1.0, 1.0, 1.0 };
    static float default_zero[3] = { 0.0, 0.0, 0.0 };
    
    S32 mcount = 0;

    if (IFileSystem * fmain = OpenDirectory(file, NULL))
    {
        if (IFileSystem * fmesh = OpenDirectory("openFLAME 3D N-mesh", fmain))
        {
            if (IFileSystem * fmat = OpenDirectory("Material library", fmesh))
            {
                WIN32_FIND_DATA wfd;
                HANDLE h = fmat->FindFirstFile("*.*", &wfd);

                while (fmat->FindNextFile(h, &wfd))
                {
                    if (IFileSystem * mat = OpenDirectory(wfd.cFileName, fmat))
                    {
                        SaveFile("Ambient", default_one, sizeof(default_one), mat);
                        SaveFile("Diffuse", default_one, sizeof(default_one), mat);
                        SaveFile("Specular", default_zero, sizeof(default_zero), mat);
                        SaveFile("Emission", default_zero, sizeof(default_zero), mat);
                        SaveFile("Transparency", &one, sizeof(one), mat);

                        mcount++;
                    }
                }

                fmat->Release();
            }

            fmesh->Release();
        }
        
        fmain->Release();
    }

    return mcount;
}


S32 set_face_shading(char * file, BOOL32 set_flat, BOOL32 set_gouraud, BOOL32 clear_hidden)
{
	S32 fcount = 0;

    if (IFileSystem * fmain = OpenDirectory(file, NULL))
    {
        if (IFileSystem * fmesh = OpenDirectory("openFLAME 3D N-mesh", fmain))
        {
            if (IFileSystem * faces = OpenDirectory("Face groups", fmesh))
            {
                WIN32_FIND_DATA wfd;
                HANDLE h = faces->FindFirstFile("Group*", &wfd);

				do
				{
				    if (IFileSystem * fg = OpenDirectory(wfd.cFileName, faces))
                    {
						LoadFile("Face count", &fcount, 4, fg);

						FACE_PROPERTY * prop_list = new FACE_PROPERTY[fcount];
						LoadFile("Face property", prop_list, sizeof(FACE_PROPERTY) * fcount, fg);

						for (int i = 0; i < fcount; i++)
						{
							// slow
							if (set_flat)
							{
								prop_list[i] |= FLAT_SHADED;
								prop_list[i] &= ~SMOOTH_SHADED;
							}

							if (set_gouraud)
							{
								prop_list[i] |= SMOOTH_SHADED;
								prop_list[i] &= ~FLAT_SHADED;
							}

							if (clear_hidden)
							{
								prop_list[i] &= ~HIDDEN;
							}
						}

						SaveFile("Face property", prop_list, sizeof(FACE_PROPERTY) * fcount, fg);
    
						delete prop_list;
						prop_list = NULL;

						fg->Release();
					}
				}
                while (fmesh->FindNextFile(h, &wfd));
               
            faces->Release();
            }

            fmesh->Release();
        }
    
        fmain->Release();
    }
    
    return fcount;
}

void main(int argc, char *argv[])
{
	DACOM = DACOM_Acquire();
	
	if (!DACOM)
		fatal("Error: Failed to acquire DACOM.\n");

    start_dacom();
	
	if (argc < 3)
	{
		char instructions[2048];
		strcpy(instructions, "\nUsage: FIX3DB <file.3db> {options}\n\n");
		strcat(instructions, "Options:  -flat               set all faces to be flat shaded\n");
        strcat(instructions, "          -smooth             set all faces to be gouraud (smooth) shaded\n");
        strcat(instructions, "          -nohide             remove all 'hidden' flags on faces\n");
        strcat(instructions, "          -mdefault           reset all material attributes\n");
        strcat(instructions, "          -mlist              list all materials and their attributes\n");
		strcat(instructions, "          -strip              strip textures (if textures don't contain alpha)\n");
		strcat(instructions, "          -clamp              clamp all textures in library (wrapping mode)\n");
		strcat(instructions, "          -lowercase          convert all texture references to lowercase filenames\n");
//        strcat(instructions, "          -spin               rotate object 180 degrees around Z axis\n");
        
		fatal(instructions);
		
	}

    WIN32_FIND_DATA wf;
    HANDLE h = FindFirstFile(argv[1], &wf);

    if (h == INVALID_HANDLE_VALUE)
        fatal("\nError: File not found.\n");
    
    if (wf.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        fatal("\nError: File <%s> is read-only, can't open.\n", argv[1]); 

    S32 i = 2;
    
	do
	{
		char * infile = wf.cFileName;

		printf("%s: ", infile);

		if (!stricmp(argv[i], "-flat"))
		{
			printf("Set %d faces to flat shading.", set_face_shading(infile, TRUE, FALSE, FALSE));
		}
		else
		if (!stricmp(argv[i], "-smooth"))
		{
			printf("Set %d faces to smooth shading.", set_face_shading(infile, FALSE, TRUE, FALSE));
		}
		else
		if (!stricmp(argv[i], "-nohide"))
		{
			printf("Removed hidden flag from all (%d) faces.", set_face_shading(infile, FALSE, FALSE, TRUE));
		}
		else
		if (!stricmp(argv[i], "-mdefault"))
		{
			printf("Set %d materials to default values.", set_default_materials(infile));
		}
		else
		if (!stricmp(argv[i], "-mlist"))
		{
			list_materials(infile);
		}
		else
		if (!stricmp(argv[i], "-spin"))
		{
			spin_object(infile);
		}
		else
		if (!stricmp(argv[i], "-strip"))
		{
			strip_objects(infile);
		}
		else
		if (!stricmp(argv[i], "-clamp"))
		{
			clamp_textures(infile);
		}
		else
		if (!stricmp(argv[i], "-lowercase"))
		{
			lowercase_materials(infile);
		}

		printf("\n");
		
	}
	while (FindNextFile(h, &wf));

    DACOM->ShutDown();

}