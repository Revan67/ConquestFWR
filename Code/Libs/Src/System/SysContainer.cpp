//--------------------------------------------------------------------------//
//                                                                          //
//                           SysContainer.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Pbleisch $

   $Header: /Libs/dev/Src/System/SysContainer.cpp 7     3/21/00 4:30p Pbleisch $

           			  System component container class
*/
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "system.h"
#include "TComponent.h"
#include "TSmartPointer.h"
#include "da_heap_utility.h"
#include "IProfileParser.h"
#include "IConnection.h"
#include "FDump.h"
#include "Tempstr.h"

extern ICOManager *DACOM;                    // Handle to component manager
struct SystemContainer;

//--------------------------------------------------------------------------//
//
struct SysConInner : public DAComponentInner<SystemContainer>
{
	SysConInner (SystemContainer * _owner) : DAComponentInner<SystemContainer>(_owner)
	{
	}
	
	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);
};
//--------------------------------------------------------------------------//
//-------------------------System Container class---------------------------//
//--------------------------------------------------------------------------//

struct SystemContainer : public ISystemContainer, IDAConnectionPointContainer
{
	struct ELEMENT
	{
	  	struct ELEMENT *pNext;
		COMPTR<IDAComponent>  pInner;
		IDAComponent        * pOuter;
		ISystemComponent    * pSysComp;
		IAggregateComponent * pAggComp;

		ELEMENT (void)
		{
			pNext = 0;
		}
	};
	
	
	BEGIN_DACOM_MAP_INBOUND(SystemContainer)
	DACOM_INTERFACE_ENTRY(ISystemContainer)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY(ISystemComponent)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_ISystemContainer,ISystemContainer)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_ISystemComponent,ISystemComponent)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer,IDAConnectionPointContainer)
	END_DACOM_MAP()

	ELEMENT *pList, *pLast;
	SysConInner innerComponent;
	IDAComponent *outerComponent;
	BOOL32 bAggMember, bLoaded;
	
	//
	// methods
	// 

	SystemContainer( void ) : innerComponent( this )
	{ 
		outerComponent = &innerComponent;
	};

	~SystemContainer (void);

	GENRESULT init (AGGDESC *info);

	DA_HEAP_DEFINE_NEW_OPERATOR(SystemContainer);
	
	/* IDAComponent methods */

	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance)
	{
		return outerComponent->QueryInterface(interface_name, instance);
	}

	DEFMETHOD_(U32,AddRef) (void)
	{
		return outerComponent->AddRef();
	}

	DEFMETHOD_(U32,Release) (void)
	{
		return outerComponent->Release();
	}

	/* ISystemContainer methods */
   
	DEFMETHOD(LoadSystemComponents) (void);
	
	DEFMETHOD(Shutdown) (void);
	
	DEFMETHOD(AddComponent) (const AGGDESC * descriptor);

	/* IAggregateComponent methods */

	DEFMETHOD(Initialize) (void);

	/* ISystemComponent methods */

	DEFMETHOD_(void,Update) (void);

	/* IDAConnectionPointContainer methods */

	DEFMETHOD(FindConnectionPoint) (const C8 *connectionName, struct IDAConnectionPoint **connPoint);

	DEFMETHOD_(BOOL32,EnumerateConnectionPoints) (CONNCONTAINER_ENUM_PROC proc, void *context=0);
	
	/* SystemContainer methods */

	static BOOL32 __stdcall internalEnumCallback (struct IDAConnectionPointContainer * container, struct IDAConnectionPoint *connPoint, void *context);

	IDAComponent * getBase (void)
	{
		return static_cast<ISystemContainer *>(this);
	}
};

