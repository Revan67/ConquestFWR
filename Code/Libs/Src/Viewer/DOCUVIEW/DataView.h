#ifndef DATAVIEW_H
#define DATAVIEW_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DataView.H                                 //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Ajackson $
*/			    
//--------------------------------------------------------------------------//


#ifndef VIEWER_H
#include "Viewer.h"
#endif

#ifndef IDOCCLIENT_H
#include "IDocClient.h"
#endif

#ifndef DOCUMENT_H
#include "Document.h"
#endif

#ifndef SYMTABLE_H
#include "SymTable.h"
#endif

#ifndef TCOMPONENT_H
#include "TComponent.h"
#endif

#ifndef TDISPATCH_H
#include "TDispatch.h"
#endif

#ifndef TSMARTPOINTER_H
#include "TSmartPointer.h"
#endif

#ifndef ISTRUCTENUMERATOR_H
#include "IStructEnumerator.h"
#endif

#ifndef ISTRINGSET_H
#include "IStringSet.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#define MAX_NAME_LENGTH 128


struct DACOM_NO_VTABLE IDataViewer : public IViewer
{
	DEFMETHOD(set_display_value) (const C8 *name) = 0;

	DEFMETHOD(get_display_value) (C8 *name) = 0;

	DEFMETHOD(set_auto_close) (BOOL32 value) = 0;

	DEFMETHOD(get_auto_close) (BOOL32 *value) = 0;

	DEFMETHOD(set_hex_numbers) (BOOL32 value) = 0;

	DEFMETHOD(get_hex_numbers) (BOOL32 *value) = 0;

};
//--------------------------------------------------------------------------//
//


