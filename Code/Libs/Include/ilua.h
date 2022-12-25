#ifndef ILUA_H
#define ILUA_H
//
// ILua.h - An interface for getting to a LUA interpreter state
//

//
// Design Notes:
//     This interface provides access to a LUA interpreter. Ideally, the interpreter object would
// also implement a generic scripting interface, so this interface is there for LUA specific tasks.
//

//
// Include files
//

#include <dacom.h>
#include <davariant.h>

//
// Interfaces
//

// For now, this interface is very basic.
struct ILua : public IDAComponent
{
	// Calls the named LUA function, pushing the parameters before calling and returning the first
	// result of the method.
	DEFMETHOD(CallFunction) (const C8 *funcName, DACOM_VARIANT *result, int paramCount, DACOM_VARIANT *params[]) = 0;
};

#endif
