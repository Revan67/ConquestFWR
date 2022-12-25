#ifndef STDWIDGET_H
#define STDWIDGET_H
//
// StdWidget.h - Standard widget library header.
//

//
// Design Notes:
//     This header declares the class for each of the widgets supplied by UniTool. These widgets are
// compiled into the tool itself, and are therefore always available.
//     The standard widgets are: Button, Label (static text), Image (static image), Text (text input),
// TopLevel (overlapped Win32 window), Frame (plain colored or transparent window), Scroll (scroll bar),
// List (list box), and Combo (combo box).
//

//
// Include files
//

#include "WinWidget.h"

//
// Class and structure definitions
//

struct ButtonWidget : public BaseWidget
{
protected:
	HWND buttonWnd;  // The actual button control window, kept the same size as the main window

public:
	ButtonWidget();
	~ButtonWidget();
	
	// Simple type definitions
	enum ButtonType
	{
		BTN_PUSH,
		BTN_RADIO,
		BTN_CHECK
	};

	// Creation methods
	bool create (int w, int h, const char *text, HWND parent, ButtonType type=BTN_PUSH);

	// BaseWidget overloads
	virtual LRESULT handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual bool on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight); // WM_SIZE
	virtual bool on_command (WORD wNotifyCode, WORD wID, HWND hwndCtl); // WM_COMMAND

	// === Supported Interfaces === 
	// IWidget Interface
	virtual SIZE get_prefsize();

	// IEventSource Interface, inherited from IWidget
	virtual EventId event_count(void);
	virtual const char *event_name(EventId index);
};

struct LabelWidget : public BaseWidget
{
protected:
	HWND labelWnd;  // The actual control window, kept the same size as the main window

public:
	LabelWidget();
	~LabelWidget();
	
	// Creation methods
	bool create (int w, int h, const char *text, HWND parent);

	// BaseWidget overloads
	virtual LRESULT handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual bool on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight); // WM_SIZE

	// === Supported Interfaces === 
	// IWidget Interface
	virtual SIZE get_prefsize();
};

struct ImageWidget : public BaseWidget
{
protected:
	HWND imageWnd;  // The actual control window, kept the same size as the main window

public:
	ImageWidget();
	~ImageWidget();
	
	// Creation methods
	bool create (int w, int h, const char *text, HWND parent);

	// Image manipulation methods
	bool set_image (const char *filename);
	bool set_image (HANDLE hNewImage);  // takes ownership of the given handle
    HANDLE get_image ();

	// BaseWidget overloads
	virtual bool on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight); // WM_SIZE

	// === Supported Interfaces === 
	// IWidget Interface
	virtual SIZE get_prefsize();

	// IScriptable Interface, inherited from IWidget
	virtual int method_count(void);
	virtual const char *method_name(int index);
	virtual int method_speclen(int index);
	virtual const ParamSpec *method_spec (int index);
	virtual bool invoke (const char *methodName, Variant *result, int paramCount, Variant *params);
	virtual bool invokeByIndex (int index, Variant *result, int paramCount, Variant *params);
};

struct TopLevelWidget : public BaseWidget
{
protected:
public:
	TopLevelWidget ();
	~TopLevelWidget ();

	// Creation methods
	bool create (int x, int y, int w, int h, const char *title);

	// Menu related methods
	HMENU get_menu ();
	void set_menu (HMENU hNewMenu);

	// BaseWidget overloads
	virtual bool on_menu (WORD wID);  // WM_COMMAND, wNotifyCode == 0
	virtual bool on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight); // WM_SIZE
	virtual bool on_buttondown (MouseFlags fMouse, WPARAM fwKeys, WORD xPos, WORD yPos);  // WM_{L,R,M}BUTTONDOWN
	virtual bool on_buttonup (MouseFlags fMouse, WPARAM fwKeys, WORD xPos, WORD yPos);    // WM_{L,R,M}BUTTONUP
	virtual void on_mousemove (WPARAM fwKeys, WORD xPos, WORD yPos);   // WM_MOUSEMOVE

	virtual void set_size (int w, int h);
	virtual SIZE get_size();

	// === Supported Interfaces === 

	// IScriptable Interface, inherited from IWidget
	virtual int method_count(void);
	virtual const char *method_name(int index);
	virtual int method_speclen(int index);
	virtual const ParamSpec *method_spec (int index);
	virtual bool invoke (const char *methodName, Variant *result, int paramCount, Variant *params);
	virtual bool invokeByIndex (int index, Variant *result, int paramCount, Variant *params);

	// IEventSource Interface, inherited from IWidget
	virtual EventId event_count(void);
	virtual const char *event_name(EventId index);
};

