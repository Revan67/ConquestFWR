// DALibs_Template.cpp: implementation of the CDALibs_Template class.
//
//////////////////////////////////////////////////////////////////////

#define STRICT
#include <windows.h>
#include <comdef.h>
#include <stdio.h>

#include "dacom.h"
#include "davariant.h"
#include "TSmartPointer.h"

#include "DPF.h"
#include "TCollection.h"
#include "MessageCrackers.h"
#include "DACOM_Utility.h"

#define DACOM_COMPONENT_NAME CDALibs_Template
const char *CLSID_CDALibs_Template = "CDALibs_Template";

dacom_component CDALibs_Template :	dacom_implements IComponentFactory
{
	// IComponentFactory && IDAComponent
	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);
	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);
	DEFMETHOD_(U32,AddRef)    (void);
	DEFMETHOD_(U32,Release)   (void);
	

public:		// Interface
	CDALibs_Template( CLSID_DACOMDESC &creation_data );
	~CDALibs_Template();

protected:	// Private Data

	int		m_RefCnt;
};

//



//

CDALibs_Template::CDALibs_Template( CLSID_DACOMDESC &creation_data )
{
	m_RefCnt = 0;
}

//

CDALibs_Template::~CDALibs_Template()
{
}

//

GENRESULT CDALibs_Template::QueryInterface( const C8 *interface_name, void **instance)
{
	DACOM_QUERYINTERFACE_BEGIN(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_ENTRY(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_END(instance)
}

//

HRESULT RegisterCDALibs_Template( ICOManager *dacom )
{
	COMPTR<IDAComponent> IDAC;
	CLSID_DACOMDESC desc( CLSID_CDALibs_Template );
	if( T_DACOM_CreateInstance<CDALibs_Template,CLSID_DACOMDESC>( CLSID_CDALibs_Template, &desc, (IDAComponent **) &IDAC ) == S_OK ) {
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

GENRESULT CDALibs_Template::CreateInstance( DACOMDESC *desc, void **instance )
{
	return T_DACOM_CreateInstance<CDALibs_Template,CLSID_DACOMDESC>( CLSID_CDALibs_Template, desc, (IDAComponent **) instance );
}

//

U32 CDALibs_Template::AddRef(void)
{
	m_RefCnt++;
	return 0;
}

//

U32 CDALibs_Template::Release(void)
{
	m_RefCnt--;
	if( m_RefCnt <= 0 ) {
		delete this;
	}
	return 0;
}

