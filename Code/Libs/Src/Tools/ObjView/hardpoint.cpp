//
// Hardpoint.cpp
//
//

#ifdef HARDPOINTS

#include <windows.h>
#include <vector>

//

#include "TSmartPointer.h"
#include "IHardPoint.h"
#include "RPUL.h"

//

#include "CmpndView.h"

//

struct render_hp
{
	HardpointInfo hp_info;
	INSTANCE_INDEX obj_id;
	char name[64];
};

//

struct hpconnectinfo
{
	U32 parent_hp_index;
	U32 child_hp_index;
};

//

typedef std::vector<render_hp> HardpointList;

//

HardpointList hp_list;
HardpointList child_hp_list;

//

int HardpointList_GetCount()
{
	return hp_list.size();;
}

//

void HardpointList_Clear( void )
{
	hp_list.clear();
}

//

void DrawHP( const render_hp & hp, U32 zenable )
{
	SetRenderVolume( NULL );

	RenderPrim->set_texture_stage_texture( 0, 0 );

	RenderPrim->set_render_state( D3DRS_ZENABLE, zenable );
	RenderPrim->set_render_state( D3DRS_LIGHTING, FALSE );
	
	float scale = 0.2f * global_scale;
	Vector p1, p2;
	Matrix orientation ( Engine->get_orientation(hp.obj_id) );
	Vector position ( Engine->get_position(hp.obj_id) );

	p1 = position + orientation * hp.hp_info.point;

	pb.Color4ub(255,255,255,255);
	pb.Begin(PB_POINTS);
	pb.Vertex3f(p1.x, p1.y, p1.z);
	pb.End();

	// draw axis
	if((hp.hp_info.type == JT_REVOLUTE) || (hp.hp_info.type == JT_PRISMATIC))
	{
		p2 = position + orientation * (hp.hp_info.point + 1.5f * scale * hp.hp_info.orientation * hp.hp_info.axis);
		pb.Color4ub(255,255,255,255);
		pb.Begin(PB_LINES);
		pb.Vertex3f(p1.x, p1.y, p1.z);
		pb.Vertex3f(p2.x, p2.y, p2.z);
		pb.End();
	}

	// draw limits
	if(hp.hp_info.type == JT_PRISMATIC)
	{
		pb.Color4ub(255,255,0,255);
		
		pb.Begin(PB_POINTS);
		p2 = position + orientation * (hp.hp_info.point + scale * hp.hp_info.min0 * hp.hp_info.orientation * hp.hp_info.axis);
		pb.Vertex3f(p2.x, p2.y, p2.z);
		
		p2 = position + orientation * (hp.hp_info.point + scale * hp.hp_info.max0 * hp.hp_info.orientation * hp.hp_info.axis);
		pb.Vertex3f(p2.x, p2.y, p2.z);
		pb.End();
	}

	RenderPrim->set_render_state( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	pb.Begin(PB_LINES);
	p2 = position + orientation * (hp.hp_info.point + scale * hp.hp_info.orientation.get_i());
	pb.Color4ub(255,0,0,255);
	pb.Vertex3f(p1.x, p1.y, p1.z);
	pb.Vertex3f(p2.x, p2.y, p2.z);
	
	p2 = position + orientation * (hp.hp_info.point + scale * hp.hp_info.orientation.get_j());
	pb.Color4ub(0,255,0,255);
	pb.Vertex3f(p1.x, p1.y, p1.z);
	pb.Vertex3f(p2.x, p2.y, p2.z);

	p2 = position + orientation * (hp.hp_info.point + scale * hp.hp_info.orientation.get_k());
	pb.Color4ub(0,0,255,255);
	pb.Vertex3f(p1.x, p1.y, p1.z);
	pb.Vertex3f(p2.x, p2.y, p2.z);
	pb.End();
	RenderPrim->set_render_state( D3DRS_ZFUNC, D3DCMP_LESS );

	Transform model_m(false);
	model_m.set_x_rotation( M_PI ); // make it face +Z
	model_m.translation = TheCamera->get_inverse_transform() * p1;

	RenderPipe->set_modelview( model_m );
	Font.SetSize( .0005f * global_scale );
	Font.RenderFormattedString( .0f, .0f, "%s", hp.name);
}

//

void HardpointList_Render( unsigned int which_hp, unsigned int flags )
{
	ASSERT( which_hp >= 1 );

	if( which_hp == 1 ) {
		// render all of the hps
		//
		for( int i = 0; i < hp_list.size(); i++ ) {
			DrawHP( hp_list[i], (flags&HIGH_BIT)?TRUE:FALSE );
		}
	}
	else {
		DrawHP( hp_list[which_hp - 2], (flags&HIGH_BIT)?TRUE:FALSE );
	}

}

//

struct HPL_AHP
{
	IHardpoint		*IHP;
	ARCHETYPE_INDEX	 arch_index;
	INSTANCE_INDEX	 inst_index;
	HardpointList	*hp_list;
};

//

void __cdecl HardpointList_AppendHP( const char *hp_name, void *user_data )
{
	HPL_AHP *data = (HPL_AHP*)user_data;
	render_hp rhp;

	data->IHP->retrieve_hardpoint_info( data->arch_index, hp_name, rhp.hp_info );

	strcpy( rhp.name, hp_name );
	rhp.obj_id = data->inst_index;

	data->hp_list->push_back( rhp );
}

//

void HardpointList_AppendToList( INSTANCE_INDEX root_inst_index, HardpointList *list )
{
	COMPTR<IHardpoint> IHP;

	if( FAILED( Engine->QueryInterface( IID_IHardpoint, (void**)&IHP ) ) ) {
		return;
	}
	
	HPL_AHP data;

	// do the root
	//
	ARCHETYPE_INDEX arch_index = Engine->get_instance_archetype( root_inst_index );

	data.IHP = IHP;
	data.inst_index = root_inst_index;
	data.arch_index = arch_index;
	data.hp_list = list;

	IHP->enumerate_hardpoints( HardpointList_AppendHP, arch_index, (void*)&data );
	
	Engine->release_archetype( arch_index );

	// now do children
	//
	INSTANCE_INDEX child = INVALID_INSTANCE_INDEX;

	while( (child = Engine->get_instance_child_next( root_inst_index, 0, child )) != INVALID_INSTANCE_INDEX ) {

		arch_index = Engine->get_instance_archetype( child );

		data.IHP = IHP;
		data.inst_index = child;
		data.arch_index = arch_index;

		IHP->enumerate_hardpoints( HardpointList_AppendHP, arch_index, (void*)&data );
		
		Engine->release_archetype( arch_index );
	}
}

//

void HardpointList_Append( INSTANCE_INDEX root_inst_index )
{
	HardpointList_AppendToList( root_inst_index, &hp_list );
}

//

void HardpointList_LoadChildObject( INSTANCE_INDEX *out_inst_index, char *out_filename )
{
	const char *FILTER = "Object Files (*.pte;*.3db;*.cmp)\0*.pte;*.3db;*.cmp;*.utf\0All Files (*.*)\0*.*\0\0";

	INSTANCE_INDEX inst_index ;

	*out_inst_index = INVALID_INSTANCE_INDEX;

	out_filename[0] = 0;

	if( !GetFileNameFromUser( hWnd, false, FILTER, out_filename, MAX_PATH ) ) {
		return;
	}

	
	COMPTR<IFileSystem> IFS;

	if( FAILED( Engine->create_file_system( out_filename, IFS ) ) ) {
		GENERAL_WARNING( TEMPSTR( "Unable to cfs for '%s'", out_filename ) ) ;
		return;
	}

	TextureLib->load_library( IFS, NULL );
	if( MaterialLib ) MaterialLib->load_library( IFS );

	*out_inst_index = inst_index = Engine->create_instance( out_filename, IFS, NULL );

	//LoadAnimation( out_filename, inst_index );

	TextureLib->get_texture_count( &texture_count );
	
	if( MaterialLib ) {
		MaterialLib->get_material_count( &material_count );
		MaterialLib->verify_library( 0, 1.0f );
	}
}

//

BOOL CALLBACK HardpointList_DlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static hpconnectinfo *info;
	static INSTANCE_INDEX child_inst_index = INVALID_INSTANCE_INDEX;
	static U32 dummy_hp_count = 0;

	switch( msg ) 
	{
		case WM_INITDIALOG:
		{
			info = reinterpret_cast<hpconnectinfo*>( lParam );

			HWND parent_hp_cb = GetDlgItem( hDlg, IDC_PARENT_HARDPOINT );
			HWND child_hp_cb = GetDlgItem( hDlg, IDC_CHILD_HARDPOINT );

			HardpointList::iterator beg = hp_list.begin();
			HardpointList::iterator end = hp_list.end();
			HardpointList::iterator hp;

			for( hp = beg; hp != end; hp++ ) {
				SendMessage( parent_hp_cb, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(hp->name) );
			}

			SendMessage( parent_hp_cb, CB_SETCURSEL, 0, 0 );

			child_inst_index = INVALID_INSTANCE_INDEX;

			EnableWindow( GetDlgItem( hDlg, IDOK ), 0 );
			EnableWindow( child_hp_cb, 0 );

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch( LOWORD(wParam) )
			{
				case IDOK:
				{
					HWND parent_hp_cb = GetDlgItem( hDlg, IDC_PARENT_HARDPOINT );
					HWND child_hp_cb = GetDlgItem( hDlg, IDC_CHILD_HARDPOINT );

					info->parent_hp_index = SendMessage( parent_hp_cb, CB_GETCURSEL, 0, 0 );
					info->child_hp_index = SendMessage( child_hp_cb, CB_GETCURSEL, 0, 0 );

					// need to account for current state of hplist as the child
					// hps will be appended to the hp list.
					//
					info->child_hp_index += hp_list.size();

					ASSERT( child_inst_index != INVALID_INSTANCE_INDEX );

					HardpointList_Append( child_inst_index );

					EndDialog( hDlg, IDOK );
					break;
				}

				case IDC_CHILD_OBJECT_BROWSE:
				{
					char filename[MAX_PATH];

					HWND child_hp_cb = GetDlgItem( hDlg, IDC_CHILD_HARDPOINT );

					SendMessage( child_hp_cb, CB_RESETCONTENT, 0, 0 );

					HardpointList_LoadChildObject( &child_inst_index, filename );

					if( child_inst_index != INVALID_INSTANCE_INDEX ) {
			
						COMPTR<IHardpoint> IHP;

						if( SUCCEEDED( Engine->QueryInterface( IID_IHardpoint, (void**)&IHP ) ) ) {

							child_hp_list.clear();

							HardpointList_AppendToList( child_inst_index, &child_hp_list );

							if( !child_hp_list.empty() ) {

								HardpointList::iterator beg = child_hp_list.begin();
								HardpointList::iterator end = child_hp_list.end();
								HardpointList::iterator hp;

								for( hp = beg; hp != end; hp++ ) {
									SendMessage( child_hp_cb, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(hp->name) );
								}

								SendMessage( child_hp_cb, CB_SETCURSEL, 0, 0 );
	
								EnableWindow( GetDlgItem( hDlg, IDOK ), 1 );
								EnableWindow( child_hp_cb, 1 );
							}
							else if( MessageBox( hDlg, 
												 "The object you have selected does not have any hardpoints.\n"
												 "\n"
												 "You can click OK and a dummy hardpoint will be inserted at the origin of the child\n"
												 "Or, you can click Cancel and select a new object.",
												 "Error", 
												 MB_OKCANCEL ) == IDOK ) {


								HardpointInfo hpinfo;
								char name[MAX_PATH], arch_name[MAX_PATH];
								ARCHETYPE_INDEX arch_index;

								arch_index = Engine->get_instance_archetype( child_inst_index );

								_splitpath( Engine->get_archetype_name( arch_index ), NULL, NULL, arch_name, NULL );

								memset( &hpinfo, 0, sizeof(hpinfo) );

								hpinfo.type = JT_FIXED;
								hpinfo.orientation.set_identity();

								sprintf( name, "_%s_hp_%d", arch_name, dummy_hp_count++ );

								IHP->set_hardpoint_info( arch_index,
														 name,
														 hpinfo );

								HardpointList_AppendToList( child_inst_index, &child_hp_list );

								SendMessage( child_hp_cb, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name) );
								SendMessage( child_hp_cb, CB_SETCURSEL, 0, 0 );
	
								EnableWindow( GetDlgItem( hDlg, IDOK ), 1 );
								EnableWindow( child_hp_cb, 1 );
							}
							else {	
								EnableWindow( GetDlgItem( hDlg, IDOK ), 0 );
								EnableWindow( child_hp_cb, 0 );
							}
						}

						SetDlgItemText( hDlg, IDC_CHILD_OBJECT, filename );
					}
					break;
				}

				case IDCANCEL:
					child_hp_list.clear();

					if( child_inst_index != INVALID_INSTANCE_INDEX ) {
						Engine->destroy_instance( child_inst_index );
					}

					EndDialog( hDlg, IDCANCEL );
					break;
			}

			break;
		}
	}

	return FALSE;
}

//

void HardpointList_Connect( void )
{
	COMPTR<IHardpoint> IHP;

	if( FAILED( Engine->QueryInterface( IID_IHardpoint, (void**)&IHP ) ) ) {
		return;
	}

	if( hp_list.empty() ) {
		MessageBox( hWnd, "There are no hardpoints on the current object(s)", "Error", MB_OK );
		return;
	}

	hpconnectinfo info;
	int ret;

	ret = DialogBoxParam( GetModuleHandle(NULL), 
						  MAKEINTRESOURCE(IDD_CONNECT_OBJECT), 
						  hWnd, 
						  HardpointList_DlgProc, 
						  (LPARAM)&info );

	if( ret == IDCANCEL ) {
		return;
	}

	const render_hp *parent_hp = &hp_list[info.parent_hp_index];
	const render_hp *child_hp  = &hp_list[info.child_hp_index];

	ret = IHP->connect( parent_hp->obj_id, 
						parent_hp->name, 
						child_hp->obj_id, 
						child_hp->name );

	if( ret != 0 ) {
		MessageBox( hWnd, "Unable to connect hardpoints", "Error", MB_OK );
	}
}

//

#endif