struct TextWidget : public BaseWidget
{
protected:
	HWND textWnd;

public:
	TextWidget();
	~TextWidget();
	
	// Creation methods
	bool create (int w, int h, const char *text, HWND parent);

	// BaseWidget overloads
	virtual LRESULT handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual bool on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight); // WM_SIZE
	virtual bool on_command (WORD wNotifyCode, WORD wID, HWND hwndCtl); // WM_COMMAND

	// === Supported Interfaces === 

	// IScriptable Interface, inherited from IWidget
#if 0
	virtual int method_count(void);
	virtual const char *method_name(int index);
	virtual int method_speclen(int index);
	virtual const ParamSpec *method_spec (int index);
	virtual bool invoke (const char *methodName, Variant *result, int paramCount, Variant *params);
	virtual bool invokeByIndex (int index, Variant *result, int paramCount, Variant *params);

#endif
	// IEventSource Interface, inherited from IWidget
	virtual EventId event_count(void);
	virtual const char *event_name(EventId index);
};

struct FrameWidget : public BaseWidget
{
protected:
public:
	FrameWidget ();
	~FrameWidget ();

	// Creation methods
	bool create (int w, int h, HWND parent);

	// === Supported Interfaces === 
	// IWidget Interface
//	virtual SIZE get_prefsize();

	// IScriptable Interface, inherited from IWidget
#if 0
	virtual int method_count(void);
	virtual const char *method_name(int index);
	virtual int method_speclen(int index);
	virtual const ParamSpec *method_spec (int index);
	virtual bool invoke (const char *methodName, Variant *result, int paramCount, Variant *params);
	virtual bool invokeByIndex (int index, Variant *result, int paramCount, Variant *params);

	// IEventSource Interface, inherited from IWidget
	virtual EventId event_count(void);
	virtual const char *event_name(EventId index);
#endif
};

struct ScrollWidget : public BaseWidget
{
protected:
	HWND scrollWnd;

public:
	ScrollWidget ();
	~ScrollWidget ();

	// Simple type definitions
	enum ScrollType
	{
		SCR_VERTICAL,
		SCR_HORIZONTAL
	};

	// Creation methods
	bool create (int w, int h, HWND parent, ScrollType type=SCR_HORIZONTAL);

	// Scroll related methods, also exported via invoke.
	void set_scroll_range (int min, int max);
	int get_scroll_min();
	int get_scroll_max();
	void set_scroll_pos (int pos);
	int get_scroll_pos ();
	void set_scroll_page (unsigned int pagelen);
	unsigned int get_scroll_page();

	// BaseWidget overloads
	virtual LRESULT handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual bool on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight); // WM_SIZE

	// === Supported Interfaces === 
	// IScriptable Interface, inherited from IWidget
	virtual int method_count(void);
	virtual const char *method_name(int index);
	virtual int method_speclen(int index);
	virtual const ParamSpec *method_spec (int index);
	virtual bool invoke (const char *methodName, Variant *result, int paramCount, Variant *params);
	virtual bool invokeByIndex (int index, Variant *result, int paramCount, Variant *params);

	// IEventSource Interface, inherited from IWidget
	virtual EventId event_count(void);
	virtual const char *event_name(EventId index);
};

struct ListWidget : public BaseWidget
{
protected:
	HWND   listWnd;
	char * itemTextBuffer;   // pointer to the cached text of last get_item_string
	int *  itemIndexBuffer;  // pointer to the cached selection list.
	int    itemIndexLen;     // length of itemIndexBuffer, in elements (i.e. bytes == itemIndexLen * sizeof(int))

public:
	ListWidget ();
	~ListWidget ();

	// Simple type definitions
	enum ListType
	{
		LST_NOSEL,
		LST_SEL,
		LST_MULTISEL
	};

