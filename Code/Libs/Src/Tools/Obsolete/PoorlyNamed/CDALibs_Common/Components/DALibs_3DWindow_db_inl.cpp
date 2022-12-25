// DALibs_3DWindow_db_inl.cpp
//
//  This is code that should really go away when the DB is externalized.
//


	typedef HRESULT (*FEIOO_CB)( INSTANCE_INDEX idx, LPVOID context );

	//

	struct ITEMINFO
	{
		ITEMINFO *next;
		ITEMINFO *prev;
		INSTANCE_INDEX item;
	};

	//

	struct ITEMCBDATA
	{
		CDALibs_3DWindow	*_3DWindow;
		COMPTR<IModel>		 _IModel;
		LList<ITEMINFO>		 Items;
	};

	//
	static HRESULT FindEngineCameras( INSTANCE_INDEX idx, LPVOID context )
	{
		ITEMCBDATA *d = (ITEMCBDATA*)context;

		COMPTR<ICamera> IC;
		if( SUCCEEDED( d->_3DWindow->m_IEngine->query_interface( idx, "ICamera", (void**) &IC ) ) ) {
			ITEMINFO *ii = d->Items.alloc();
			d->Items.link( ii );
			ii->item = idx;
		}
		return S_OK;
	}

	//

	static HRESULT ReplaceEngineCamera( INSTANCE_INDEX idx, LPVOID context )
	{
		ITEMCBDATA *d = (ITEMCBDATA*)context;
		static U32 counter = 0;

		COMPTR<ICamera> IC;
		if( FAILED( d->_3DWindow->m_IEngine->query_interface( idx, "ICamera", (void**) &IC ) ) ) {
			return E_FAIL;
		}

		// 
		// Create camera
		//
		char sz[255+1];
		const char *n = d->_IModel->get_name( idx );
		if( n != NULL ) {
			sprintf( sz, "%s", n );
		}
		else {
			sprintf( sz, "(Unknown_%d)", counter );
			counter++;
		}

		CLSID_DACOMDESC desc( "CDALibs_Camera", d->_3DWindow->m_ISystem, d->_3DWindow->m_IEngine );
		COMPTR<IDAComponent> cam;
		if( FAILED( d->_3DWindow->m_ICOManager->CreateInstance( &desc, (void**) &cam ) ) ) {
			return E_FAIL;
		}

		// Need an instance index from the camera for the joints.
		//
		COMPTR<IDACOMEngineInstance> IEI;
		if( FAILED( cam->QueryInterface( IID_IDACOMEngineInstance, IEI ) ) ) {
			return E_FAIL;
		}

		INSTANCE_INDEX new_idx;
		IEI->GetInstanceIndex( &new_idx );
		if( new_idx == INVALID_INSTANCE_INDEX ) {
			COMPTR<IPersistable> IP2;
			if( SUCCEEDED( cam->QueryInterface( IID_IPersistable, (void**) &IP2) ) ) {
				if( FAILED( IP2->LoadFromFileSystem( "DefaultCamera", NULL ) ) ) {
					return E_FAIL;
				}
			}
			IEI->GetInstanceIndex( &new_idx );
		}

		// 
		// Set new camera to engine camera parameters
		// 
		COMPTR<IGeoTransformable> IGT;
		if( SUCCEEDED( cam->QueryInterface( IID_IGeoTransformable, IGT ) ) ) {

			COMPTR<ILowLevelCamera> ILLC;
			if( SUCCEEDED( cam->QueryInterface( IID_ILowLevelCamera, ILLC ) ) ) {
				
				Vector v;
				Transform T;
				const _pane *vp;
				v = IC->get_position();
				IGT->SetTranslation( &v );
				T = IC->get_transform();
				IGT->SetTransform( &T );
				vp = IC->get_pane();
				ILLC->SetViewport( vp->x0, vp->y0, vp->x1-vp->x0+1, vp->y1-vp->y0+1 );
				ILLC->SetFarClipDistance( IC->get_zfar() );
				ILLC->SetNearClipDistance( IC->get_znear() );
				ILLC->SetHorizontalFieldOfView( 2.0 * IC->get_fovx() );
				ILLC->SetAspect( ILLC_ASPECT_H2V, IC->get_aspect() );
			}
		}

		if( new_idx != INVALID_INSTANCE_INDEX ) {
			//
			// Update linkages
			//

			INSTANCE_INDEX child_idx = -1, parent_idx = d->_IModel->get_parent( idx );
			while( (child_idx = d->_IModel->get_child( idx, child_idx )) != INVALID_INSTANCE_INDEX ) {
				JOINT_INDEX joint_idx = d->_IModel->find_joint( idx, child_idx );
				const Joint *joint_info = d->_IModel->get_joint( joint_idx );
				Joint new_joint( *joint_info, new_idx, child_idx );
				d->_IModel->disconnect( idx, child_idx );
				d->_IModel->connect( &new_joint );
			}

			JOINT_INDEX joint_idx = d->_IModel->find_joint( parent_idx, idx );
			const Joint *joint_info = d->_IModel->get_joint( joint_idx );
			Joint new_joint( *joint_info, parent_idx, idx );
			d->_IModel->disconnect( parent_idx, idx );
			d->_IModel->connect( &new_joint );
		}

		//
		// Add the camera
		//
		d->_3DWindow->m_Cameras.insert( sz, cam );	


		return S_OK;
	}

	//

	static HRESULT ReplaceEngineLight( INSTANCE_INDEX idx, LPVOID context )
	{
		static U32 counter = 0;

		return S_OK;
	}

	//

	HRESULT ForEachItemOnObject( INSTANCE_INDEX idx, FEIOO_CB cb, LPVOID context )
	{
		COMPTR<IModel> IM;
		if( SUCCEEDED( m_IEngine->QueryInterface( IID_IModel, IM ) ) ) {
			INSTANCE_INDEX child_idx = -1;
			while( (child_idx = IM->get_child( idx, child_idx )) != INVALID_INSTANCE_INDEX ) {
				cb( child_idx, context );
				ForEachItemOnObject( child_idx, cb, context );
			}
		}
		return S_OK;
	}

	//

	template <class I1, class I2=IDAComponent, class I3=IDAComponent>
	struct OBJLISTNODEDATA
	{
		OBJLISTNODEDATA *next;
		OBJLISTNODEDATA *prev;
		char name[255+1];
		COMPTR< I1 > _I1;
		COMPTR< I2 > _I2;
		COMPTR< I3 > _I3;
	};

	template <class I1, class I2=IDAComponent, class I3=IDAComponent>
	struct OBJLIST
	{
		LList< OBJLISTNODEDATA< I1, I2, I3 > >	list;
		char IID_1[255+1];
		char IID_2[255+1];
		char IID_3[255+1];
	
		void SetIIDs( const char *IID_I1, const char *IID_I2="IDAComponent", const char *IID_I3="IDAComponent" )
		{
			strcpy( IID_1, IID_I1 );
			strcpy( IID_2, IID_I2 );
			strcpy( IID_3, IID_I3 );
		}

		U32 get_item_count()
		{
			return list.count();
		}

		OBJLISTNODEDATA< I1, I2, I3 > *first( )
		{
			return list.first();
		}

		OBJLISTNODEDATA< I1, I2, I3 > *last( )
		{
			return list.last();
		}

		OBJLISTNODEDATA< I1, I2, I3 > *insert( const char *name, IDAComponent *item )
		{
			OBJLISTNODEDATA< I1, I2, I3 > *d = list.alloc();
			strcpy( d->name, name );
			item->QueryInterface( IID_1, (void**) &d->_I1 );
			item->QueryInterface( IID_2, (void**) &d->_I2 );
			item->QueryInterface( IID_3, (void**) &d->_I3 );
			return d;
		}

		void remove( IDAComponent *item )
		{
			OBJLISTNODEDATA *d = list.first();
			while( d ) {
				if( item == d->_I1 ) {
					list.unlink( d );
					return;
				}
				d = d->next;
			}
		}

		void remove( const char *name )
		{
			OBJLISTNODEDATA< I1, I2, I3 > *d = list.first();
			while( d ) {
				if( strcmp( name, d->name ) == 0 ) {
					list.unlink( d );
					return;
				}
				d = d->next;
			}
		}

		OBJLISTNODEDATA< I1, I2, I3 > *find( const char *name )
		{
			OBJLISTNODEDATA< I1, I2, I3 > *d = list.first();
			while( d ) {
				if( strcmp( name, d->name ) == 0 ) {
					return d;
				}
				d = d->next;
			}
			return NULL;
		}
	};

	OBJLIST<IRenderable>										m_Renderables;
	OBJLISTNODEDATA<IRenderable,IDAComponent,IDAComponent>		*m_RenderablesIter;

	OBJLIST<ILowLevelCamera,ISceneCamera,ICamera>				m_Cameras;
	OBJLISTNODEDATA<ILowLevelCamera,ISceneCamera,ICamera>		*m_CamerasIter;

	OBJLIST<ILight>												m_Lights;
	OBJLISTNODEDATA<ILight>										*m_LightsIter;

	//

	HRESULT DB_Initialize()
	{
		m_Cameras.SetIIDs( IID_ILowLevelCamera, IID_ISceneCamera, IID_ICamera );
		m_Renderables.SetIIDs( IID_IRenderable );
		m_Lights.SetIIDs( IID_ILight );

		return S_OK;
	}

	//

	HRESULT DB_FindRenderables( )
	{
		m_RenderablesIter = NULL;

		if( m_Renderables.first() ) {
			return S_OK;
		}

		return E_FAIL;
	}

	//

	HRESULT DB_NextRenderable( IRenderable **out )
	{
		if( m_RenderablesIter == NULL ) {
			m_RenderablesIter = m_Renderables.first();
		}
		else {
			m_RenderablesIter = m_RenderablesIter->next;
		}

		if( m_RenderablesIter ) {
			m_RenderablesIter->_I1->QueryInterface( IID_IRenderable, (void**)out );
			return S_OK;
		}
		return E_FAIL;
	}

	//

	HRESULT DB_FindCameras( )
	{
		m_CamerasIter = NULL;

		if( m_Cameras.first() ) {
			return S_OK;
		}

		return E_FAIL;
	}

	//

	HRESULT DB_GetCameraCount( U32 *count )
	{
		*count = m_Cameras.get_item_count();
		return S_OK;
	}
	//

	HRESULT DB_NextCamera( ILowLevelCamera **out )
	{
		if( m_CamerasIter == NULL ) {
			m_CamerasIter = m_Cameras.first();
		}
		else {
			m_CamerasIter = m_CamerasIter->next;
		}

		if( m_CamerasIter ) {
			m_CamerasIter->_I1->QueryInterface( IID_ILowLevelCamera, (void**)out );
			return S_OK;
		}
		return E_FAIL;
	}

	//

	HRESULT DB_GetCameraName( ILowLevelCamera *cam, char *out, U32 size ) 
	{
		OBJLISTNODEDATA<ILowLevelCamera,ISceneCamera,ICamera> *c;
		c = m_Cameras.first();
		while( c ) {
			if( cam == c->_I1 ) {
				strncpy( out, c->name, size );
			}
			c = c->next;
		}
		return S_OK;
	}

	//

	HRESULT DB_FindCameraByName( const char *name, ILowLevelCamera **out )
	{
		OBJLISTNODEDATA<ILowLevelCamera,ISceneCamera,ICamera> *c;

		if( (c = m_Cameras.find( name )) != NULL ) {
			c->_I1->QueryInterface( IID_ILowLevelCamera, (void**)out );
			return S_OK;
		}
		return E_FAIL;
	}

	//

	HRESULT DB_FindLights( )
	{
		m_LightsIter = NULL;

		if( m_Lights.first() ) {
			return S_OK;
		}

		return E_FAIL;
	}

	//

	HRESULT DB_NextLight( ILight **out )
	{
		if( m_LightsIter == NULL ) {
			m_LightsIter = m_Lights.first();
		}
		else {
			m_LightsIter = m_LightsIter->next;
		}

		if( m_LightsIter ) {
			m_LightsIter->_I1->QueryInterface( IID_ILight, (void**)out );
			return S_OK;
		}
		return E_FAIL;
	}

	//

	HRESULT DB_GetLightName( ILight *cam, char *out, U32 size ) 
	{
		OBJLISTNODEDATA<ILight,IDAComponent,IDAComponent> *c;
		c = m_Lights.first();
		while( c ) {
			if( cam == c->_I1 ) {
				strncpy( out, c->name, size );
			}
			c = c->next;
		}
		return S_OK;
	}

	//

	HRESULT DB_FindLightByName( const char *name, ILight **out )
	{
		OBJLISTNODEDATA<ILight,IDAComponent,IDAComponent> *c;

		if( (c = m_Lights.find( name )) != NULL ) {
			c->_I1->QueryInterface( IID_ILight, (void**)out );
			return S_OK;
		}
		return E_FAIL;
	}

	//

	HRESULT DB_InsertObject( const char *name, IDAComponent *obj )
	{
		OBJLISTNODEDATA<IRenderable,IDAComponent,IDAComponent> *r;
		r = m_Renderables.insert( name, obj );
		if( r->_I1 == NULL ) {
			m_Renderables.remove( name );
		}
		
		OBJLISTNODEDATA<ILowLevelCamera,ISceneCamera,ICamera> *c;
		c = m_Cameras.insert( name, obj );
		if( c->_I1 == NULL ) {
			m_Cameras.remove( name );
		}
		
		OBJLISTNODEDATA<ILight,IDAComponent,IDAComponent> *l;
		l = m_Lights.insert( name, obj );
		if( l->_I1 == NULL ) {
			m_Lights.remove( name );
		}

		COMPTR<IDACOMEngineInstance> IEI;
		if( SUCCEEDED( obj->QueryInterface( IID_IDACOMEngineInstance, IEI ) ) ) {
			INSTANCE_INDEX idx;
			IEI->GetInstanceIndex( &idx );
			if( idx != INVALID_INSTANCE_INDEX ) {

				ITEMCBDATA data;
				
				data._3DWindow = this;
				m_IEngine->QueryInterface( IID_IModel, data._IModel );

				data.Items.reset();
				ForEachItemOnObject( idx, FindEngineCameras, &data );
				for( ITEMINFO *ii = data.Items.first(); ii; ii=ii->next ) {
					ReplaceEngineCamera( ii->item, &data );	
				}

#if 0
				data.Items.reset();
				ForEachItemOnObject( idx, FindEngineCameras, &data );
				for( ITEMINFO *ii = data.Items.first(); ii; ii=ii->next ) {
					ReplaceEngineCamera( ii->item, &data );	
				}
#endif
			}
		}
		return S_OK;
	}

	//



