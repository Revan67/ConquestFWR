#ifndef PLUGIN_H
#define PLUGIN_H
//
// PlugIn.h - UniTool PlugIn interfaces
//

//
// Design Notes:
//        This header defines the interfaces used by UniTool plugins and internal components.
// There is an interface for publishing methods from a tool for use with a scripting language, and
// for publishing the events that a component can generate.
//

//
// Interface Definitions
//

// IScriptable allows UniTool to determine what functions can be called on the component via
// IDispatch, as well as the syntax supported by the function.
// Parameters have both a type, which describes their binary format, and a tag, which defines
// their context, allowing, for example, a 32 bit integer screen coordinate to be distinguished
// from a 32 bit integer handle. [This concept was stolen from LUA].
// Tags are generally only useful for PS_OBJECT.
// 
// There are two classes of parameters: values and references.  Values are entirely stored in the
// parameter itself. References are parameters that refer to real objects via pointers. If a
// parameter to a complex type, like PS_STRING or PS_OBJECT is passed by value, that means the
// pointer stored in the parameter is to be concidered a read-only value, and the callee should not
// keep the pointer around beyond the scope of the function. If the parameter is passed by reference,
// the pointer is to a pointer to the given object, and can therefore be written to.

typedef unsigned short ParamType;
typedef signed short ParamTag;

const ParamType PS_REF = (unsigned short)0x8000; // flag indicating a reference to the type specified
const ParamType	PS_VOID = 0;      // spec for the return type of a function that returns nothing.
const ParamType	PS_INT = 1;       // a 32 bit integer
const ParamType	PS_FLOAT = 2;     // a 32 bit floating point number
const ParamType	PS_STRING = 3;    // a null terminated string, ownership passed.
const ParamType	PS_ELLIPSIS = 4;  // place holder for variable number of subsequent parameters
const ParamType	PS_OBJECT = 5;    // a pointer to some object, opaque to UniTool.
const ParamType PS_BOOL = 6;      // bool value

struct ParamSpec
{
	ParamType type;
	ParamTag  tag;
};

// Structure used in the invoke() call.
struct Variant
{
	ParamSpec spec;
	union
	{
		// Non-reference values
		int    iVal;        // PS_INT
		float  fVal;        // PS_FLOAT
		const char * sVal;  // PS_STRING - The variant refers to the string, but does not own the memory
		void * oVal;        // PS_OBJECT
		bool   bVal;        // PS_BOOL

		// Same as above, but with PS_REF
		int *  iValRef;
		float *fValRef;
		char **sValRef;
		void **oValRef;
		bool * bValRef;
	};

	Variant () {spec.type = PS_INT; spec.tag = 0; iVal = 0; }
	Variant (Variant &val) { spec.type = val.spec.type; spec.tag = val.spec.tag; iVal = val.iVal; }
	Variant (int val) { *this = val; }
	Variant (unsigned int val) { *this = val; }
	Variant (float val) { *this = val; }
	Variant (bool val) { *this = val; }
	Variant (const char *val) { *this = val; }
	Variant (void *val) { *this = val; }

	Variant & operator = (int val) { spec.type = PS_INT; spec.tag = 0; iVal = val; return *this; }
	Variant & operator = (unsigned int val) { spec.type = PS_INT; spec.tag = 0; iVal = (int) val; return *this; }
	Variant & operator = (float val) { spec.type = PS_FLOAT; spec.tag = 0; fVal = val; return *this; }
	Variant & operator = (bool val) { spec.type = PS_BOOL; spec.tag = 0; bVal = val; return *this; }
	Variant & operator = (const char *val) { spec.type = PS_STRING; spec.tag = 0; sVal = val; return *this; }
	Variant & operator = (void *val) { spec.type = PS_OBJECT; spec.tag = 0; oVal = val; return *this; }
};

#define PURE_VIRTUAL /*const*/ = 0

// Cheezy IUnknown interface 
struct IDontKnow
{
	// Returns true and sets the given interface pointer if the named interface is supported,
	// returns false otherwise.
	virtual bool query_interface (const char *name, void **iface) PURE_VIRTUAL;
};

struct IScriptable
{
	// Returns the number of methods published
	virtual int method_count(void) PURE_VIRTUAL;

	// Returns the name of the zero-base-indexed method. 
	virtual const char *method_name(int index) PURE_VIRTUAL; 

	// Returns the length of the spec buffer for the zero-base-indexed method
	// This will always be at least 1 (the return value)
	virtual int method_speclen(int index) PURE_VIRTUAL;

	// Returns a pointer to the spec for zero-base-indexed method.
	// The first member of the array is the return value (which must be there), and the
	// rest are the parameters, in left-to-right order.
	virtual const ParamSpec *method_spec (int index) PURE_VIRTUAL;

	// For now, IScriptable also contains in invoke() method, which stands in for the DACOM
	// IDADispatch interface.
	// The invokeByIndex() method is a little more efficient, since it does not require a string
	// compare or a search.
	virtual bool invoke (const char *methodName, Variant *result, int paramCount, Variant *params) PURE_VIRTUAL;
	virtual bool invokeByIndex (int index, Variant *result, int paramCount, Variant *params) PURE_VIRTUAL;
};

// IEventSource allows UniTool to determine the names of the events exported by a component, and 
// provides the means for UniTool to be notified when an event occurs so it can call the
// appropriate script or other action.
// Events do not pass parameters. Any information needed by the event handler will have to
// be provided and retrieved via the object's IDispatch interface.

typedef unsigned int EventId;

struct IEventSink
{
	// Handles an event from an IEventSource instance.
	// The EventId is the index of the event. The id and the sender interface pointer uniquely
	// define the event being sent.
	// NOTE: The event id is the index into the sender's event list, therefore the name of the
	// sent event is available by calling sender->event_name(id);
	virtual void handle_event (EventId id, struct IEventSource *sender, int pCount, Variant *params) PURE_VIRTUAL;
};

// NOTE: This just exposes the events, it does not dispatch them to handlers. That is
// the responsibility of UniTool.
struct IEventSource
{
	// Returns the number of events this component can send.
	virtual EventId event_count(void) PURE_VIRTUAL;

	// Returns the name of the zero-base-indexed event.
	virtual const char *event_name(EventId index) PURE_VIRTUAL;

	// These methods allow an app to store and retrieve a DWORD of application defined
	// data to and from the event source.
	virtual void set_app_data (DWORD data) PURE_VIRTUAL;
	virtual DWORD get_app_data () PURE_VIRTUAL;

	// Tells this component who should receive events. NULL indicates that the
	// source should send events to no one.
	// Returns the old handler, NULL if none.
	virtual IEventSink *set_event_handler (IEventSink *sink) PURE_VIRTUAL;

	// *** There needs to be a way of getting more information from the event source when an event
	// *** occurs. The nature of this data is determined by the sender of the event and the event itself.
	// *** One method would be to add a method to this interface for retrieving event properties by either
	// *** index or name, and for enumerating event property names for a given event id. 
	// *** Or, we could reuse the ParamSpec from the IScriptable interface to describe the parameters of each event,
	// *** making automatic publishing of the event data easy, almost self-documenting.
	// *** The COM method is to define an interface for describing the kinds of event sink interfaces the event source
	// *** supports, forcing the appropriate event sink interface to be created outside the event source and given to
	// *** the event source as an IUnknown, which the source queries for the desired interrface. Once it has the
	// *** desired interface, it simply calls methods on that interface to accomplish its work.
	// *** Perhaps the best idea is to pass a variant array to the event sink, then let the event sink figure out what
	// *** to do with the parameters, since the sink has to understand the events anyway.
};

#endif

