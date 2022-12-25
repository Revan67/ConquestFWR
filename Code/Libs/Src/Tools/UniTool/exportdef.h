#ifndef EXPORTDEF_H
#define EXPORTDEF_H
//
// ExportDef.h - Tools for helping with exporting IScriptable methods
//

// *** TODO: Figure out a better way of handling this stuff. Perhaps an offline generator of this code
// *** from a spec file.

//
// Structure and class definitions

struct MethodDef
{
	const char *      name;
	int               specLen;
	const ParamSpec * spec;
};

//
// Macro definitions
//

#define METHOD_SPEC_BEGIN(widgetName, methodName, count, retSpec) static ParamSpec pSpec_ ## widgetName ## _ ## methodName [(count)+1] = {{retSpec, 0},
#define METHOD_SPEC_ENTRY(type) {type, 0}
#define METHOD_SPEC_END(widgetName, methodName, count) }; static MethodDef mDef_ ## widgetName ## _ ## methodName = {#methodName, (count)+1, pSpec_ ## widgetName ## _ ## methodName};

#define METHOD_TABLE_START(widgetName) static MethodDef *methods_ ## widgetName [] = {
#define METHOD_DEF(widgetName, methodName) &(mDef_ ## widgetName ## _ ## methodName)
#define METHOD_TABLE_END() };

#define METHOD_COUNT(widgetName) (sizeof(methods_ ## widgetName)/sizeof(MethodDef *))
#define METHOD_DATA(widgetName, index) (methods_ ## widgetName [index])


#define GET_METHOD_COUNT(widgetName, ancestor) \
	(ancestor::method_count() + METHOD_COUNT(widgetName))

#define GET_METHOD_NAME(index, widgetName, ancestor) \
( \
	(index < ancestor::method_count()) ? \
		ancestor::method_name(index) : \
		( \
			(index - ancestor::method_count() < METHOD_COUNT(widgetName)) ? \
				METHOD_DATA(widgetName, index - ancestor::method_count())->name : \
				NULL \
		) \
)

#define GET_METHOD_SPECLEN(index, widgetName, ancestor) \
( \
	(index < ancestor::method_count()) ? \
		ancestor::method_speclen(index) : \
		( \
			(index - ancestor::method_count() < METHOD_COUNT(widgetName)) ? \
				METHOD_DATA(widgetName, index - ancestor::method_count())->specLen : \
				0 \
		) \
)

#define GET_METHOD_SPEC(index, widgetName, ancestor) \
( \
	(index < ancestor::method_count()) ? \
		ancestor::method_spec(index) : \
		( \
			(index - ancestor::method_count() < METHOD_COUNT(widgetName)) ? \
				METHOD_DATA(widgetName, index - ancestor::method_count())->spec : \
				NULL \
		) \
)

#define METHOD_INVOKE(widgetName, ancestor, retval, methName, res, pCount, params) \
{ \
	retval = false;\
	int i; \
	int aCount = ancestor::method_count(); \
	for (i = 0; i < METHOD_COUNT(widgetName); ++i) \
	{ \
		if (!strcmp (methName, METHOD_DATA(widgetName, i)->name)) \
		{ \
			retval = invokeByIndex (i + aCount, res, pCount, params); \
			break; \
		} \
	} \
	if (i >= METHOD_COUNT(widgetName)) \
		retval = ancestor::invoke(methName, res, pCount, params); \
}


//
// Inline functions
//

#endif