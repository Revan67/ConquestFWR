// DALibs_BasicSceneDatabase.cpp: implementation of the CDALibs_BasicSceneDatabase class.
//
//////////////////////////////////////////////////////////////////////

#define STRICT
#include <windows.h>
#include <comdef.h>
#include <stdio.h>
#include <assert.h>

#include "dacom.h"
#include "davariant.h"
#include "TSmartPointer.h"

#include "DPF.h"
#include "DACOM_Utility.h"
#include "IObjectDatabase.h"


#define DACOM_COMPONENT_NAME CDALibs_BasicSceneDatabase

const char *CLSID_CDALibs_BasicSceneDatabase = "CDALibs_BasicSceneDatabase";

dacom_component CDALibs_BasicSceneDatabase :	dacom_implements IObjectDatabase,
												dacom_implements IComponentFactory
{

	// IObjectDatabase
	DACOM_INTERFACE_METHOD_DECL( ImportData,		( IDAComponent *data ));
	DACOM_INTERFACE_METHOD_DECL( InsertObject,		( IDAComponent *object ));
	DACOM_INTERFACE_METHOD_DECL( UpdateObject,		( IDAComponent *object ));
	DACOM_INTERFACE_METHOD_DECL( DeleteObject,		( IDAComponent *object ));
	DACOM_INTERFACE_METHOD_DECL( ExecuteListQuery,	( IDB_OBJECTTABLETYPE table ));
	DACOM_INTERFACE_METHOD_DECL( ExecuteNameQuery,	( IDB_OBJECTTABLETYPE table,  const char *name ));
	DACOM_INTERFACE_METHOD_DECL( ExecuteRangeQuery,	( IDB_OBJECTTABLETYPE table, IDB_RANGE &range ));
	DACOM_INTERFACE_METHOD_DECL( CloneResults,		( IDAComponent **out_results ));
	DACOM_INTERFACE_METHOD_DECL( GetCount,			( U32 *out_item_count ));
	DACOM_INTERFACE_METHOD_DECL( Reset,				( void ));
	DACOM_INTERFACE_METHOD_DECL( Item,				( U32 item, IDAComponent **out_item ));
	DACOM_INTERFACE_METHOD_DECL( Next,				( IDAComponent **out_item ));

	// IComponentFactory && IDAComponent
	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);
	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);
	DEFMETHOD_(U32,AddRef)    (void);
	DEFMETHOD_(U32,Release)   (void);

public:		// Interface
	CDALibs_BasicSceneDatabase( CLSID_DACOMDESC &creation_info );
	~CDALibs_BasicSceneDatabase();

protected:	// Private Data
	

	int	m_RefCnt;
};

//

DACOM_INTERFACE_METHOD_IMPL( ImportData,( IDAComponent *data ))
{
	return E_FAIL;
}

//

DACOM_INTERFACE_METHOD_IMPL( InsertObject,( IDAComponent *object ))
{
#if 0
	AddRenderables( data );
	AddCameras( data );
	AddLights( data );
#endif
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( UpdateObject,( IDAComponent *object ))
{
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( DeleteObject,( IDAComponent *object ))
{
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( ExecuteListQuery,( IDB_OBJECTTABLETYPE table ))
{

	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( ExecuteNameQuery,( IDB_OBJECTTABLETYPE table,  const char *name ))
{
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( ExecuteRangeQuery,( IDB_OBJECTTABLETYPE table, IDB_RANGE &range ))
{
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( CloneResults,( IDAComponent **out_results ))
{
	return E_FAIL;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetCount,( U32 *out_item_count ))
{
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Reset,( void ))
{
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Item,( U32 item, IDAComponent **out_item ))
{
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Next,( IDAComponent **out_item ))
{
	return S_OK;
}

//

GENRESULT CDALibs_BasicSceneDatabase::QueryInterface( const C8 *interface_name, void **instance)
{
	DACOM_QUERYINTERFACE_BEGIN(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_ENTRY(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_ENTRY(instance, IObjectDatabase)
	DACOM_QUERYINTERFACE_END(instance)
}


//

CDALibs_BasicSceneDatabase::CDALibs_BasicSceneDatabase( CLSID_DACOMDESC &creation_info )
{
	m_RefCnt = 0;
}

//

CDALibs_BasicSceneDatabase::~CDALibs_BasicSceneDatabase()
{
}

//

U32 CDALibs_BasicSceneDatabase::AddRef(void)
{
	m_RefCnt++;
	return 0;
}

//

U32 CDALibs_BasicSceneDatabase::Release(void)
{
	m_RefCnt--;
	if( m_RefCnt <= 0 ) {
		delete this;
	}
	return 0;
}

//

HRESULT RegisterCDALibs_BasicSceneDatabase( ICOManager *dacom )
{
	COMPTR<IDAComponent> IDAC;
	CLSID_DACOMDESC desc( CLSID_CDALibs_BasicSceneDatabase );
	if( T_DACOM_CreateInstance<CDALibs_BasicSceneDatabase,CLSID_DACOMDESC>( CLSID_CDALibs_BasicSceneDatabase, &desc, (IDAComponent **) &IDAC ) == S_OK ) {
		COMPTR<IComponentFactory> ICF;
		if( SUCCEEDED( IDAC->QueryInterface( IID_IComponentFactory, (void**) &ICF ) ) ) {
			dacom->RegisterComponent( ICF, IID_IDAComponent );	// NOTE: DACOM requires us to register an 'interface 
																// provider' not a component!  EEG
			return S_OK;
		}
	}
	return E_FAIL;
}

//

GENRESULT CDALibs_BasicSceneDatabase::CreateInstance( DACOMDESC *desc, void **instance )
{
	return T_DACOM_CreateInstance<CDALibs_BasicSceneDatabase,CLSID_DACOMDESC>( CLSID_CDALibs_BasicSceneDatabase, desc, (IDAComponent **) instance );
}

