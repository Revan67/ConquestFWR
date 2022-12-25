//----------------------------------------------------------------------------------------------
//
// CQ2MatEditor.h
//
// To inform the plug-in about and contain the material list from CQ2
//
//----------------------------------------------------------------------------------------------

#ifndef _MATERIAL_EDITOR_HEADER_H_FILE_
#define _MATERIAL_EDITOR_HEADER_H_FILE_

#include <windows.h>
#include <string>
#include <list>
#include <IMaterialManager.h>

typedef DWORD MatHandle;

class CQ2MatEditor : public IMaterialCallback
{
public:
	enum
	{
		MATHANDLE_INVALID = 0,
	};

private:
	struct Material
	{
		std::string diffuseFilename;
		std::string materialName;
		MatHandle   matHandle;

		Material() : matHandle(0) {}

		Material( const char* _fn, const char* _mn, MatHandle _matHandle )
		{
			diffuseFilename = _fn;
			materialName    = _mn;
			matHandle       = _matHandle;
		}

		Material( const Material& _mat )
		{
			diffuseFilename = _mat.diffuseFilename;
			materialName    = _mat.materialName;
			matHandle       = _mat.matHandle;
		}
	};

	struct MaterialList : std::list<Material>
	{
	};

	MaterialList m_MaterialList;
	Material*    m_CurrentMat;

public:
	CQ2MatEditor();
	~CQ2MatEditor();

	// Initializes the list of CQ2 materials
	// If _comboBox is non-NULL, this will be filled out with the list
	// 
	bool Init( HWND _comboBox ); 

	// Gets the diffuse channel's texture filename by handle
	// 
	const char* GetFilename( MatHandle _handle );

	// Gets the material name by handle
	// 
	const char* GetMatname( MatHandle _handle );

	// Sets the material name to use, sets m_CurrentMat
	// returns the material handle
	//
	MatHandle SetMaterialName( const char* _materialName );

	// Fills out RichEdit control with the material's properties, can also set up the current selected on the combo box
	//
	bool FillOutMatProps( MatHandle _handle, HWND _richEdit, HWND _comboBox );

	// Gets a handle to a material 
	// 
	MatHandle GetMatHandle( const char* _materialName );

	// Starts the material editor's window for editing CQ2 materials
	// Note: Will send the "current materaial" as set by SetMaterialName()
	//
	void StartEditor( void );

protected:

	// IMaterialCallback
	virtual void Added( IMaterial* );
	virtual void Changed( IMaterial* );
	virtual void Removed( IMaterial* );

protected:

	bool loadMaterials( void );

	Material* getMat( MatHandle _handle );

};

#endif