// Python.cpp : Defines the entry point for the dll application.
//

#include "stdafx.h"

#include <IPython.h>
#include <tcomponent.h>
#include <da_heap_utility.h>
#include <filesys.h>
#include <SaveLoad.h>
#include <Python\Python.h>

#include <map>
#include <string>
#include <list>

struct Python : public IPython, public IAggregateComponent, public ISaverLoader
{
	BEGIN_DACOM_MAP_INBOUND(Python)
		DACOM_INTERFACE_ENTRY(Python)
		DACOM_INTERFACE_ENTRY2(IID_IPython,IPython)
		DACOM_INTERFACE_ENTRY(IAggregateComponent)
		DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
		DACOM_INTERFACE_ENTRY(ISaverLoader)
		DACOM_INTERFACE_ENTRY2(IID_ISaverLoader,ISaverLoader)
	END_DACOM_MAP()

	// IPython

	virtual void AppendScriptPath( const char* _scriptDir );
	virtual bool StartDebugListener( void );
	virtual bool LoadModule( const char* _pyFilename );
	virtual bool CreateModule( const char* _module );
	virtual bool SaveModules( const char* _directory );
	virtual bool EnumModules( IPythonEnum& _pythonEnum, DWORD _context );
	virtual bool ExportModuleSymbols( const char* _module, IFileSystem& _file );
	virtual bool CompileModule( const char* _buffer, DWORD _bufferSize, char* _moduleName, IPythonEnum* _errorEnum );
	virtual bool SetConstant( const char* _module, const char* _constant, int _contantValue );
	virtual bool EnumConstants( IPythonEnum& _pythonEnum, DWORD _context );
	virtual bool FindFunctionInModule( const char* _module, const char* _function );
	virtual bool ExecuteFunction( const char* _module, const char* _function, IPythonEnum* _errorEnum );
	virtual bool EnumFunctions( IPythonEnum& _pythonEnum, const char* _module, DWORD _context );
	virtual bool RegisterObjectType( IPythonType* _type );
	virtual PyObject* CreateObject( const char* _type, PyObject* _args );

	// IAggregateComponent 

	GENRESULT COMAPI Initialize(void);
	GENRESULT init( AGGDESC *desc );

	// ISaverLoader

	virtual bool Save( class TiXmlNode& );
	virtual bool Load( class TiXmlNode& );
	virtual bool Save( struct IFileSystem& );
	virtual bool Load( struct IFileSystem& );

	// data

	struct Pathname
	{
		char szPath[_MAX_PATH];
		char szDrive[_MAX_DRIVE];
		char szDir[_MAX_DIR];
		char szFilename[_MAX_FNAME];
		char szExt[_MAX_EXT];

		Pathname( const char* _fullpath )
		{
			// get file path information of this executable
			strcpy( szPath, _fullpath );
			strlwr( szPath );

			// split file path into drive, directory, file name, extension (incl. leading period)
			_splitpath( szPath, szDrive, szDir, szFilename, szExt );
		}
	};

	struct ModuleMap : std::map< std::string /*Module Name*/, PyObject* /*Module*/ >
	{
		PyObject* FindModule( const char* _name )
		{
			iterator modIt = find( _name );
			if( modIt != end() )
			{
				return modIt->second;
			}

			return NULL;
		}
	};

	struct PythonType : std::map< std::string /* Type Name */, IPythonType* /* PythonType */ >
	{
	};

	struct PyObjectList : std::list< PyObject* >
	{
	};

	ModuleMap  m_ModuleMap;
	PythonType m_TypeMap;

	// locals

	Python (void);
	~Python (void);

	bool loadModuleFromDisk( const char* _module );
	void reportError( IPythonEnum* _errorEnum, const char* _fmt, ... );
	void enumReturnValues( IPythonEnum* _returnEnum, PyObject* _returnValues );

