//
// IPython.h
//

#ifndef PYTHON_INTERFACE_HEADER_H
#define PYTHON_INTERFACE_HEADER_H

#include "dacom.h"

struct IFileSystem;

// from the Python's Object.h file
struct _object;
typedef struct _object PyObject;

struct IPythonType
{
	virtual PyObject*   Create( PyObject* _args ) = 0;
	virtual const char* Type() = 0;
	virtual bool        IsType( PyObject* _other ) = 0;
};

struct IPythonEnum
{
	struct Module
	{
		const char* name;
		const char* filename;
		DWORD context;
	};

	struct Function
	{
		const char*  name;
		const char*  type;
		IPythonType* returnType;
		DWORD context;
	};

	struct Constant
	{
		const char* name;
		const char* type;
		int         value;
		DWORD context;
	};

	struct Error
	{
		const char* error;
	};

	struct Parameter
	{
		enum Type
		{
			LONG,
			FLOAT,
			STRING,
		};

		union Value
		{
			long        _long;
			float       _float;
			const char* _string;
		};

		// in data
		const char* module;
		const char* function;
		int         parameterIndex;

		// out data
		Value value;
		Type  type;
	};

	struct Result : public Parameter
	{
	};

	// for getting elements from IPython
	virtual void EnumModule( Module& _module ) {}
	virtual void EnumFunction( Function& _function ) {}
	virtual void EnumConstant( Constant& _constant ) {}
	virtual void EnumError( Error& _error ) {}
	virtual void EnumResult( Result& _result ) {}

	// for IPython to querry for elements
	virtual bool QueryParameter( Parameter& ){ return false; }
};

struct DACOM_NO_VTABLE IPython : IDAComponent
{
	// debugging

	virtual void AppendScriptPath( const char* _scriptDir ) = 0;

	virtual bool StartDebugListener( void ) = 0;

	// script modules

	virtual bool LoadModule( const char* _pyFilename ) = 0;

	virtual bool CreateModule( const char* _module ) = 0;

	virtual bool SaveModules( const char* _directory ) = 0;

	virtual bool EnumModules( IPythonEnum& _pythonEnum, DWORD _context ) = 0;

	virtual bool ExportModuleSymbols( const char* _module, IFileSystem& _file ) = 0;

	virtual bool CompileModule( const char* _buffer, DWORD _bufferSize, char* _moduleName, IPythonEnum* _errorEnum = 0 ) = 0;

	// constants

	virtual bool SetConstant( const char* _module, const char* _constant, int _contantValue ) = 0;

	virtual bool EnumConstants( IPythonEnum& _pythonEnum, DWORD _context ) = 0;

	// functions

	virtual bool FindFunctionInModule( const char* _module, const char* _function ) = 0;

	virtual bool ExecuteFunction( const char* _module, const char* _function, IPythonEnum* _pythonEnum = 0 ) = 0;

	virtual bool EnumFunctions( IPythonEnum& _pythonEnum, const char* _module, DWORD _context ) = 0;

	// objects

	virtual bool RegisterObjectType( IPythonType* _type ) = 0;

	virtual PyObject* CreateObject( const char* _type, PyObject* _args ) = 0;
};

#define IID_IPython MAKE_IID("IPython",1)

#endif

