#ifndef DAUI_H
#define DAUI_H
//
// DAUI.H - DA 2D User Interface Header File
//

//
// Include files
//

#include <dacom.h>

//
// Macros
//

#define DAMETHOD virtual GENRESULT COMAPI
#define DAMETHOD_(type) virtual type COMAPI

//
// Interface definitions
//

// Basic control class. Size, position are supported.
// It is important to not that IControls don't directly nest; controls can only be the children
// of an IControlParent. However, an object can implement both the control and parent APIs, forming a control that
// can contain other controls.
struct DACOM_NO_VTABLE IControl : public IDAComponent
{
public:
	// Size and position API
	DAMETHOD set_rect (int x1, int y1, int x2, int y2) = 0;  // exclusive of right and bottom
	DAMETHOD get_rect (int *x1, int *y1, int *x2, int *y2) = 0;

	// General control properties
	DAMETHOD set_enabled (bool enabled) = 0;     // true == enable, false == disable
	DAMETHOD get_enabled (bool *enabled) = 0;

	DAMETHOD set_focused (bool focused) = 0;     // true == accepts keyboard, false == ignored keyboard
	DAMETHOD get_focused (bool *focused) = 0;

	DAMETHOD set_visible (bool visible) = 0;     // true == potentially visible, false == invisible
	DAMETHOD get_visible (bool *visible) = 0;

	DAMETHOD set_default (bool isDefault) = 0;   // true == default control, false == otherwise 
	DAMETHOD get_default (bool *isDefault) = 0;

	// Highlight and selected have different meaning. Highlight is when the mouse moves over an item and is
	// therefore temporary. Selection is persistant.
	DAMETHOD set_highlight (bool highlight) = 0; // true == brighter, false == normal
	DAMETHOD get_highlight (bool *highlight) = 0;

	DAMETHOD set_selected (bool selected) = 0;   // true == brighter, false == normal
	DAMETHOD get_selected (bool *selected) = 0;

	// This flag is used to indicate that the state of the control has changed. This allows for
	// optimized drawing.
	DAMETHOD clear_control_changed () = 0;            // resets the changed flag to false
	DAMETHOD get_control_changed (bool *changed) = 0; // true if the control's state has changed since the last clear

	// This allows a control to carry around some application specific data.
	// It should be noted that the control will not free any data it holds here, so you should be
	// careful.
	DAMETHOD set_app_data (void *data) = 0;
	DAMETHOD get_app_data (void **data) = 0;

	// Controls need unique identifiers so that handlers can determine the origin of control messages.
	DAMETHOD set_id (unsigned long id) = 0;
	DAMETHOD get_id (unsigned long *id) = 0;
	DAMETHOD set_name (const char *name) = 0;
	DAMETHOD get_name (char *name, int bufferlen) = 0;
};

// Controls can only be contained in a parent. The parent does two things: it adjusts its child controls
// as needed, and automatically hooks up event callbacks between child and parent objects, if the parent
// supports IEventCallback and the child has a connection point for that interface.
struct DACOM_NO_VTABLE IControlParent : public IDAComponent
{
public:
	// Containment API
	DAMETHOD add_child (IControl *which) = 0;     // makes the given control a child of this parent
	DAMETHOD remove_child (IControl *which) = 0;  // orphans the given control from this parent

	// Aggregators of a control parent must be able to access the list of children.
	// For example, a GraphicFrame object would need to draw all of its children. If the list were
	// not available, it would not be able to tell the children to draw.
	DAMETHOD get_child_count (int *count) = 0;
	DAMETHOD get_child (int index, IControl **which) = 0;

	// While these are not strictly needed, these operations are more efficient when done by the
	// actual container.

	DAMETHOD get_child_index (IControl *which, int *index) = 0; // index < 0 if control is not contained.
};


//
// Design Notes:
//     What makes a button a button is that it processes mouse events, is is a control, sends
// button events, and has button properties. Processing events comes from IEventCallback, control properties
// come from IControl, sending events comes from IEventCallback connection points, and button properties come
// from IButton. A BaseControl implementation that does the control and connection point handling will make
// writing the common controls easier.
//     To make a real button, the game programmer will aggregate a Button component into a component that handles
// drawing the button. The behavior comes for free, while the drawing is left up to the programmer. The same
// goes for frames and other controls: the stuff you don't want to write is done for you, while the stuff you
// do need to write is completely in your control.

// Button Interface. Buttons uniquely have pressed and released (down and up) states. They also have different
// behaviors: push button (pressed only while the mouse is down on them), and two-state (push once for down,
// push again for up). Their press state can be set independent of mouse processing.

struct DACOM_NO_VTABLE IButton : public IDAComponent
{
public:
	enum ButtonType
	{
		BUTTON_PUSH,
		BUTTON_TOGGLE
	};

	// Button states
	DAMETHOD set_pressed (bool pushed) = 0; // true == 'down', false == 'up'
	DAMETHOD get_pressed (bool *pushed) = 0;

	DAMETHOD set_type (ButtonType type) = 0;
	DAMETHOD get_type (ButtonType *type) = 0;
};

struct DACOM_NO_VTABLE IEditBox : public IDAComponent
{
public:
	// Edit box states
	DAMETHOD set_text (const char *text) = 0;
	DAMETHOD get_text (char *buffer, int bufferLength, int *length) = 0;
	DAMETHOD get_text_length (int *length) = 0;

