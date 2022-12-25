#ifndef __DISKEYW_H
#define __DISKEYW_H
#include <TCHAR.H>
#include <STDLIB.H>
//Max Number of media keywords to look for.
#define MAX_DISK_LABELS 7

//Reserved DISK ID labels and numbers
#define DISK_NOT_SPECIFIED	-1
#define DISK_01		0
#define DISK_02		1
#define DISK_03		2
#define DISK_04		3
#define DISK_05		4
#define DISK_06		5
#define DISK_07		6
#define DISK_08		7

#define DISKPATH "\\disk"
#define DISKONEPATH "\\disk1"
#define DISKONESUFFIX '1'
#define DISKPREFIX "disk"

typedef struct tagDISKKEYWORD
{
	int		nDiskId;
    TCHAR   pszKeyword[_MAX_PATH];		//For Prepstub and Prepstub98
//	TCHAR   pszDiskPath[_MAX_PATH];		//For Network Installs and FileInfo Gathering
} DISKKEYWORD, *LPDISKKEYWORD;

extern DISKKEYWORD g_DiskKeywords[MAX_DISK_LABELS + 1];
#endif //__DISKKEYW_H