	// Creation methods
	bool create (int w, int h, HWND parent, ListType type=LST_SEL, bool sort=true);

	// List methods
	int add_string (const char *string);
	bool del_string (int index);
	bool del_named_string (const char *string);
	int find_string (const char *string); // exact match
	int find_partial_string (const char *prefix); // string that starts with prefix
	const char *get_item_string (int index);
	DWORD get_item_data (int index);
	int get_item_count ();
	bool set_select (int count, const int *indices, bool set=true); // takes indices[count] of desired selections
	bool set_single_select (int index, bool set=true); // sets a single selection. Same as set_select(1, &index, set)
	int get_select_count ();
	const int *get_select (); // returns array of selected indices, get_select_count() long.
	int get_single_select (int index);  // returns get_select()[index].
	void reset_content ();

	// BaseWidget overloads
	virtual LRESULT handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual bool on_command (WORD wNotifyCode, WORD wID, HWND hwndCtl); // WM_COMMAND
	virtual bool on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight); // WM_SIZE
	virtual bool on_paint (HDC hdc); // WM_PAINT
	virtual bool on_erasebkgnd (HDC hdc); // WM_ERASEBKGND

	// === Supported Interfaces === 
#if 0
	// IWidget Interface
	virtual SIZE get_prefsize();
#endif

	// IScriptable Interface, inherited from IWidget
	virtual int method_count(void);
	virtual const char *method_name(int index);
	virtual int method_speclen(int index);
	virtual const ParamSpec *method_spec (int index);
	virtual bool invoke (const char *methodName, Variant *result, int paramCount, Variant *params);
	virtual bool invokeByIndex (int index, Variant *result, int paramCount, Variant *params);

	// IEventSource Interface, inherited from IWidget
	virtual EventId event_count(void);
	virtual const char *event_name(EventId index);
};

struct ComboWidget : public BaseWidget
{
protected:
	HWND comboWnd;
	char * itemTextBuffer;   // pointer to the cached text of last get_item_string
	int *  itemIndexBuffer;  // pointer to the cached selection list.
	int    itemIndexLen;     // length of itemIndexBuffer, in elements (i.e. bytes == itemIndexLen * sizeof(int))

public:
	ComboWidget ();
	~ComboWidget ();

	// Simple type definitions
	enum ComboType
	{
		CMB_SIMPLE,
		CMB_DROPBOX,
		CMB_DROPLIST
	};

	// Creation methods
	bool create (int w, int h, HWND parent, ComboType type=CMB_SIMPLE, bool sort=true);

	// Combo methods.
	// NOTE: These function the same way they do for a ListWidget
	int add_string (const char *string);
	bool del_string (int index);
	bool del_named_string (const char *string);
	int find_string (const char *string); // exact match
	int find_partial_string (const char *prefix); // string that starts with prefix
	const char *get_item_string (int index);
	DWORD get_item_data (int index);
	int get_item_count ();
	void set_item_data (int index, DWORD value);
	bool set_single_select (int index, bool set=true); // sets a single selection in the list box
	int get_single_select ();  // returns the index of the current selection.

	// BaseWidget overloads
	virtual LRESULT handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual bool on_command (WORD wNotifyCode, WORD wID, HWND hwndCtl); // WM_COMMAND
	virtual bool on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight); // WM_SIZE
	virtual bool on_paint (HDC hdc); // WM_PAINT
	virtual bool on_erasebkgnd (HDC hdc); // WM_ERASEBKGND

	// === Supported Interfaces === 
#if 0
	// IWidget Interface
	virtual SIZE get_prefsize();
#endif

	// IScriptable Interface, inherited from IWidget
	virtual int method_count(void);
	virtual const char *method_name(int index);
	virtual int method_speclen(int index);
	virtual const ParamSpec *method_spec (int index);
	virtual bool invoke (const char *methodName, Variant *result, int paramCount, Variant *params);
	virtual bool invokeByIndex (int index, Variant *result, int paramCount, Variant *params);

	// IEventSource Interface, inherited from IWidget
	virtual EventId event_count(void);
	virtual const char *event_name(EventId index);
};

