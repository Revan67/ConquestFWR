//
// UITest.cpp - Test code for the DAUI stuff
//

#include "stdafx.h"

#include "daui.h"
#include <tconnpoint.h>
#include <tconncontainer.h>
#include <tsmartpointer.h>
#include <eventsys.h>
#include <rendpipeline.h>
#include <fdump.h>
#include <system.h>
#include <stddat.h>
#include "bmptexture.h"
#include "txttexture.h"
#include "dauievent.h"

#include "dauires.h"

//
// External globals
//

extern IRenderPipeline *PIPE;
extern ISystemContainer *SYSTEM;
extern ICOManager *DACOM;

//
// Interfaces
//

//===============================================
// Standard user interface components.
//===============================================

//
// BaseControl - This class implements default behavior for a control. The most simple way to
// create a control component is to inherit from this class.
// NOTE: This class is not intended for application use. It is intended for the control component writer. The end
// user is expected to aggregate a control object into the final control, adding drawing methods.
//

struct CONTROLDESC : public AGGDESC
{
	IDAComponent *eventSource;

	CONTROLDESC (const char *kind, IDAComponent *_eventSource = NULL) : AGGDESC ("IControl", kind)
	{
		size = sizeof(*this);
		eventSource = _eventSource;
	}
};

struct ControlMessage
{
	U32 message;
	void *param;

	ControlMessage (U32 _message, void *_param = NULL) { message = _message; param = _param; }
	ControlMessage () { message = 0; param = NULL; }
};

BOOL32 __stdcall notify_up_enum (struct IDAConnectionPoint * connPoint, struct IDAComponent *client, void *context)
{
	ASSERT(context);
	// Query the IEventCallback interface from the client.
	COMPTR<IEventCallback> handler;
	if (client->QueryInterface(IID_IEventCallback, handler) == GR_OK)
	{
		// The context should be a ControlMessage pointer
		ControlMessage *msg = (ControlMessage *) context;
		handler->Notify (msg->message, msg->param);
	}
	return TRUE;
}

class BaseControl : 
	public IControl,
	public IEventCallback,
	public ConnectionPointContainer<BaseControl>,
	public IAggregateComponent
{
protected:
	ConnectionPoint<BaseControl> outEventPoint;

	BEGIN_DACOM_MAP_OUTBOUND(BaseControl)
	DACOM_INTERFACE_ENTRY_AGGREGATE("IEventCallback", outEventPoint)
	END_DACOM_MAP()

	BEGIN_DACOM_MAP_INBOUND(BaseControl)
	DACOM_INTERFACE_ENTRY(IControl)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IControl,IControl)
	DACOM_INTERFACE_ENTRY2(IID_IEventCallback,IEventCallback)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer,IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

protected:
	RECT rect;
	bool isEnabled;
	bool isFocused;
	bool isVisible;
	bool isDefault;
	bool isHighlight;
	bool isSelected;
	bool isChanged;
	void *appData;
	unsigned long id;
	char *name;

	// Event related stuff
	U32 sourceHandle;
	COMPTR<IDAConnectionPoint> sourcePoint;

public:
	BaseControl ();
	virtual ~BaseControl ();

	// Message sending methods
	void notify_up (U32 message, void *param = 0);

	// Overloadables
	virtual bool valid_description (const char *desc);

	// Methods required for DAAggregateComponent and DAComponent templates
	GENRESULT init (CONTROLDESC * desc);

	// ====== IControl methods ======
	// Size and position API
	DAMETHOD set_rect (int x1, int y1, int x2, int y2);  // exclusive of right and bottom
	DAMETHOD get_rect (int *x1, int *y1, int *x2, int *y2);

	// General control properties
	DAMETHOD set_enabled (bool enabled);     // true == enable, false == disable
	DAMETHOD get_enabled (bool *enabled);

	DAMETHOD set_focused (bool focused);     // true == accepts keyboard, false == ignored keyboard
	DAMETHOD get_focused (bool *focused);

	DAMETHOD set_visible (bool visible);     // true == potentially visible, false == invisible
	DAMETHOD get_visible (bool *visible);

	DAMETHOD set_default (bool _isDefault);   // true == default control, false == otherwise 
	DAMETHOD get_default (bool *_isDefault);

	// Highlight and selected have different meaning. Highlight is when the mouse moves over an item and is
	// therefore temporary. Selection is persistant.
	DAMETHOD set_highlight (bool highlight); // true == brighter, false == normal
	DAMETHOD get_highlight (bool *highlight);

	DAMETHOD set_selected (bool selected);   // true == brighter, false == normal
	DAMETHOD get_selected (bool *selected);

	// This flag is used to indicate that the state of the control has changed. This allows for
	// optimized drawing.
	DAMETHOD clear_control_changed ();            // resets the changed flag to false
	DAMETHOD get_control_changed (bool *changed); // true if the control's state has changed since the last clear

	// This allows a control to carry around some application specific data.
	// It should be noted that the control will not free any data it holds here, so you should be
	// careful.
	DAMETHOD set_app_data (void *data);
	DAMETHOD get_app_data (void **data);

	// Unique identifiers for controls
	DAMETHOD set_id (unsigned long _id);
	DAMETHOD get_id (unsigned long *_id);
	DAMETHOD set_name (const char *_name);
	DAMETHOD get_name (char *_name, int bufferlen);

	// ====== IEvemtCallback methods ======
	DEFMETHOD(Notify) (U32 message, void *param = 0);

	// ====== IConnectioPointContainer methods ======
	// NOTE: These are all provided by the ConnectionPointContainer template.

	// ====== IAggregateComponent methods ======
	DEFMETHOD(Initialize) (void);
};

