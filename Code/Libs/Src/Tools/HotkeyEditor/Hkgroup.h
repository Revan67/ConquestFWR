#ifndef HKGROUP_H
#define HKGROUP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               HKGroup.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author:   JYENAWINE  $
*/			    
//--------------------------------------------------------------------------//
//------------------------------- #INCLUDES --------------------------------//

#define   MSG_BEGIN_REC	   0x1000
#define   MSG_END_REC	   0x1001

struct HKRECORD 
{
	char szName[64];
	char szKeyCombo[64];
	char szDescription[64];
	DWORD dwKeyCombo[4];
	DWORD dwAssignedNumber;
	static int iSortColumn;

	HKRECORD (void)
	{
		memset(this, 0, sizeof(*this));
	}

	int compare (HKRECORD & rec);

	BOOL operator > (HKRECORD & rec)
	{
	 	return (compare(rec) > 0);
	}

	BOOL operator < (HKRECORD & rec)
	{
	 	return (compare(rec) < 0);
	}

	BOOL operator == (HKRECORD & rec)
	{
	 	return (compare(rec) == 0);
	}
};

enum ACTION
{
	Invalid=0,
	Insert,
	Delete,
	Replace
};

class HKGROUP;

class HKGROUP
{
	HKGROUP * next;
public:
	struct IHotkeyRecorder * hkmanager;
	HWND    hDlg;				// handle to child Dialog window
	HWND	hRecWnd;
	struct DAHOTKEY_STRUCT * hHKey;				// handle to HotKey;
	BOOL 	bRecording;
	BOOL 	bStatus;			// 0==OK, 1==OK_PRESSED, 2==CANCEL_PRESSED
	DWORD	dwEditItem;
	char	szText[256];
	HKRECORD hkrecord;
	ACTION  action;
		

	HKGROUP (void);

	~HKGROUP (void);

	BOOL init (HWND hParent, HKRECORD *lpRec, DWORD dwItem, ACTION _action);

	BOOL update (void);

	static BOOL Update (void);		// update all 	

	static BOOL Translate (MSG & msg);	// return TRUE if message handled

	static BOOL IsBeingEdited (DWORD dwItem);	// bring box to the surface if true
	
	static BOOL InsertUnderway (void);

	static BOOL NewRecordWasInserted (DWORD dwNum);

	static BOOL RecordWasDeleted (DWORD dwNum);

	static BOOL AnyEditUnderway (void);
};


#endif