//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE DataViewer : public Dispatch<DataViewer, IDataViewer>, IDocumentClient
{
public:
	enum PROP
	{
		DISPLAY_STATE,
		DISPLAY_VALUE,
		CLASS_NAME,
		INSTANCE_NAME,
		PARENT_WINDOW,
		MAIN_WINDOW,
		WINDOW_RECT,
		READ_ONLY_DATA,
		STRING_LENGTH,
		AUTO_CLOSE,
		HEX_NUMBERS
	};

	BEGIN_DACOM_MAP_INBOUND(DataViewer)
	DACOM_INTERFACE_ENTRY(IViewer)
	DACOM_INTERFACE_ENTRY(IDADispatch)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY2(IID_IViewer,IViewer)
	DACOM_INTERFACE_ENTRY2(IID_IDADispatch,IDADispatch)
	DACOM_INTERFACE_ENTRY2(IID_IDocumentClient,IDocumentClient)
	END_DACOM_MAP()

	char szReturnString[MAX_NAME_LENGTH];
	char szDisplayName[MAX_NAME_LENGTH];
	char szClassName[MAX_NAME_LENGTH];
	char szInstanceName[MAX_NAME_LENGTH];
	HWND hParentWindow;
	HWND hMainWindow;
	HWND hListView, hEdit, hCombo;
	RECT rect;
	DWORD offset;	// offset inside the larger structure
	BOOL32 bVisible, bAutoClose, bChildControl, bTopMost;
	BOOL32 bWinDataChanged;
	char *pLastData;
	DWORD dwLastDataSize;
	int iMinHeight;
	int bPaintCount;
	bool bExtraReference;
	bool bMenuItemAdded;		// true when GDI menu item has been added, for zero length structs
	U32 userModifiers, symModifiers;

	IDocument * doc;
	struct Symbol *symbol;
	U32 connHandle;

	DataViewer *pViewerList;
	DataViewer *pNext;
	DataViewer *pParentViewer;

	IStringSet * stringSet;

#if !defined(_XBOX)
	WNDPROC lpfnOldEditProcedure;
#endif

	BEGIN_DACOM_DISPATCH_MAP(DataViewer)
	DACOM_DISPATCH_METHOD(set_display_state,  DAVT_BOOL32)
	DACOM_DISPATCH_METHOD(get_display_state,  DAVT_BOOL32|DAVT_BYREF )
	DACOM_DISPATCH_METHOD(get_class_name,     DAVT_STRING)
	DACOM_DISPATCH_METHOD(set_instance_name,  DAVT_STRING)
	DACOM_DISPATCH_METHOD(get_instance_name,  DAVT_STRING)
	DACOM_DISPATCH_METHOD(get_main_window,	  DAVT_PVOID|DAVT_BYREF)
	DACOM_DISPATCH_METHOD(set_rect,			  DAVT_PRECT)
	DACOM_DISPATCH_METHOD(get_rect,			  DAVT_PRECT)
	DACOM_DISPATCH_METHOD(set_read_only,	  DAVT_BOOL32)
	DACOM_DISPATCH_METHOD(get_read_only,	  DAVT_PBOOL32)
	DACOM_DISPATCH_METHOD(set_display_value,  DAVT_STRING)
	DACOM_DISPATCH_METHOD(get_display_value,  DAVT_STRING)
	DACOM_DISPATCH_METHOD(set_hex_numbers,	  DAVT_BOOL32)
	DACOM_DISPATCH_METHOD(get_hex_numbers,	  DAVT_PBOOL32)
	DACOM_DISPATCH_METHOD(Enumerate,		  DAVT_COMPONENT)
	DACOM_DISPATCH_METHOD(Update,			  DAVT_COMPONENT)
	DACOM_DISPATCH_METHOD(set_spelling_list,  DAVT_COMPONENT)
	DACOM_DISPATCH_METHOD(set_viewer_path,    DAVT_STRING)
	DACOM_DISPATCH_MEMBER_PROPERTY("auto_close", bAutoClose, DAVT_BOOL32)
	DACOM_DISPATCH_MEMBER_PROPERTY("top_most", bTopMost, DAVT_BOOL32)
	END_DACOM_DISPATCH_MAP()


	//----------------------------------------------------------
	//----------------------------------------------------------

	DataViewer (void)
	{
  		rect.left = 300;
		rect.top = 200;
		rect.right = 650;
		rect.bottom = 550;
		stringSet = NULL;
	}

	void * operator new (size_t size);		// calls calloc()

	~DataViewer (void);

	/* IViewer members */
	
	DEFMETHOD(set_display_state) (BOOL32 state);

	DEFMETHOD(get_display_state) (BOOL32 *state);

	DEFMETHOD(get_class_name) (C8 *name);

	DEFMETHOD(set_instance_name) (const C8 *name);

	DEFMETHOD(get_instance_name) (C8 *name);

	DEFMETHOD(get_main_window) (void **hwnd);

	DEFMETHOD(set_rect) (const struct tagRECT *rect);

	DEFMETHOD(get_rect) (struct tagRECT *rect);

	DEFMETHOD(set_read_only) (BOOL32 value);

	DEFMETHOD(get_read_only) (BOOL32 *value);

	DEFMETHOD(set_display_value) (const C8 *name);

	DEFMETHOD(get_display_value) (C8 *name);

	DEFMETHOD(set_auto_close) (BOOL32 value);

	DEFMETHOD(get_auto_close) (BOOL32 *value);

	DEFMETHOD(set_hex_numbers) (BOOL32 value);

	DEFMETHOD(get_hex_numbers) (BOOL32 *value);

	DEFMETHOD(set_spelling_list) (IDAComponent * set);

	DEFMETHOD(set_viewer_path) (const char *path);

	/* IDocumentClient members */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message=0, void *parm=0);

	DEFMETHOD(OnClose) (struct IDocument *doc);

	/* DataViewer members */

	BOOL MainDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	BOOL32 init (void);

	void onAddRequested (void);	// user has requested to increase size of zero based array
	void addToFile (U32 bytesToAdd);

	BOOL32 WriteNewData (void);

	BOOL SetListViewHeader (void);

	BOOL UpdateListView (void);

	BOOL32 UpdateCombo (void);

	BOOL32 UpdateEdit (void);

	BOOL32 UpdateView (void);

	BOOL32 CreateView (void);

	BOOL32 initArray (void);

	BOOL32 initRecord (void);

	BOOL32 initType (void);

	BOOL32 initPointer (void);

	BOOL32 UpdateData (const char *pData, U32 dwSize);

	BOOL32 ViewChild (int i, bool skipModifiers = false);

	BOOL32 ClearChild(int i);

	BOOL32 JumpToChild (int i);

	BOOL32 ModifyChild( int i, int val );

	BOOL32 InitCombo (void);

	BOOL32 InitEdit (void);

	BOOL32 SetNewReadOnly (BOOL32 newVal);

	BOOL32 SetNewHexValue (BOOL32 newVal);

	BOOL32 SetNewSpellCheckValue (BOOL32 newVal);

	BOOL32 SubstituteInstanceName (const char *pName);

	BOOL32 SetDataDirectory(const char *pDirName);

	GENRESULT __stdcall Enumerate (IDAComponent * enumerator)
	{
		GENRESULT result = GR_INVALID_PARMS;
		if (enumerator)
		{
			COMPTR<IStructEnumerator> _enumerator;

			if (enumerator->QueryInterface(IID_IStructEnumerator, _enumerator)==GR_OK)
			{
				result = enumerate(_enumerator);
			}
		}
		return result;
	}

	GENRESULT enumerate (struct IStructEnumerator * enumerator);
	GENRESULT enumerateBasic(struct IStructEnumerator *enumerator);
	GENRESULT enumerateArray(struct IStructEnumerator *enumerator);
	GENRESULT enumerateRecord(struct IStructEnumerator *enumerator);

	GENRESULT __stdcall Update (IDAComponent * _doc)
	{
		GENRESULT result = GR_INVALID_PARMS;
		if (_doc)
		{
			COMPTR<IDocument> doc;

			if (_doc->QueryInterface(IID_IDocument, doc)==GR_OK)
			{
				result = update(doc);
			}
		}
		return result;
	}

	GENRESULT update (IDocument * doc);

	static const char * __stdcall Dereference (SYMBOL sym, const char *pData);

	static BOOL32 __stdcall IsStringBaseType (SYMBOL sym, U32 modifiers);	// char or wchar_t
	static SYMBOL __stdcall GetType (SYMBOL sym);

	static int GetBitField32 (const char *pData, int bit_offset, int bit_count);
	static __int64 GetBitField64 (const char *pData, int bit_offset, int bit_count);

	static int GetBitFieldU32 (const char *pData, int bit_offset, int bit_count);
	static unsigned __int64 GetBitFieldU64 (const char *pData, int bit_offset, int bit_count);

	static BOOL32 __stdcall _GetSymTypeName (SYMBOL sym, U32 modifiers, char *value);
	static LONG CALLBACK EditControlProcedure(HWND hwnd, UINT message, UINT wParam, LONG lParam);

	static BOOL32 GetSymValue (SYMBOL sym, U32 modifiers, const char *pData, char *value, U32 symbolSize, IStringSet * set);

	static BOOL32 SetSymValue (SYMBOL sym, char *pData, const char *value, U32 symbolSize);

	static int SetBitField32 (const char *pData, int bit_offset, int bit_count, U32 value);

	static __int64 SetBitField64 (const char *pData, int bit_offset, int bit_count, unsigned __int64 value);

	static BOOL32 __stdcall GetSymTypeName (SYMBOL sym, U32 modifiers, char *value);

	static BOOL CALLBACK StaticDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	static BOOL32 CopySymValue (SYMBOL src, U32 srcModifiers, const char *srcData, 
									 SYMBOL dst, U32 dstModifiers, char *dstData);

	static BOOL32 IsEqual (SYMBOL oldSymbol, SYMBOL newSymbol);

	static BOOL32 HasVariableSize (SYMBOL symbol);

	static void ConvertToString8 (U8 value, char * string);

	static void ConvertToString16 (U16 value, char * string);

	static void ConvertToString32 (U32 value, char * string);

	static void ConvertToString64 (U64 value, char * string);

	bool isReadOnly (void)
	{
		return (((userModifiers|symModifiers|symbol->modifiers) & MODIFIER_READONLY) != 0);
	}

	IDAComponent * BaseComponent (void)
	{
		return (IDAComponent *) (daoffsetofclass(IViewer, DataViewer) + ((U32) this));
	}
};

#define WM_DTV_SET_DATA_PATH (WM_USER+2)

//--------------------------------------------------------------------------//
//----------------------------End DataView.h--------------------------------//
//--------------------------------------------------------------------------//

#endif