BaseControl::BaseControl () : outEventPoint(0)
{
	// Set up the default state.
	rect.left = rect.top = 0;
	rect.bottom = rect.right = 1;
	isEnabled = true;
	isFocused = false;
	isVisible = true;
	isDefault = false;
	isHighlight = false;
	isSelected = false;
	isChanged = true;
	name = NULL;
	id = 0;
	appData = NULL;
	sourceHandle = 0;
}

BaseControl::~BaseControl ()
{
	if (sourceHandle && sourcePoint != NULL)
	{
		sourcePoint->Unadvise (sourceHandle);
		sourceHandle = 0;
	}
}

GENRESULT COMAPI BaseControl::set_rect (int x1, int y1, int x2, int y2)
{
	rect.left = x1;
	rect.right = x2;
	rect.top = y1;
	rect.bottom = y2;
	isChanged = true;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::get_rect (int *x1, int *y1, int *x2, int *y2)
{
	if (x1)
	{
		*x1 = rect.left;
	}
	if (y1)
	{
		*y1 = rect.top;
	}
	if (x2)
	{
		*x2 = rect.right;
	}
	if (y2)
	{
		*y2 = rect.bottom;
	}
	return GR_OK;
}

GENRESULT COMAPI BaseControl::set_enabled (bool enabled)
{
	if (isEnabled != enabled)
	{
		isChanged = true;
	}
	isEnabled = enabled;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::get_enabled (bool *enabled)
{
	ASSERT (enabled != NULL);
	*enabled = isEnabled;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::set_focused (bool focused)
{
	if (isFocused != focused)
	{
		isChanged = true;
	}
	isFocused = focused;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::get_focused (bool *focused)
{
	ASSERT (focused != NULL);
	*focused = isFocused;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::set_visible (bool visible)
{
	if (isVisible != visible)
	{
		isChanged = true;
	}
	isVisible = visible;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::get_visible (bool *visible)
{
	ASSERT (visible != NULL);
	*visible = isVisible;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::set_default (bool _isDefault)
{
	if (isDefault != _isDefault)
	{
		isChanged = true;
	}
	isDefault = _isDefault;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::get_default (bool *_isDefault)
{
	ASSERT (_isDefault != NULL);
	*_isDefault = isDefault;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::set_highlight (bool highlight)
{
	if (isHighlight != highlight)
	{
		isChanged = true;
	}
	isHighlight = highlight;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::get_highlight (bool *highlight)
{
	ASSERT (highlight != NULL);
	*highlight = isHighlight;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::set_selected (bool selected)
{
	if (isSelected != selected)
	{
		isChanged = true;
	}
	isSelected = selected;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::get_selected (bool *selected)
{
	ASSERT (selected != NULL);
	*selected = isSelected;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::clear_control_changed ()
{
	isChanged = false;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::get_control_changed (bool *changed)
{
	ASSERT (changed);
	*changed = isChanged;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::set_app_data (void *data)
{
	appData = data;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::get_app_data (void **data)
{
	ASSERT (data);
	*data = appData;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::set_id (unsigned long _id)
{
	id = _id;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::get_id (unsigned long *_id)
{
	ASSERT(_id);
	*_id = id;
	return GR_OK;
}

GENRESULT COMAPI BaseControl::set_name (const char *_name)
{
	if (name)
	{
		delete name;
		name = NULL;
	}
	if (_name == NULL)
	{
		name = NULL;
	}
	else
	{
		int len = strlen(_name) + 1;
		name = new char[len];
		strncpy (name, _name, len);
	}
	return GR_OK;
}

GENRESULT COMAPI BaseControl::get_name (char *_name, int bufferlen)
{
	ASSERT(_name);
	ASSERT(bufferlen > 0);
	if (name == NULL)
	{
		_name[0] = '\0';
	}
	else
	{
		strncpy (_name, name, bufferlen);
		_name[bufferlen-1] = '\0';
	}
	return GR_OK;
}

// ====== IEventCallback methods ======
GENRESULT COMAPI BaseControl::Notify (U32 message, void *param)
{
	// By default, all received messages swallowed.
	// If you wanted to reflect a message, you would enumerate the connections on outEventPoint, sending
	// the message to each in turn.
	return GR_OK;
}

// ====== IAggregateComponent methods ======
GENRESULT COMAPI BaseControl::Initialize(void)
{ 
	if (sourcePoint != NULL)
	{
		if (sourcePoint->Advise (static_cast<IEventCallback *>(this), &sourceHandle) == GR_OK)
		{
			// All is well, so return success.
			return GR_OK;
		}
	}
	return GR_GENERIC;
}

// Methods required for DAAggregateComponent and DAComponent templates
GENRESULT BaseControl::init (CONTROLDESC * desc)
{
	// Make sure the descriptor matches
	if
	(
		desc->size != sizeof(CONTROLDESC) ||
		strcmp(desc->interface_name, "IControl") ||
		!valid_description (desc->description)
	)
	{
		return GR_NOT_IMPLEMENTED;
	}

	// If an event source is supplied, query it for, and attach to if present, an outgoing
	// IEventCallback connection point.

	if (desc->eventSource)
	{
		if (desc->eventSource->QueryOutgoingInterface ("IEventCallback", sourcePoint) == GR_OK)
		{
			// The actual connection of to the point is handled after all of the components are
			// loaded.
			// All is well. Return success.
		}
		else
		{
			return GR_INTERFACE_UNSUPPORTED;
		}
	}
	return GR_OK;
}

// Overloadables
bool BaseControl::valid_description (const char *desc)
{
	return false;
}

// Sends notifications up the connection heirarchy.
void BaseControl::notify_up (U32 message, void *param)
{
	ControlMessage msg (message, param);

	outEventPoint.EnumerateConnections (notify_up_enum, &msg);
}

//
// StaticControl - This control simply sets its highlight when the mouse moves over it and clears its highlight when
// the mouse moves outside of it.
//

class StaticControl : public BaseControl
{
protected:

public:
	StaticControl () { }

	virtual bool valid_description (const char *desc)
	{
		if (desc && !strcmp(desc, "static"))
		{
			return true;
		}
		return false;
	}

	// ====== IEventCallback methods ======
	virtual GENRESULT COMAPI Notify (U32 message, void *param)
	{
		// If the message is a mouse move message, check for a highlight state change. Otherwise, swallow the
		// message.
		if (message == SYS_MOUSE_MOVE)
		{
			// param points to a MousePos structure.
			// *** It should really be passing around a component for the message data.
			// *** Each class of system message would have an interface it understood, like IMouseEvent or
			// *** IKeyboardEvent. Receivers could then sanity check their message data via a query.
			// *** For now, we will make assumptions about the parameter.
			MousePos *pos = (MousePos *) param;
			if (pos->x < rect.right && pos->x >= rect.left && pos->y < rect.bottom && pos->y >= rect.top)
			{
				set_highlight (true);
			}
			else
			{
				set_highlight (false);
			}
		}
		return GR_OK;
	}
};

// ButtonControl - This implements standard button behavior

class ButtonControl : public BaseControl, public IButton
{
protected:
	bool       isPressed;
	ButtonType type;

	// Internal state flags.
	bool bKeyboardPressed;		// keyboard used to press button
	bool bMousePressed;         // mouse used to press button

	// ACK! The entire outgoing interface must be specified here because we added IButton.
	BEGIN_DACOM_MAP_INBOUND(ButtonControl)
	DACOM_INTERFACE_ENTRY(IControl)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY(IButton)
	DACOM_INTERFACE_ENTRY2(IID_IControl,IControl)
	DACOM_INTERFACE_ENTRY2(IID_IEventCallback,IEventCallback)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer,IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IButton,IButton)
	END_DACOM_MAP()

public:
	ButtonControl ()
	{
		isPressed = false;
		type = BUTTON_PUSH;
		bKeyboardPressed = false;
		bMousePressed = false;
	}

	virtual bool valid_description (const char *desc)
	{
		if (desc && !strcmp(desc, "button"))
		{
			return true;
		}
		return false;
	}

	// ====== IButton methods ======
	virtual GENRESULT COMAPI set_pressed (bool pushed)
	{
		if (isPressed != pushed)
		{
			isPressed = pushed;
			isChanged = true;
		}
		return GR_OK;
	}

	virtual GENRESULT COMAPI get_pressed (bool *pushed)
	{
		ASSERT(pushed);
		*pushed = isPressed;
		return GR_OK;
	}

	virtual GENRESULT COMAPI set_type (ButtonType _type)
	{
		if (type != _type)
		{
			type = _type;
			isChanged = true;
		}
		return GR_OK;
	}

	virtual GENRESULT COMAPI get_type (ButtonType *_type)
	{
		ASSERT(_type);
		*_type = type;
		return GR_OK;
	}

	void do_action ()
	{
		// Send a button message to all listeners
		ControlEvent evt;
		evt.id = id;
		evt.name = name;
		notify_up (SYS_BUTTON_PRESSED, &evt);
	}

	// ====== IEventCallback methods ======
	virtual GENRESULT COMAPI Notify (U32 message, void *param)
	{
		switch (message)
		{
		case SYS_MOUSE_MOVE:
			{
				// If the message is a mouse move message, check for a highlight state change.
				// param points to a MousePos structure.
				// *** It should really be passing around a component for the message data.
				// *** Each class of system message would have an interface it understood, like IMouseEvent or
				// *** IKeyboardEvent. Receivers could then sanity check their message data via a query.
				// *** For now, we will make assumptions about the parameter.
				MousePos *pos = (MousePos *) param;
				if (pos->x < rect.right && pos->x >= rect.left && pos->y < rect.bottom && pos->y >= rect.top)
				{
					set_highlight (true);
					if (bMousePressed)
					{
						isPressed = true;
					}
				}
				else
				{
					set_highlight (false);
					if (bMousePressed)
					{
						isPressed = false;
					}
				}
			}
			break;

		case SYS_MOUSE_LEFT_DOWN:
			{
				if (isEnabled)
				{
					MousePos *pos = (MousePos *) param;
					if (pos->x < rect.right && pos->x >= rect.left && pos->y < rect.bottom && pos->y >= rect.top)
					{
						bKeyboardPressed = false;
						bMousePressed = true;
						isPressed = true;
					}
				}
			}
			break;

		case SYS_MOUSE_LEFT_UP:
			{
				if (isEnabled)
				{
					if (bMousePressed)
					{
						bMousePressed = bKeyboardPressed = false;
						if (isPressed)
						{
							isPressed = false;
							do_action ();
						}
					}
					else
					{
						bMousePressed = false;
					}
				}
			}
			break;

		case SYS_KEY_DOWN:
			{
				if (isEnabled && isFocused && bKeyboardPressed==false)
				{
					KeyData *keydata = (KeyData *) param;
					switch (keydata->key)
					{
					case VK_RETURN:
					case VK_SPACE:
						bKeyboardPressed = true;
						bMousePressed = false;
						isPressed = true;
						break;
					} // end switch (keydata->key)
				}
			}
			break;

		case SYS_KEY_UP:
			{
				if (isEnabled && isFocused && bKeyboardPressed==true)
				{
					KeyData *keydata = (KeyData *) param;
					switch (keydata->key)
					{
					case VK_RETURN:
					case VK_SPACE:
						bKeyboardPressed = false;
						bMousePressed = false;
						isPressed = false;
						do_action ();
						break;
					} // end switch (keydata->key)
				}
			}
			break;

		default:
			break;
		}

		return GR_OK;
	}
};

// FrameControl - A container for controls

struct FrameChild
{
	COMPTR<IControl>           child;
	COMPTR<IDAConnectionPoint> connPoint;
	U32                        connHandle;

	FrameChild (IEventCallback *parent, IControl *_child)
	{
		child = _child;
		if (child->QueryOutgoingInterface ("IEventCallback", connPoint) == GR_OK)
		{
			// We have a child connection point. Advise it that we want to receive events.
			connPoint->Advise(parent, &connHandle);
		}
	}

	~FrameChild ()
	{
		if (connHandle && connPoint != NULL)
		{
			connPoint->Unadvise (connHandle);
			connHandle = 0;
		}
	}
};

class FrameControl : public BaseControl, public IControlParent
{
protected:
	// ACK! The entire outgoing interface must be specified here because we added IControlParent.
	BEGIN_DACOM_MAP_INBOUND(FrameControl)
	DACOM_INTERFACE_ENTRY(IControl)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY(IControlParent)
	DACOM_INTERFACE_ENTRY2(IID_IControl,IControl)
	DACOM_INTERFACE_ENTRY2(IID_IEventCallback,IEventCallback)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer,IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IControlParent,IControlParent)
	END_DACOM_MAP()

	// *** This is really a list of IControl pointers!
	MetaList<FrameChild> children;

public:
	FrameControl () {} 

	virtual bool valid_description (const char *desc)
	{
		if (desc && !strcmp(desc, "frame"))
		{
			return true;
		}
		return false;
	}

	// ====== IControl overloads from BaseControl ======
	virtual GENRESULT COMAPI set_rect (int x1, int y1, int x2, int y2)
	{
		int dx = (x1 - rect.left);
		int dy = (y1 - rect.top);
		BaseControl::set_rect(x1, y1, x2, y2);
		isChanged = true;

		// Adjust the position of each child control rectangle.
		MetaNode<FrameChild> *here = children.get_head();
		if (here)
		{
			do
			{
				int cx1, cy1, cx2, cy2;
				here->object->child->get_rect (&cx1, &cy1, &cx2, &cy2);
				here->object->child->set_rect (cx1+dx, cy1+dy, cx2+dx, cy2+dy);
			} while (children.traverse(here));
		}
		return GR_OK;
	}

	// ====== IControlParent methods ======
	virtual GENRESULT COMAPI add_child (IControl *which)
	{
		ASSERT(which);
		// Make sure that the same child it not added more than once.
		MetaNode<FrameChild> *here = children.get_head();
		if (here)
		{
			do
			{
				if (here->object->child == which)
				{
					// Already in the list. Abort
					return GR_OK;
				}
			} while (children.traverse(here));
		}

		// Not in the list, so add it and make the event connection.
		children.append (new FrameChild(static_cast<IEventCallback *>(this), which));
		which->AddRef();
		return GR_OK;
	}

	virtual GENRESULT COMAPI remove_child (IControl *which)
	{
		// Find the given child.
		ASSERT(which);
		MetaNode<FrameChild> *here = children.get_head();
		if (here)
		{
			do
			{
				if (here->object->child == which)
				{
					delete here->object;
					children.remove (here);
					break;
				}
			} while (children.traverse(here));
		}
		return GR_OK;
	}

	// Aggregators of a control parent must be able to access the list of children.
	// For example, a GraphicFrame object would need to draw all of its children. If the list were
	// not available, it would not be able to tell the children to draw.
	virtual GENRESULT COMAPI get_child_count (int *count)
	{
		ASSERT (count);
		*count = children.count();
		return GR_OK;
	}

	virtual GENRESULT COMAPI get_child (int index, IControl **which)
	{
		ASSERT(which);
		// Find the given child.
		MetaNode<FrameChild> *here = children.get_head();
		while (index--)
		{
			if (!children.traverse(here))
			{
				return GR_GENERIC;
			}
		}
		*which = here->object->child;
		(*which)->AddRef();
		return GR_OK;
	}

	// While these are not strictly needed, these operations are more efficient when done by the
	// actual container.

	virtual GENRESULT COMAPI get_child_index (IControl *which, int *index)
	{
		// Find the given child.
		ASSERT(which);
		ASSERT(index);
		*index = 0;
		MetaNode<FrameChild> *here = children.get_head();
		if (here)
		{
			do
			{
				if (here->object->child == which)
				{
					return GR_OK;
					break;
				}
				++(*index);
			} while (children.traverse(here));
		}
		return GR_GENERIC;
	}

	// ====== IEventCallback methods ======
	virtual GENRESULT COMAPI Notify (U32 message, void *param)
	{
		// *** TODO: Write code to move the keyboard focus among the children, only if this frame has the
		// *** focus itself. The keyboard focus is a hierarchy, going from the root control (if any) down each
		// *** control container to a leaf child. At any given level, only one control has the focus, and a
		// *** focused control's parent must have the focus.
		switch (message)
		{
		case SYS_BUTTON_PRESSED:
			// *** HACK!!! ***
			// Move this control a little bit.
			{
				set_rect (rect.left + 10, rect.top + 10, rect.right + 10, rect.bottom + 10);
			}
			break;

		default:
			// Send unhandled messages to our listeners.
			notify_up (message, param);
			break;
		}
		return GR_OK;
	}
};
	
//===============================================
// End user controls
//===============================================

// In order to create a common base class for aggregating view and model behavior, it has to
// aggregate both behaviors as contained.
// To create controls, you create a control instance with a view and a model component.
// So, all you write is the view component.

class UserControl : public IDAComponent
{
protected:
	U32 ref_count;
	COMPTR<IDAComponent> model;   // the "model" (and controller) of this control's behavior.
	COMPTR<IDAComponent> view;    // the "view" of this control.

public:
	UserControl ();
	virtual ~UserControl () { }

	// Aggregating method
	void set_model_view (IDAComponent *_model, IDAComponent *_view);

	// ====== IDAComponent methods ======
	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);
	DEFMETHOD_(U32,AddRef)           (void);
	DEFMETHOD_(U32,Release)          (void);
};

UserControl::UserControl ()
{
	ref_count=1;
}

void UserControl::set_model_view (IDAComponent *_model, IDAComponent *_view)
{
	ASSERT(model == NULL && view == NULL);
	model = _model;
	view = _view;

	COMPTR<IAggregateComponent> m;
	if (model->QueryInterface(IID_IAggregateComponent, m) == GR_OK)
	{
		m->Initialize ();
	}
	COMPTR<IAggregateComponent> v;
	if (view->QueryInterface(IID_IAggregateComponent, v) == GR_OK)
	{
		v->Initialize ();
	}
}

GENRESULT COMAPI UserControl::QueryInterface (const C8 *interface_name, void **instance)
{
	// We only support interfaces supported by either the model or the view.

	GENRESULT result;
	if (model)
	{
		if ((result = model->QueryInterface (interface_name, instance)) == GR_OK)
		{
			return result;
		}
	}
	if (view)
	{
		if ((result = view->QueryInterface (interface_name, instance)) == GR_OK)
		{
			return result;
		}
	}
	return GR_INTERFACE_UNSUPPORTED;
}

U32 COMAPI UserControl::AddRef (void)
{
	ref_count++;
	return ref_count;
}

U32 COMAPI UserControl::Release (void)
{
	if (ref_count > 0)
		ref_count--;

	if (ref_count == 0)
	{
		ref_count++;		// artificially add reference to prevent infinite loops
		delete this;
		return 0;
	}

	return ref_count;
}

// Test drawing interface. This interface stands in for one provided by the application.

class IAppDraw : public IDAComponent
{
public:
	virtual void COMAPI draw (float depth=0) = 0;
};

//
// A simple drawing test.
//

struct APPDRAWDESC : public AGGDESC
{
	const char *filename;

	APPDRAWDESC (const char *kind = "static", const char *_filename = NULL) : AGGDESC ("IAppDraw", kind)
	{
		size = sizeof(*this);
		filename = _filename;	
	}
};

class StaticView : public IAppDraw, public IAggregateComponent
{
protected:
	RPVertex1  v[4];
	BMPTexture tex;

	BEGIN_DACOM_MAP_INBOUND(StaticView)
	DACOM_INTERFACE_ENTRY(IAppDraw)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAppDraw,IAppDraw)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

public:
	StaticView () {}

	// Methods required for DAAggregateComponent and DAComponent templates
	GENRESULT init (APPDRAWDESC * desc);

	// ====== IAppDraw methods ======
	virtual void COMAPI draw (float depth);

	// ====== IAggregateComponent methods ======
	DEFMETHOD(Initialize) (void);
};

GENRESULT StaticView::init (APPDRAWDESC * desc)
{
	if
	(
		desc->size != sizeof(APPDRAWDESC) ||
		strcmp (desc->interface_name, "IAppDraw") ||
		(
			desc->description != NULL &&
			strcmp(desc->description, "static")
		) ||
		desc->filename == NULL
	)
	{
		return GR_NOT_IMPLEMENTED;
	}

	tex.create (desc->filename);
	return GR_OK;
}

void COMAPI StaticView::draw (float depth)
{
	// Get our control interface. If we don't have one, simply exit.
	COMPTR<IControl> control;
	COMPTR<IButton> button;
	if (static_cast<IAggregateComponent *>(this)->QueryInterface (IID_IControl, control) != GR_OK)
	{
		// This object does implement IControl. We cannot proceed.
		GENERAL_WARNING ("No IControl interface!");
		return;
	}

	static_cast<IAggregateComponent *>(this)->QueryInterface (IID_IButton, button);

	// Get the current state. Ignore the changed flag for now.
	int x1, y1, x2, y2;
	bool isEnabled;
	bool isFocused;
	bool isVisible;
	bool isDefault;
	bool isHighlight;
	bool isSelected;

	control->get_rect (&x1, &y1, &x2, &y2);
	control->get_enabled (&isEnabled);
	control->get_focused (&isFocused);
	control->get_visible (&isVisible);
	control->get_default (&isDefault);
	control->get_highlight (&isHighlight);
	control->get_selected (&isSelected);

	// If not visible, don't draw.
	if (!isVisible)
	{
		return;
	}

	// Fill out the vertices.

	unsigned long color = 0xFFA0A0A0; // not full intensity
	if (!isEnabled)
	{
		color = 0x80808080;  // less intense
	}
	if (isHighlight)
	{
		color = 0xFFFFFFFF;  // highest intensity
	}

	if (button != NULL)
	{
		bool isPressed;
		button->get_pressed (&isPressed);
		if (isPressed)
		{
			x1 += 2;
			y1 += 2;
			x2 += 2;
			y2 += 2;
		}
	}

	v[0].pos = Vector(x1, y1, depth);
	v[0].color = color;
	v[0].u = 0.0;
	v[0].v = 1.0;

	v[1].pos = Vector(x2, y1, depth);
	v[1].color = color;
	v[1].u = 1.0;
	v[1].v = 1.0;

	v[2].pos = Vector(x2, y2, depth);
	v[2].color = color;
	v[2].u = 1.0;
	v[2].v = 0.0;

	v[3].pos = Vector(x1, y2, depth);
	v[3].color = color;
	v[3].u = 0;
	v[3].v = 0.0;

	// Draw the rectangle
	PIPE->set_texture_stage_texture (0, tex.get_handle());
	PIPE->draw_primitive (D3DPT_TRIANGLEFAN, D3DVT_RPVERTEX, v, 4, 0);
}

GENRESULT COMAPI StaticView::Initialize (void)
{
	// Make sure that IControl is available.
	COMPTR<IControl> control;
	if (static_cast<IAggregateComponent *>(this)->QueryInterface(IID_IControl, control) == GR_OK)
	{
		return GR_OK;
	}

	GENERAL_WARNING ("No IControl interface!");
	return GR_GENERIC;
}

// FrameView - This is a static view that expects to be connected to a control parent.
// It will first draw itself, then it will traverse the control parent's children, telling each to draw itself.

class FrameView : public IAppDraw, public IAggregateComponent
{
protected:
	RPVertex1  v[4];
	BMPTexture tex;

	BEGIN_DACOM_MAP_INBOUND(FrameView)
	DACOM_INTERFACE_ENTRY(IAppDraw)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAppDraw,IAppDraw)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

public:
	FrameView () {}

	// Methods required for DAAggregateComponent and DAComponent templates
	GENRESULT init (APPDRAWDESC * desc);

	// ====== IAppDraw methods ======
	virtual void COMAPI draw (float depth);

	// ====== IAggregateComponent methods ======
	DEFMETHOD(Initialize) (void);
};

GENRESULT FrameView::init (APPDRAWDESC * desc)
{
	if
	(
		desc->size != sizeof(APPDRAWDESC) ||
		strcmp (desc->interface_name, "IAppDraw") ||
		(
			desc->description != NULL &&
			strcmp(desc->description, "frame")
		) ||
		desc->filename == NULL
	)
	{
		return GR_NOT_IMPLEMENTED;
	}

	tex.create (desc->filename);
	return GR_OK;
}

void COMAPI FrameView::draw (float depth)
{
	// Get our control interface. If we don't have one, simply exit.
	COMPTR<IControl> control;
	COMPTR<IControlParent> parent;
	if (static_cast<IAggregateComponent *>(this)->QueryInterface (IID_IControlParent, parent) != GR_OK)
	{
		// This object does implement IControl. We cannot proceed.
		GENERAL_WARNING ("No IControlParent interface!");
		return;
	}
	if (static_cast<IAggregateComponent *>(this)->QueryInterface (IID_IControl, control) != GR_OK)
	{
		// This object does implement IControl. We cannot proceed.
		GENERAL_WARNING ("No IControl interface!");
		return;
	}

	// Get the current state. Ignore the changed flag for now.
	int x1, y1, x2, y2;
	bool isEnabled;
	bool isFocused;
	bool isVisible;
	bool isDefault;
	bool isHighlight;
	bool isSelected;

	control->get_rect (&x1, &y1, &x2, &y2);
	control->get_enabled (&isEnabled);
	control->get_focused (&isFocused);
	control->get_visible (&isVisible);
	control->get_default (&isDefault);
	control->get_highlight (&isHighlight);
	control->get_selected (&isSelected);

	// If not visible, don't draw.
	if (!isVisible)
	{
		return;
	}

	// Fill out the vertices.

	unsigned long color = 0xFFA0A0A0; // not full intensity
	if (!isEnabled)
	{
		color = 0x80808080;  // less intense
	}
	if (isHighlight)
	{
		color = 0xFFFFFFFF;  // highest intensity
	}

	v[0].pos = Vector(x1, y1, depth);
	v[0].color = color;
	v[0].u = 0.0;
	v[0].v = 1.0;

	v[1].pos = Vector(x2, y1, depth);
	v[1].color = color;
	v[1].u = 1.0;
	v[1].v = 1.0;

	v[2].pos = Vector(x2, y2, depth);
	v[2].color = color;
	v[2].u = 1.0;
	v[2].v = 0.0;

	v[3].pos = Vector(x1, y2, depth);
	v[3].color = color;
	v[3].u = 0;
	v[3].v = 0.0;

	// Draw the rectangle
	PIPE->set_texture_stage_texture (0, tex.get_handle());
	PIPE->draw_primitive (D3DPT_TRIANGLEFAN, D3DVT_RPVERTEX, v, 4, 0);

	// Traverse the children, telling each to draw.
	static drawCount = 0;
	++drawCount;
	int count;
	if (parent->get_child_count (&count) == GR_OK)
	{
		for (int i = 0; i < count; ++i)
		{
			COMPTR<IControl> child;
			if (parent->get_child(i, child) == GR_OK)
			{
				COMPTR<IAppDraw> idraw;
				if (child->QueryInterface (IID_IAppDraw, idraw) == GR_OK)
				{
					idraw->draw (depth + 0.01);
				}
			}
		}
	}
}

GENRESULT COMAPI FrameView::Initialize (void)
{
	// Make sure that IControl is available.
	COMPTR<IControl> control;
	COMPTR<IControlParent> parent;
	if
	(
		static_cast<IAggregateComponent *>(this)->QueryInterface(IID_IControl, control) == GR_OK &&
		static_cast<IAggregateComponent *>(this)->QueryInterface(IID_IControlParent, parent) == GR_OK
	)
	{
		return GR_OK;
	}

	GENERAL_WARNING ("No IControl or no IControlParent interface!");
	return GR_GENERIC;
}

//===================================
// Testing functions
//===================================

//
// Local globals
//

static RPVertex1 verts[4];
static BMPTexture tex;
static TXTTexture txtTex;

const int MAX_CONTROLS = 16;
static IDAComponent *testControls[MAX_CONTROLS];

//
// Routines
//

//
// Finally, create an entire control based on this architecture.
//

IDAComponent *create_test_control (AGGDESC &modelDesc, AGGDESC &viewDesc, int x, int y, int w, int h)
{
	UserControl *control = new UserControl;
	IDAComponent *model;
	IDAComponent *view;

	viewDesc.outer = control;
	viewDesc.inner = &view;

	modelDesc.outer = control;
	modelDesc.inner = &model;

	// NOTE: The actual instance returned here is ignored, because all we care about is the
	// inner class.

	void *instance;
	if (DACOM->CreateInstance (&modelDesc, &instance) != GR_OK)
	{
		ASSERT(false && "Failed to create a model instance");
		return NULL;
	}
	if (DACOM->CreateInstance (&viewDesc, &instance) != GR_OK)
	{
		ASSERT(false && "Failed to create a view instance");
		return NULL;
	}

	control->set_model_view(model, view);

	{
		COMPTR<IControl> ctrl;
		if (control->QueryInterface (IID_IControl, ctrl) == GR_OK)
		{
			ctrl->set_rect (x, y, x+w, y+h);
		}
	}

	return control;
}

void add_child_to_frame (IDAComponent *frame, IDAComponent *control)
{
	COMPTR<IControlParent> parent;
	if (frame->QueryInterface (IID_IControlParent, parent) == GR_OK)
	{
		COMPTR<IControl> child;
		if (control->QueryInterface (IID_IControl, child) == GR_OK)
		{
			parent->add_child (child);
		}
	}
}

// startup routine

bool daui_startup ()
{
	// Fill out the vertices.
	verts[0].pos = Vector(0, 0, -0.1);
	verts[0].color = 0xFFFFFFFF;
	verts[0].u = 0.0;
	verts[0].v = 0.0;

	verts[1].pos = Vector(255, 0, -0.1);
	verts[1].color = 0xFFFFFFFF;
	verts[1].u = 1.0;
	verts[1].v = 0.0;

	verts[2].pos = Vector(255, 255, -0.1);
	verts[2].color = 0xFFFFFFFF;
	verts[2].u = 1.0;
	verts[2].v = 1.0;

	verts[3].pos = Vector(0, 255, -0.1);
	verts[3].color = 0xFFFFFFFF;
	verts[3].u = 0;
	verts[3].v = 1.0;

	tex.create ("terrain.bmp");

	// Create and register the component factories
	ASSERT(DACOM);
	
	IComponentFactory *server;
	
	server = new DAComponentFactory2<DAComponentAggregate<StaticControl>,CONTROLDESC>("IControl");
	DACOM->RegisterComponent(server, "IControl", DACOM_LOW_PRIORITY);
	server->Release();
	
	server = new DAComponentFactory2<DAComponentAggregate<ButtonControl>,CONTROLDESC>("IControl");
	DACOM->RegisterComponent(server, "IControl", DACOM_LOW_PRIORITY);
	server->Release();
	
	server = new DAComponentFactory2<DAComponentAggregate<FrameControl>,CONTROLDESC>("IControl");
	DACOM->RegisterComponent(server, "IControl", DACOM_LOW_PRIORITY);
	server->Release();
	
	server = new DAComponentFactory2<DAComponentAggregate<StaticView>,APPDRAWDESC>("IAppDraw");
	DACOM->RegisterComponent(server, "IAppDraw", DACOM_LOW_PRIORITY);
	server->Release();

	server = new DAComponentFactory2<DAComponentAggregate<FrameView>,APPDRAWDESC>("IAppDraw");
	DACOM->RegisterComponent(server, "IAppDraw", DACOM_LOW_PRIORITY);
	server->Release();

	// Create a Font factory for the default gui font.

	HFONT hFont = (HFONT) GetStockObject (DEFAULT_GUI_FONT);
	
	FONTFACTORYDESC desc("Arial", 20);
//	GetObject (hFont, sizeof(LOGFONT), &desc.logFont);
	desc.logFont.lfItalic = TRUE;

	COMPTR<IFontFactory> font;
	if (DACOM->CreateInstance (&desc, font) != GR_OK)
	{
		ASSERT(false && "Failed to create a font factory");
	}
	else
	{
//		txtTex.create(font, L"This is a test of the font factory");

		txtTex.create(font, GetModuleHandle(NULL), IDS_TITLE);
	}

	// Create the test components

	memset (testControls, 0, sizeof(testControls));

	
#if 1
	IDAComponent *button1 = create_test_control
						(
							CONTROLDESC("button", SYSTEM),
							APPDRAWDESC("static", "terrain.bmp"),
							200, 100, 200, 50
						);
#endif

#if 1
	IDAComponent *button2 = create_test_control
						(
							CONTROLDESC("button", SYSTEM),
							APPDRAWDESC("static", "button.bmp"),
							200, 160, 200, 50
						);
#endif

	IDAComponent *frame1 = create_test_control
						(
							CONTROLDESC("frame", SYSTEM),
							APPDRAWDESC("frame", "test.bmp"),
							150, 50, 300, 250
						);

	IDAComponent *frame2 = create_test_control
						(
							CONTROLDESC("frame", SYSTEM),
							APPDRAWDESC("frame", "terrain.bmp"),
							100, 45, 400, 260
						);
	if (frame1 != NULL)
	{
//		add_child_to_frame (frame1, button1);
		add_child_to_frame (frame1, button2);
		if (frame2 != NULL)
		{
			add_child_to_frame (frame2, frame1);
			add_child_to_frame (frame2, button1);
		}
	}

	testControls[0] = frame2;
	return true;
}

void daui_doframe ()
{
	// Do "once per frame" stuff here

	// Clear the buffer.
	PIPE->set_pipeline_state(RP_CLEAR_COLOR, 0x00FF0000);
	PIPE->set_pipeline_state(RP_CLEAR_DEPTH, 0xFFFFFFFF);
	PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL);

	// Draw the scene
	PIPE->begin_scene ();
	PIPE->set_ortho (0, 639, 479, 0);
//	PIPE->set_texture_stage_texture (0, tex.get_handle());
	PIPE->set_texture_stage_texture (0, txtTex.get_handle());
	PIPE->draw_primitive (D3DPT_TRIANGLEFAN, D3DVT_RPVERTEX, verts, 4, 0);
	for (int i = 0; i < MAX_CONTROLS; ++i)
	{
		if (testControls[i] != NULL)
		{
			COMPTR<IAppDraw> appdraw;
			if (testControls[i]->QueryInterface (IID_IAppDraw, appdraw) == GR_OK)
			{
				appdraw->draw (-0.01);
			}
		}
	}
	PIPE->end_scene ();

	// Swap the buffers.
	PIPE->swap_buffers ();
}

void daui_shutdown ()
{
	// *** TODO: Put shutdown stuff here.
}
