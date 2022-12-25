#ifndef SCRIPTING_H
#define SCRIPTING_H

//--------------------------------------------------------------------------//
//                                                                          //
//                               Scripting.h                                //
//                                                                          //
//--------------------------------------------------------------------------//
/*
*/			    
//-------------------------------------------------------------------
/*
	The scripting engine
*/
//-------------------------------------------------------------------
//-------------------------------------------------------------------

#include <DACOM.h>
#include <list>

enum ScriptEvent
{
	SE_FIRST = 0,
	SE_MISSION_START,
	SE_PLAYER_START,
	SE_UPDATE,
	SE_SHIP_ENTER,
	SE_SHIP_EXIT,
	SE_OBJECT_DYING,
	SE_OBJECT_DIED,
	SE_MOVIE_FINISED,
	SE_FMV_MOVIE_FINISHED,
	SE_HEADSHOT_FINISHED,

	//must ALWAYS be last
	SE_LAST,
};

enum ScriptParameterType
{
	SPT_OBJECT,
	SPT_LONG,
	SPT_FLOAT,
	SPT_STRING,
	SPT_WIDESTRING,
	SPT_GRIDVECTOR,
};

struct IScriptParameter
{
	virtual ScriptParameterType GetType() = 0;
	virtual void*               GetValue() = 0;
	virtual const char*         GetName() = 0;
};

struct ScriptParameterList : std::list<IScriptParameter*>
{
	MEXTERN bool Push( IBaseObject* _object, const char* _paramname );
	MEXTERN bool Push( U32 _long, const char* _paramname );
	MEXTERN bool Push( float _float, const char* _paramname );
	MEXTERN bool Push( const char* _string, const char* _paramname );
	MEXTERN bool Push( const wchar_t* _wideString, const char* _paramname );
	MEXTERN bool Push( struct GRIDVECTOR* _gridVector, const char* _paramname );

	MEXTERN virtual ~ScriptParameterList();
};

struct DACOM_NO_VTABLE IScripting : public IDAComponent
{
	// startup/shutdown

	virtual bool Startup() = 0;

	virtual bool Shutdown() = 0;

	// Script Handling

	virtual U32 RunScript (const char * _module, const char * _function, ScriptParameterList* _parameters = NULL ) = 0;

	virtual bool ScriptExists(const char * _module, const char * _function) = 0;

	// Event 

	virtual void CallScriptEvent( ScriptEvent _eventID, ScriptParameterList* _parameters = NULL ) = 0;

	virtual U32 AddEventListener(const char * _module, const char * _function, ScriptEvent _eventID, char* _scriptContext = NULL) = 0;

	virtual void RemoveEventListener(U32 _listenerID, ScriptEvent _eventID) = 0;

	virtual void ScheduleEvent(SINGLE _deltaTime, const char * _module, const char * _function) = 0;

	// Debugging

	virtual const char* GetTraceback( void ) = 0;

	virtual const char* BuildCallStack( void ) = 0;

	virtual void AppendScriptPath( const char* _scriptDir ) = 0;

	virtual bool PushCall( const char* _name ) = 0;

	virtual bool PopCall( const char* _name ) = 0;

	virtual bool StartDebugger( const char* _scriptDirectory ) = 0;

	virtual bool StopDebugger() = 0;

	virtual bool StopDebuggerRunning() = 0;

	virtual char* GetErrorString() = 0;

	// Compiling

	virtual bool CompileStringBuffer( const char* _buffer, DWORD _bufferSize, const char* _module, bool _printError = true ) = 0;

	virtual bool SetConstant( int _constant, char* _constantName, const char* _module )  = 0;
};


#endif