	DAMETHOD set_caret (int index) = 0;
	DAMETHOD get_caret (int *index) = 0;

	DAMETHOD set_selection (int start, int end) = 0;
	DAMETHOD get_selection (int *start, int *end) = 0;

	DAMETHOD get_selected_text (char *buffer, int bufferLength, int *length) = 0;
	DAMETHOD set_selected_text (const char *buffer) = 0;

	DAMETHOD clear_text_changed () = 0;
	DAMETHOD get_text_changed (bool *changed) = 0;

	// *** Should some word wrapping stuff be here?
};

struct DACOM_NO_VTABLE ISlider : public IDAComponent
{
public:
	// Slider and scroll bar stuff
	enum SliderType
	{
		SLIDER_VERT,
		SLIDER_HORZ
	};

	DAMETHOD set_type (SliderType type) = 0;
	DAMETHOD get_type (SliderType *type) = 0;

	// These methods determine the logical range of the scrollbar.
	// This allows the control to automatically map between the visual size of the slider and the
	// range which it is controlling.
	// While this it not strictly needed in this control, you end up doing the same sort of thing in
	// the app code. Doing it here means that your code can be written to the logical coordinates and
	// will therefore automatically work if the pixel size of the slider changes.

	DAMETHOD set_range (int start, int end) = 0;           // logical range of the active area
	DAMETHOD get_range (int *start, int *end) = 0;

	DAMETHOD set_thumb_range (int pos, int length) = 0;    // logical range of thumb
	DAMETHOD get_thumb_range (int *pos, int *length) = 0;

	// These methods determine the actual pixel size of the active ranges
	// in the control. They should be kept in sync with the artwork for the controls.
	// The slider has an arrow at each end of the specified size. If the size is 0 or negative, there
	// are no arrows. The are between these two is the valid range. The thumb is positioned within this
	// range, with the given size.
	// When the thumb's logical coordinates are changed, the appropriate pixel values are updated. Likewise, if
	// the thumb's pixel coordinates are changed, the appropriate logical values are updated.

	DAMETHOD set_arrow_rect (int size) = 0;                // the pixel size of each end arrow, in pixels along the slider
	DAMETHOD get_arrow_rect (int *size) = 0;

	DAMETHOD set_thumb_rect (int start, int end) = 0;      // the pixel range of the thumb
	DAMETHOD get_thumb_rect (int *start, int *end) = 0;
};

// This is an interface for a container that holds items that are selectable. It is expected that a
// concrete implementation of this will also implement IControlParent, making its contained controls selectable
// by controlling their highlight states and keeping track of selection.
struct DACOM_NO_VTABLE ISelectionList : public IDAComponent
{
public:
	enum SelectionListType
	{
		SELECT_SINGLE,
		SELECT_MULTIPLE
	};

	DAMETHOD get_selection_count (int *count) = 0;

	// The single select methods select the given item when in SINGLE mode, and add to the selection
	// in MULTIPLE mode.
	// NOTE: Indices are of the contained controls.
	DAMETHOD select (IControl *control, bool unselect=false) = 0;  // the given control iff it is contained
	DAMETHOD select_indexed (int index, bool unselect=false) = 0; // the indexed control iff index is valid
	DAMETHOD select_multiple (int count, IControl *controls[], bool unselect=false) = 0;
	DAMETHOD select_multiple_indexed (int count, int indices[], bool unselect=false) = 0;
	
	// Selection getting for SINGLE and MULTIPLE is done with the same methods. SINGLE mode means selection
	// count is 0 or 1.
	// NOTE: Indices are of the selected controls.
	DAMETHOD get_selected_count (int *count) = 0;
	DAMETHOD get_selected (int selectIndex, IControl *which) = 0;
	DAMETHOD get_selected_index (int selectIndex, int *which) = 0;
};

//
// Example objects.
//

/*
	A class that implements IControl, IEventCallback, and an IConnectionContainer with a connection point for
IEventCallback, can be the basis for physical implementations of controls. At the very least it would provide the
storage and implementation for the control properties, and it could also handle the event callbacks by checking for
mouse events within the rectangle if enabled and visible, then calling a virtual function to process the message.
This BaseControl class will be used in the examples below.

	The Button component would process mouse events that occur over it, setting the pushed state according to
the mouse messages and the button type. It inherits its control properties from BaseControl. Again, there is no
drawing, so the Button component is useless if it needs to be visible.

	A Checkbox control aggregates a Button component whose type is BUTTON_UPDOWN and draws the UP state as
an empty box, and the down state as a box filled with an X. When it draws, it uses its aggregated IButton interface
to query the state of the button (up or down). It also uses that button's IControl interface to get the rest of
the state (highlight, enabled, visible, focused, and default) changing the drawing accordingly.

	The Frame component inherits from BaseControl and also implements IControlParent. It connects itself to each
child's IEventCallback connection point (if it has one) and reflects each received control message to its own
connection point. When it is moved, it adjusts its children's coordinates to maintain their relative positions. It
processes received key presses to change the focus among its children.

	The SimpleFrame control aggregates a Frame component and draws a background graphic before drawing all of the
child controls. For each child of the Frame, it queries for a drawing interface; if the interface exists, it
calls it.
*/

#endif
