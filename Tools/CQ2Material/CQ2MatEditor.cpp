//----------------------------------------------------------------------------------------------
//
// CQ2MatEditor.cpp
//
// To inform the plug-in about and contain the material list from CQ2
//
//----------------------------------------------------------------------------------------------

#include "CQ2MatEditor.h"

#include "CQ2Material.h"
#include <windowsx.h>
#include <FileSys.h>

namespace dacom
{
	bool init = false;
	int nextMadId = 1;
	IMaterialManager* materialManager = NULL;
	IFileSystem* materialFS = NULL;

	void Init( void )
	{
		if( !init )
		{
			// init the file system
			{
				GENRESULT gr = DACOM->AddLibrary("DOSFile.dll");

				if( gr != GR_OK )
				{
					gr = DACOM->AddLibrary("Z:\\CQ2\\Code\\Libs\\ExplicitDLL\\DOSFile.dll");
				}

				if( gr == GR_OK )
				{
					IFileSystem* filesys = NULL;

					DAFILEDESC fdesc;
					fdesc = "Z:\\CQ2\\DATA\\MAT";
					if( DACOM->CreateInstance(&fdesc, (void **)&filesys) == GR_OK )
					{
						materialFS = filesys;
					}
				}
			}

			// init the material manager
			{
				GENRESULT gr = DACOM->AddLibrary("MaterialManager.dll");

				if( gr != GR_OK )
				{
					gr = DACOM->AddLibrary("Z:\\CQ2\\Code\\Libs\\ExplicitDLL\\MaterialManager.dll");
				}

				if( gr == GR_OK )
				{
					AGGDESC adesc = "IMaterialManager";
					if (DACOM->CreateInstance(&adesc, (void **) &materialManager) == GR_OK)
					{
						IMaterialManager::InitInfo info;
						info.MATDIR = materialFS;

						if( materialManager->Initialize(info) == GR_OK )
						{
							init = true;
						}
					}
				}
			}
		}
	}
};

//----------------------------------------------------------------------------------------------

CQ2MatEditor::CQ2MatEditor()
{
	m_CurrentMat = NULL;
}

//----------------------------------------------------------------------------------------------

CQ2MatEditor::~CQ2MatEditor()
{
	m_CurrentMat = NULL;
}

//----------------------------------------------------------------------------------------------

bool CQ2MatEditor::Init( HWND _comboBox )
{ 
	dacom::Init();
	loadMaterials();

	if( _comboBox )
	{
		ComboBox_ResetContent( _comboBox );

		for( MaterialList::iterator it = m_MaterialList.begin(); it != m_MaterialList.end(); it++ )
		{
			DWORD idx = ComboBox_AddString( _comboBox, it->materialName.c_str() );
			ComboBox_SetItemData( _comboBox, idx, it->matHandle );
		}
	}

	return( m_MaterialList.size() > 0 ); 
}

//----------------------------------------------------------------------------------------------

const char* CQ2MatEditor::GetFilename( MatHandle _handle )
{ 
	Material* m = getMat(_handle);
	if( m )
	{
		return m->diffuseFilename.c_str();
	}
	return NULL; 
}

//----------------------------------------------------------------------------------------------

const char* CQ2MatEditor::GetMatname( MatHandle _handle )
{
	Material* m = getMat(_handle);
	if( m )
	{
		return m->materialName.c_str();
	}
	return NULL; 
}

//----------------------------------------------------------------------------------------------

MatHandle CQ2MatEditor::SetMaterialName( const char* _materialName )
{
	for( MaterialList::iterator it = m_MaterialList.begin(); it != m_MaterialList.end(); it++ )
	{
		Material& mat = *it;

		if( mat.materialName == _materialName )
		{
			m_CurrentMat = &mat;
			return mat.matHandle;
		}
	}
	return MATHANDLE_INVALID; 
}

//----------------------------------------------------------------------------------------------

bool CQ2MatEditor::FillOutMatProps( MatHandle _handle, HWND _richEdit, HWND _comboBox )
{
	if( _richEdit )
	{
		Material* m = getMat( _handle );
		if( m )
		{
			std::string msg;
			msg += m->materialName;
			msg += " = ";
			msg += m->diffuseFilename;

			::SetWindowText( _richEdit, msg.c_str() );

			int idx = ComboBox_FindString( _comboBox, 0, m->materialName.c_str() );
			if( idx != CB_ERR )
			{
				ComboBox_SetCurSel( _comboBox, idx );
			}

			return true;
		}
	}
	return false;
}

//----------------------------------------------------------------------------------------------

void CQ2MatEditor::StartEditor( void )
{
	if( dacom::materialManager )
	{
		dacom::materialManager->OpenEditWindow(this);
	}
}

//----------------------------------------------------------------------------------------------

bool CQ2MatEditor::loadMaterials( void )
{
	m_MaterialList.clear();

	if( dacom::materialManager )
	{
		IMaterial* mat = NULL;

		if( dacom::materialManager->FindFirstMaterial(&mat) == GR_OK )
		{
			while( mat )
			{
				Material m;
				m.diffuseFilename = mat->GetDefaultBaseTexture();
				m.materialName    = mat->GetName();
				m.matHandle       = dacom::nextMadId++;

				m_MaterialList.push_back( m );

				dacom::materialManager->FindNextMaterial(mat,&mat);
			}
		}
	}

	return true;
}

//----------------------------------------------------------------------------------------------

CQ2MatEditor::Material* CQ2MatEditor::getMat( MatHandle _handle )
{
	for( MaterialList::iterator it = m_MaterialList.begin(); it != m_MaterialList.end(); it++ )
	{
		Material& mat = *it;

		if( mat.matHandle == _handle )
		{
			return &mat;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------------------------
// IMaterialCallback

void CQ2MatEditor::Added( IMaterial* _material )
{
}

void CQ2MatEditor::Changed( IMaterial* _material )
{
}

void CQ2MatEditor::Removed( IMaterial* _material )
{
}