//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
GENRESULT SysConInner::QueryInterface (const C8 *interface_name, void **instance)
{
	GENRESULT result;

	if ((result = DAComponentInner<SystemContainer>::QueryInterface(interface_name, instance)) == GR_OK)
		return result;
	
	//
    // Search all available ISystemComponents in order of decreasing
    // priority until one is found which supports the requested application
    // interface
    //

	SystemContainer::ELEMENT *tmp = owner->pList;

	while (tmp)
	{
		if ((result = tmp->pInner->QueryInterface(interface_name, instance)) == GR_OK)
			return result;

		tmp = tmp->pNext;
	}

	return GR_INTERFACE_UNSUPPORTED;
}
//
//--------------------------------------------------------------------------//
//--------------------------SystemContainer methods-------------------------//
//--------------------------------------------------------------------------//
//
GENRESULT SystemContainer::init (AGGDESC *info)
{
	if (info->description != 0 && strcmp(info->description, "SystemContainer")==0)
		return GR_GENERIC;

	if (info->outer)
	{
		outerComponent = info->outer;
		*(info->inner) = &innerComponent;
		bAggMember = 1;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
SystemContainer::~SystemContainer (void)
{
	AddRef();		// prevent infinite loop
	Shutdown();
}
//--------------------------------------------------------------------------//
//
GENRESULT SystemContainer::Shutdown (void)
{
	ELEMENT *tmp;

	tmp = pList;
	while(tmp)
	{
		if(tmp->pSysComp)
			tmp->pSysComp->ShutdownAggregate();
		tmp = tmp->pNext;
	}

	while (pList)
	{
		tmp = pList->pNext;
		delete pList;
		pList = tmp;
	}

	pLast = 0;
	bLoaded = 0;

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT SystemContainer::LoadSystemComponents (void)
{
	if (bLoaded == 0)
	{
		COMPTR<IProfileParser> parser;
		HANDLE hSection;
		char buffer[256];
		int line=0;

		if (DACOM->QueryInterface(IID_IProfileParser, parser) != GR_OK)
			goto Done;

		if ((hSection = parser->CreateSection("System")) == 0)
			goto Done;

		while (parser->ReadProfileLine(hSection, line++, buffer, sizeof(buffer)) != 0)
		{
			char * ptr, * ptr2;
			AGGDESC info;

			ptr = buffer;
			while (*ptr == ' ' || *ptr == '\t')
				ptr++;
			if (*ptr == ';' || *ptr == 0)
				continue;
			if ((ptr2 = strchr(ptr, '=')) != 0)
			{
				*ptr2++ = 0;
				while (*ptr2 == ' ' || *ptr2 =='\t')
					ptr2++;
			}

			ELEMENT *element = new ELEMENT;
			
			info.interface_name = ptr;
			info.outer = getBase();
			info.inner = element->pInner;
			info.description = ptr2;

			//I should change the string then set the pointer...
			if ((ptr = const_cast<char*>(strchr(info.interface_name, ' '))) != 0)
				*ptr = 0;
			if ((ptr = const_cast<char*>(strchr(info.interface_name, '\t'))) != 0)
				*ptr = 0;

			char *response = "";

			if (DACOM->CreateInstance(&info, (void **) &element->pOuter) != GR_OK)
			{
				response = "[FAILED]";
				delete element;
			}
			else
			{
				response = "[OK]";

				if (pLast)
				{
					pLast->pNext = element;
					pLast = element;
				}
				else
					pLast = pList = element;

				if (element->pInner->QueryInterface(IID_ISystemComponent, (void **) &element->pSysComp) == GR_OK)
				{
					element->pSysComp->Release();		// get rid of extra reference
				}
				if (element->pInner->QueryInterface(IID_IAggregateComponent, (void **) &element->pAggComp) == GR_OK)
				{
					element->pAggComp->Release();		// get rid of extra reference
				}
			}

			GENERAL_NOTICE( TEMPSTR( "SystemContainer: LoadSystemComponents: Loading '%s' [%s] returned %s\n",
									  info.interface_name, 
									  info.description ? info.description : "",
									  response ) );
		}


		if (bAggMember == 0)	// else wait for container to do it
			Initialize();
		bLoaded = 1;
	}

Done:
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT SystemContainer::AddComponent (const AGGDESC * descriptor)
{
	ELEMENT *element = 0;
	AGGDESC * info = (AGGDESC *) descriptor;
	GENRESULT result = GR_OK;
	IDAComponent *  outer;
	IDAComponent ** inner;

	if (descriptor == 0)
	{
		result = GR_INVALID_PARMS;
		goto Done;
	}

	outer = info->outer;		// save these values
	inner = info->inner;		

	if ((element = new ELEMENT) == 0)
	{
		result = GR_OUT_OF_MEMORY;
		goto Done;
	}

	info->outer = getBase();
	info->inner = element->pInner;

	if ((result = DACOM->CreateInstance(info, (void **) &element->pOuter)) != GR_OK)
	{
		info->outer = outer;		// restore these values
		info->inner = inner;		
		goto Done;
	}

	//
	// hook it into the list
	//

	if (pLast)
	{
		pLast->pNext = element;
		pLast = element;
	}
	else
		pLast = pList = element;

	if (element->pInner->QueryInterface("ISystemComponent", (void **) &element->pSysComp) == GR_OK)
	{
		element->pSysComp->Release();		// get rid of extra reference
	}
	if (element->pInner->QueryInterface("IAggregateComponent", (void **) &element->pAggComp) == GR_OK)
	{
		element->pAggComp->Release();		// get rid of extra reference
	}

	info->outer = outer;		// restore these values
	info->inner = inner;		

Done:
	if (result != GR_OK)
		delete element;
	return result;
}
//--------------------------------------------------------------------------//
//
void SystemContainer::Update (void)
{
	ELEMENT *tmp;

	tmp = pList;
	while (tmp)
	{
		if (tmp->pSysComp)
			tmp->pSysComp->Update();
		tmp = tmp->pNext;
	}
}
//--------------------------------------------------------------------------//
//
GENRESULT SystemContainer::Initialize (void)
{
	ELEMENT *tmp, *back=0;
	GENRESULT result = GR_OK;

	tmp = pList;
	while (tmp)
	{
		if (tmp->pAggComp && (result = tmp->pAggComp->Initialize()) != GR_OK)
		{
			if (back)
				back->pNext = tmp->pNext;
			else
				pList = tmp->pNext;
			
			delete tmp;
			
			if (bAggMember)
				break;

			tmp = pList;
			back = 0;
			continue;
		}

		back = tmp;
		tmp = tmp->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
GENRESULT SystemContainer::FindConnectionPoint (const C8 *connectionName, struct IDAConnectionPoint **connPoint)
{
	GENRESULT result=GR_GENERIC;
	COMPTR<IDAConnectionPointContainer> container;
	ELEMENT *tmp = pList;

	while (tmp)
	{
		if (tmp->pInner->QueryInterface("IDAConnectionPointContainer", container) == GR_OK)
		{
			if ((result = container->FindConnectionPoint(connectionName, connPoint)) == GR_OK)
				break;
		}

		tmp = tmp->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 SystemContainer::EnumerateConnectionPoints (CONNCONTAINER_ENUM_PROC proc, void *context)
{
	BOOL32 result=1;
	COMPTR<IDAConnectionPointContainer> container;
	ELEMENT *tmp = pList;

	while (tmp)
	{
		if (tmp->pInner->QueryInterface("IDAConnectionPointContainer", container) == GR_OK)
		{
			if ((result = container->EnumerateConnectionPoints(proc, context)) == 0)
				break;
		}

		tmp = tmp->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
void RegisterContainerFactory (ICOManager *DACOM)
{
	IComponentFactory * server = new DAComponentFactory<SystemContainer, AGGDESC> ("ISystemContainer");

	if (server)
	{
		DACOM->RegisterComponent(server, "ISystemContainer", DACOM_LOW_PRIORITY);
		server->Release();
	}
}



//--------------------------------------------------------------------------//
//----------------------------End SysContainer.cpp--------------------------//
//--------------------------------------------------------------------------//