	PyObject* getFunction( PyObject* _module, const char* _function );
	PyObject* compileCode( const char* _buffer, int _bufferSize, IPythonEnum* _errorEnum );
	PyObject* buildParameters( IPythonEnum* _paramEnum, const char* _module, const char* _function );
};

//----------------------------------------------------------------------------------------------

Python::Python(void)
{
}

//----------------------------------------------------------------------------------------------

Python::~Python(void)
{
}

//----------------------------------------------------------------------------------------------

GENRESULT COMAPI Python::Initialize(void) 
{ 
	Py_Initialize();
	return GR_OK; 
}

//----------------------------------------------------------------------------------------------

GENRESULT Python::init(AGGDESC*)
{
	return GR_OK;
}

//----------------------------------------------------------------------------------------------

void Python::AppendScriptPath( const char* _scriptDir )
{
	char buf[256];
	strncpy(buf, _scriptDir, 256);
	strupr(buf);

	char pyCommand[256];
	sprintf(pyCommand,"sys.path.append(\"%s\")", buf);
	
	PyRun_SimpleString("import sys");
	PyRun_SimpleString(pyCommand);
}

//----------------------------------------------------------------------------------------------

bool Python::StartDebugListener( void )
{
	return 0;
}

//----------------------------------------------------------------------------------------------

bool Python::LoadModule( const char* _pyFilename )
{
	return loadModuleFromDisk(_pyFilename);
}

//----------------------------------------------------------------------------------------------

bool Python::loadModuleFromDisk( const char* _filename )
{
	Pathname path(_filename);
	char* moduleName = path.szFilename;

	PyObject* module = m_ModuleMap.FindModule( moduleName );

	if( module )
	{
		module = PyImport_ReloadModule( module );
	}
	else
	{
		module = PyImport_ImportModule( moduleName );
		if( !module )
			PyErr_Clear();
	}

	if( !module )
	{
		// going to try to compile it manually
		FILE* f = fopen(_filename,"rt");
		if( f )
		{
			fseek(f, 0, SEEK_END);
			int fsize = ftell(f);
			fseek(f, 0, SEEK_SET);

			char* buffer = new char[ fsize ];
			fread( buffer, fsize, 1, f );

			PyObject* pyCode = compileCode( buffer, fsize, NULL );
			if( pyCode )
			{
				module = PyImport_ExecCodeModule(moduleName,pyCode);
			}
			Py_XDECREF( pyCode );
		}
	}

	if( module && PyModule_Check(module) )
	{
		// record it
		m_ModuleMap[moduleName] = module;
		return true;
	}

	// failed to import so clear all the Python error flags
	PyErr_Clear();
	return false;
}

//----------------------------------------------------------------------------------------------

bool Python::CreateModule( const char* _module )
{
	PyObject* module = m_ModuleMap.FindModule(_module);

	if( !module )
	{
		module = PyModule_New( (char*)_module );
	}

	if( module && PyModule_Check(module) )
	{
		// record it
		m_ModuleMap[_module] = module;
	}
	else
	{
		PyErr_Clear();
	}

	return( module != NULL );
}

//----------------------------------------------------------------------------------------------

bool Python::SaveModules( const char* _directory )
{
	return 0;
}

//----------------------------------------------------------------------------------------------

bool Python::EnumModules( IPythonEnum& _pythonEnum, DWORD _context )
{
	IPythonEnum::Module module;

	for( ModuleMap::iterator it = m_ModuleMap.begin(); it != m_ModuleMap.end(); it++ )
	{
		module.context  = _context;
		module.filename = PyModule_GetFilename( it->second );
		module.name     = PyModule_GetName( it->second );
		_pythonEnum.EnumModule( module );
	}
	return true;
}

//----------------------------------------------------------------------------------------------