/*
struct TreeWidget : public BaseWidget
{
protected:
	HWND treeWnd;

public:
	TreeWidget ();
	~TreeWidget ();

	// Creation methods
	bool create (int w, int h, HWND parent, ComboType type=CMB_SIMPLE, bool sort=true);

	// BaseWidget overloads
	virtual LRESULT handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual bool on_command (WORD wNotifyCode, WORD wID, HWND hwndCtl); // WM_COMMAND
	virtual bool on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight); // WM_SIZE
	virtual bool on_paint (HDC hdc); // WM_PAINT
	virtual bool on_erasebkgnd (HDC hdc); // WM_ERASEBKGND

	// TreeView methods.
	// NOTE: I intentionally left out a lot of the tree view's functionality to make this
	// class more simple.
	
	HTREEITEM add_item (const char *name, HTREEITEM parent, DWORD appData);
	bool del_item (HTREEITEM hItem);
	bool set_item_text (HTREEITEM hItem, const char *text);
	const char *get_item_text (HTREEITEM hItem);
	bool set_item_data (HTREEITEM hItem, DWORD data);
	DWORD get_item_data (HTREEITEM hItem);
	HTREEITEM get_parent (HTREEITEM hItem);
	HTREEITEM get_child (HTREEITEM hItem);
	HTREEITEM get_next_sibling (HTREEITEM hItem);
	bool make_visible (HTREEITEM hItem);
	bool is_selected (HTREEITEM hItem);
	HTREEITEM get_selected ();
	bool select (HTREEITEM hItem);

	// In LUA, the widget will be implemented as a normal widget table, but with a special member called "tree",
	// whose value must be a table with the TREENODE_TAG tag.  A TREENODE_TAG table has the following special members:
	// *** Members ***
	// _treechild = the first child TREENODE_TAG table of this node
	// _treeparent = the parent TREENODE_TAG table of this node
	// _treesibling = the sibling TREENODE_TAG table of this node
	// _treename = the name of this TREENODE_TAG table, displayed in the tree view
	// _treevalue = the numeric value of this TREENODE_TAG table. Converted to DWORD on C++ side.
	// _treehandle = the HTREEITEM handle of this item in the tree widget.
	// _treeroot = a reference to the root of the TreeWidget to which the node belongs.
	// *** Events ***
	// OnDoubleClick = a function that is called when the item is double clicked.
	// OnExpand = a function this is called when the item is expanded
	// OnCollapse = a function that is called when the item is collapsed
	// OnSelect = a function that is called when the item is selected
	// OnUnSelect = a function that is called when the item loses the selection
	// *** Methods ***
	// newChild = a function that adds a child TREENODE_TAG to the called table and returns it.
	// delete = a function that destroys all its children, then removes itself from its parent's list.
	// makeVisible = a function that will make the given item visibile in the tree view by expanding and scrolling
	// expand = a function that will expand or collapse the given item
	// isSelected = a function that will return if the given item is selected or not
	// select = a function that will cause the given item to become selected.
	//
	// The following TREENODE_TAG tag methods are defined:
	// settable = if the value being set is one of the settable values (_treename and _treevalue), the new value is
	// set in the treeview as well before being stored in the table, i.e. the view is kept in sync with the table.
	//
	// NOTE 1: It is explicitly supported that any other members can be added to a TREENODE_TAG table. The general way of
	// using TreeWidgets is to create one, add all of its children as needed, then perform operations on the table.
	// We could also allow you to build the table first, using the special members, then to pass it to the table
	// creation call, which would take the table, create a tree view matching it, and store the appropriate values
	// into the table to make them work together.
	//
	// NOTE 2: The event handling differs a bit on this widget from the other widgets. The 

	// === Supported Interfaces === 
#if 0
	// IScriptable Interface, inherited from IWidget
	virtual int method_count(void);
	virtual const char *method_name(int index);
	virtual int method_speclen(int index);
	virtual const ParamSpec *method_spec (int index);
	virtual bool invoke (const char *methodName, Variant *result, int paramCount, Variant *params);
	virtual bool invokeByIndex (int index, Variant *result, int paramCount, Variant *params);
#endif

	// IEventSource Interface, inherited from IWidget
	virtual EventId event_count(void);
	virtual const char *event_name(EventId index);
};
*/

//
// Global routines
//

extern void init_standard_widgets ();

#endif