bool Python::ExportModuleSymbols( const char* _module, IFileSystem& _file )
{
	// exports the symbols from this module's dictionary to a file

	PyObject* module = m_ModuleMap.FindModule( _module );
	if( !module )
	{
		return false;
	}

	struct ScriptMap : std::map< std::string /* module name */, std::string /* symbol */ > {};
	ScriptMap scriptMap;

	if( !PyModule_Check(module) )
	{
		return false;
	}

	PyObject* dict = PyModule_GetDict(module);
	if( !dict ) return false;

	const char* moduleName = PyModule_GetName(module);

	char* buffer = (char*)_alloca( 1024 * 2 );
	DWORD dwWritten;

	sprintf( buffer, "\n\nModule: %s\n", moduleName );
	_file.WriteFile( 0, buffer, strlen(buffer), &dwWritten, 0 );

	// first the functions...
	PyObject* key = NULL;
	PyObject* value = NULL;
	int pos = 0;
	std::string symbol;

	while (PyDict_Next(dict, &pos, &key, &value)) 
	{
		if( PyString_Check(key) )
		{
			const char* string = PyString_AS_STRING(key);

			// skip all internal dictionary variables
			if( strstr(string,"__") ) continue;

			if( PyFunction_Check(value) )
			{
				symbol = std::string(moduleName) + std::string(".") + std::string(string);
				scriptMap[ symbol ] = "";
			}
			else if( PyCFunction_Check(value) )
			{
				symbol = std::string(moduleName) + std::string(".") + std::string(string);

				PyObject* pydoc = PyObject_GetAttrString(value,"__doc__");
				if( pydoc )
				{
					scriptMap[ symbol ] = PyString_AS_STRING(pydoc);
					Py_DECREF( pydoc );
				}
				else
					scriptMap[ symbol ] = "";
			}
			else if( PyCallable_Check(value) )
			{
				symbol = std::string(moduleName) + std::string(".") + std::string(string);
				scriptMap[ symbol ] = "";
			}
		}
	}

	ScriptMap::iterator sit;
	for( sit = scriptMap.begin(); sit != scriptMap.end(); sit++ )
	{
		sprintf( buffer, "%s %s\n", sit->first.c_str(), sit->second.c_str() );
		_file.WriteFile( 0, buffer, strlen(buffer), &dwWritten, 0 );
	}
	scriptMap.clear();

	//... then the constants
	key = NULL;
	value = NULL;
	pos = 0;

	while (PyDict_Next(dict, &pos, &key, &value)) 
	{
		if( PyString_Check(key) )
		{
			const char* string = PyString_AS_STRING(key);

			// skip all internal dictionary variables
			if( strstr(string,"__") ) continue;

			if( PyInt_Check(value) )
			{
				symbol = std::string(moduleName) + std::string(".") + std::string(string);
				scriptMap[ symbol ] = "";
			}
		}
	}

	// export the script's constants
	for( sit = scriptMap.begin(); sit != scriptMap.end(); sit++ )
	{
		sprintf( buffer, "%s\n", sit->first.c_str());
		_file.WriteFile( 0, buffer, strlen(buffer), &dwWritten, 0 );
	}
	scriptMap.clear();

	return true;
}

//----------------------------------------------------------------------------------------------

bool Python::CompileModule( const char* _buffer, DWORD _bufferSize, char* _moduleName, IPythonEnum* _errorEnum )
{
	bool ret = false;

	PyObject* pyCode = compileCode( _buffer, _bufferSize, _errorEnum );
	if( pyCode )
	{
		PyObject* module = PyImport_ExecCodeModule(_moduleName,pyCode);

		if( module && PyModule_Check(module) )
		{
			// record new module
			m_ModuleMap[_moduleName] = module;
			ret = true;
		}
	}
	Py_XDECREF( pyCode );

	return ret;
}

//----------------------------------------------------------------------------------------------

bool Python::SetConstant( const char* _module, const char* _constant, int _contantValue )
{
	PyObject* module = m_ModuleMap.FindModule( _module );
	if( module )
	{
		int ret = PyModule_AddIntConstant( module, (char*)_constant, _contantValue);
		return( ret != -1 );
	}
	return false;
}

//----------------------------------------------------------------------------------------------

bool Python::EnumConstants( IPythonEnum& _pythonEnum, DWORD _context )
{
	IPythonEnum::Constant constant;

	for( ModuleMap::iterator it = m_ModuleMap.begin(); it != m_ModuleMap.end(); it++ )
	{
		PyObject* dict = PyModule_GetDict(it->second);

		PyObject *key, *value;
		int pos = 0;

		while( PyDict_Next(dict, &pos, &key, &value) ) 
		{
			if( PyInt_Check(value) )
			{
				constant.context = _context;
				constant.value   = PyInt_AsLong(value);
				constant.name    = PyString_AsString(key);
				constant.type    = NULL;

				_pythonEnum.EnumConstant( constant );
			}
		}
	}

	return true;
}

//----------------------------------------------------------------------------------------------

bool Python::FindFunctionInModule( const char* _module, const char* _function )
{
	return( getFunction(m_ModuleMap.FindModule(_module), _function) != NULL );
}

//----------------------------------------------------------------------------------------------

bool Python::ExecuteFunction( const char* _module, const char* _function, IPythonEnum* _pythonEnum )
{
	PyObject* function = getFunction(m_ModuleMap.FindModule(_module), _function);

	if( !function )
	{
		reportError( _pythonEnum, "Could not find function %s.%s()", _module, _function );
		return false;
	}

	// build the parameters to use for this function
	PyObject* parameters = buildParameters( _pythonEnum, _module, _function );

	// try to exectute the function
	PyObject * pValue = PyObject_CallObject(function,parameters);
	if(!pValue)
	{
		PyErr_Clear();
		reportError(_pythonEnum, "Function call failed %s.%s()", _module, _function );
		return false;
	}

	// return value(s) to enumerator
	enumReturnValues( _pythonEnum, pValue );
	Py_DECREF(pValue);

	return true;
}

//----------------------------------------------------------------------------------------------

bool Python::EnumFunctions( IPythonEnum& _pythonEnum, const char* _module, DWORD _context )
{
	PyObject* module = m_ModuleMap.FindModule( _module );

	if( !module )
	{
		return false;
	}
	if( !PyModule_Check(module) )
	{
		return false;
	}

	IPythonEnum::Function func;
	PyObject* dict = PyModule_GetDict(module);
	PyObject* key = NULL;
	PyObject* value = NULL;
	int pos = 0;

	while (PyDict_Next(dict, &pos, &key, &value)) 
	{
		if( PyString_Check(key) )
		{
			const char* string = PyString_AS_STRING(key);

			// skip all internal dictionary variables
			if( strstr(string,"__") ) continue;

			if( PyFunction_Check(value) || PyCFunction_Check(value) || PyCallable_Check(value) )
			{
				func.context	= _context;
				func.name		= PyString_AS_STRING(value);
				func.returnType = NULL;
				func.type		= NULL;
				_pythonEnum.EnumFunction( func );
			}
		}
	}

	return true;
}

//----------------------------------------------------------------------------------------------

bool Python::RegisterObjectType( IPythonType* _type )
{
	if( _type && _type->Type() )
	{
		m_TypeMap[ _type->Type() ] = _type;
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------------

PyObject* Python::CreateObject( const char* _type, PyObject* _args )
{
	PythonType::iterator it = m_TypeMap.find( _type );
	if( it != m_TypeMap.end() )
	{
		IPythonType* type = it->second;
		PyObject* instance = type->Create( _args );
		if( instance )
		{
			return instance;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------------------------

bool Python::Save( class TiXmlNode& )
{
	return 0;
}

//----------------------------------------------------------------------------------------------

bool Python::Load( class TiXmlNode& )
{
	return 0;
}

//----------------------------------------------------------------------------------------------

bool Python::Save( struct IFileSystem& )
{
	return 0;
}

//----------------------------------------------------------------------------------------------

bool Python::Load( struct IFileSystem& )
{
	return 0;
}

//----------------------------------------------------------------------------------------------

PyObject* Python::getFunction( PyObject* _module, const char* _function )
{
	if( !_module )
	{
		return false;
	}
	if( !PyModule_Check(_module) )
	{
		return false;
	}
	if( !_function )
	{
		return false;
	}

	PyObject* dict = PyModule_GetDict(_module);
	const char* moduleName = PyModule_GetName(_module);

	// first the functions...
	PyObject* key = NULL;
	PyObject* value = NULL;
	int pos = 0;

	while (PyDict_Next(dict, &pos, &key, &value)) 
	{
		if( PyString_Check(key) )
		{
			const char* string = PyString_AS_STRING(key);

			// skip all internal dictionary variables
			if( strstr(string,"__") ) continue;

			if( PyFunction_Check(value) || PyCFunction_Check(value) || PyCallable_Check(value) )
			{
				if( !strcmp(string,_function) )
				{
					return value;
				}
			}
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------------------------

void Python::reportError( IPythonEnum* _errorEnum, const char* _fmt, ... )
{
	char buff[1024*2];
	va_list args;
	va_start(args, _fmt);
	_vsnprintf( buff, sizeof(buff), _fmt, args);

	if( _errorEnum )
	{
		IPythonEnum::Error err;
		err.error = buff;
		_errorEnum->EnumError( err );
	}

	FILE* hErrFile = fopen("pyerr.log", "w+t");
	if( hErrFile )
	{
		PyObject* pPyStdErr = PyFile_FromFile(hErrFile, "<stderr>", "w", NULL);
		PySys_SetObject("stderr", pPyStdErr);

		PyErr_Print();	
		
		fflush(hErrFile);
		fseek(hErrFile, 0, SEEK_END);
		int fsize = ftell(hErrFile);
		rewind(hErrFile);
			
		char* pyError = new char[ fsize + 1];
		memset( pyError, 0, fsize+1 );
		fread(pyError, 1, fsize, hErrFile);

		if( _errorEnum )
		{
			IPythonEnum::Error err;
			err.error = pyError;
			_errorEnum->EnumError( err );
		}

		fclose(hErrFile);
		delete [] pyError;
	}

	PyErr_Clear();
}

//----------------------------------------------------------------------------------------------

PyObject* Python::compileCode( const char* _buffer, int _bufferSize, IPythonEnum* _errorEnum )
{
	char* newCodeBuf = new char[_bufferSize + 1];
	memset( newCodeBuf, 0, _bufferSize + 1 );

	// convert all those annoying "returns"
	int k = 0;
	for( int i = 0; i < (int)_bufferSize; i++ )
	{
		if( _buffer[i] != '\r')
		{
			newCodeBuf[k++] = _buffer[i];
		}
	}

	// insert a 2 "new lines" if the last chracter is a TAB
	if( newCodeBuf[k-1] == '\t' )
	{
		assert( k < i );
		newCodeBuf[k-1] = 10;
		newCodeBuf[k+0] = 10;
	}

	// try to compile the buffer
	PyObject* pyCode = Py_CompileString(newCodeBuf,"<stdin>",Py_file_input);

#ifdef _DEBUG
	// if not, get the traceback and put it into a FILE
	if( !pyCode )
	{
		reportError( _errorEnum, "COMPILE ERROR:" );
	}
#endif

	// clean up
	delete [] newCodeBuf;

	return pyCode;
}

//----------------------------------------------------------------------------------------------

void Python::enumReturnValues( IPythonEnum* _returnEnum, PyObject* _returnValues )
{
	if( !_returnEnum || !_returnValues ) return;

	if( PyTuple_Check(_returnValues) )
	{
		PyObject* pyTuple = _returnValues;
		int numItems = PyTuple_Size(pyTuple);
		for( int i = 0; i < numItems; i++ )
		{
			enumReturnValues( _returnEnum, PyTuple_GetItem(pyTuple,i) );
		}
	}
	else if( PyList_Check(_returnValues) )
	{
		PyObject* pyTuple = PyList_AsTuple(_returnValues);
		enumReturnValues( _returnEnum, pyTuple );
		Py_XDECREF( pyTuple );
	}
	else if( PyInt_Check(_returnValues) )
	{
		IPythonEnum::Result r;
		r.type = IPythonEnum::Result::LONG;
		r.value._long = PyLong_AsLong(_returnValues);
		_returnEnum->EnumResult( r );
	}
	else if( PyFloat_Check(_returnValues) )
	{
		IPythonEnum::Result r;
		r.type = IPythonEnum::Result::FLOAT;
		r.value._float = static_cast<float>( PyFloat_AsDouble(_returnValues) );
		_returnEnum->EnumResult( r );
	}
	else if( PyString_Check(_returnValues) )
	{
		IPythonEnum::Result r;
		r.type = IPythonEnum::Result::STRING;
		r.value._string = PyString_AsString(_returnValues);
		_returnEnum->EnumResult( r );
	}
}

//----------------------------------------------------------------------------------------------

PyObject* Python::buildParameters( IPythonEnum* _pythonEnum, const char* _module, const char* _function )
{
	PyObject* parameters = NULL;
	if( _pythonEnum )
	{
		int paramIndex = 0;
		IPythonEnum::Parameter parameter;
		parameter.function		 = _function;
		parameter.module		 = _module;
		parameter.parameterIndex = paramIndex;

		PyObjectList list;

		// build parameter list of PyObjects
		while( _pythonEnum->QueryParameter(parameter) )
		{
			// go to next parameter
			paramIndex++;
			parameter.parameterIndex = paramIndex;

			PyObject* obj = NULL;

			switch( parameter.type )
			{
				case IPythonEnum::Parameter::LONG:   obj = PyInt_FromLong(parameter.value._long);        break;
				case IPythonEnum::Parameter::FLOAT:	 obj = PyFloat_FromDouble(parameter.value._float);   break;
				case IPythonEnum::Parameter::STRING: obj = PyString_FromString(parameter.value._string); break;
			}

			if( obj )
				list.push_back(obj);
		}

		// any parameters?
		if( list.size() )
		{
			// start a new Tuple for parameters and place them into the tuple
			paramIndex = 0;
			parameters = PyTuple_New( list.size() );

			for( PyObjectList::iterator it = list.begin(); it != list.end(); it++ )
			{
				// NOTE: PyTuple_SetItem() "steals" a ref from the PyObject, so there is no need to DECREF these objects!
				PyTuple_SetItem( parameters, paramIndex++, *it);
			}

			// NOTE: All these objects should now be IN the tuple, and DECREFed
			list.clear();
		}
	}
	return parameters;
}

//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------

#define CLSID_Python "IPython"

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  fdwReason, LPVOID lpReserved )
{
	switch (fdwReason)
	{
		//
		// DLL_PROCESS_ATTACH: Create object server component and register it with DACOM manager
		//
		case DLL_PROCESS_ATTACH:
		{
			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE( hinstDLL );

			ICOManager * DACOM = DACOM_Acquire();
			IComponentFactory * server;

			// Register System aggragate factory
			if( DACOM && (server = new DAComponentFactory2<DAComponentAggregate<Python>, AGGDESC>(CLSID_Python)) != NULL ) 
			{
				DACOM->RegisterComponent( server, CLSID_Python, DACOM_NORMAL_PRIORITY );
				server->Release();
			}
			
			break;
		}

		case DLL_PROCESS_DETACH:
			Py_Finalize();
			break;
	}

	return TRUE;

}
