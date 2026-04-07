//----------------------------------------------------------------------------------------------
//
// MaterialManager.cpp 
//
//----------------------------------------------------------------------------------------------

#include "stdafx.h"
#include <IMaterialManager.h>
#include "InternalMaterialManager.h"
#include "Material.h"
#include "Modifier.h"
#include <d3d9.h>

#include "resource.h"

#include <tcomponent.h>
#include <da_heap_utility.h>
#include <filesys.h>
#include <commctrl.h>
#include <TSmartPointer.h>

#include <string>
#include <list>
//-----------------------------------------------------------------------------------------------
//script constants
#define SC_BEGIN_SCRIPT "BEGIN_MAT_SCRIPT"
#define SC_END_SCRIPT	"END_MAT_SCRIPT"
#define SC_PARAM		"PARAM"
#define SC_ADVANCED		"ADVANCED"
#define SC_NAME			"NAME"
#define SC_FLOATP		"FLOATP"
#define SC_FLOATV		"FLOATV"
#define SC_FLOATC		"FLOATC"
#define SC_INTP			"INTP"
#define SC_INTC			"INTC"
#define SC_INTV			"INTV"
#define SC_COLORP		"COLORP"
#define SC_COLORV		"COLORV"
#define SC_COLORC		"COLORC"
#define SC_TEXTURE		"TEXTURE"
#define SC_STATE		"STATE"
#define SC_BEGIN_ENUM   "BEGIN_ENUM"
#define SC_END_ENUM     "END_ENUM"
#define SC_ENUM_VALUE   "ENUM_VALUE"
#define SC_ENUM_SET		"ENUM_SET"
#define SC_ANIMUV		"ANIM_UV"

//-------------------------------------------------------------------------------------------------
//render state arrays

#define MAX_MAT_RENDER_STATE_VALUES 16

struct KnownRenderStateDef
{
	char name[32];
	DWORD state;
	KRS_ValueType valueType;
	struct NamedValues
	{
		char name[32];
		DWORD value;
	}namedValues[MAX_MAT_RENDER_STATE_VALUES];
};

#define MATRS_ADD_BOOL(STATENAME) {#STATENAME,STATENAME,KRS_BOOL,{{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0}}},
#define MATRS_ADD_NAMED(STATENAME,NAME1,NAME2,NAME3,NAME4,NAME5,NAME6,NAME7,NAME8,NAME9,NAME10,NAME11,NAME12,NAME13,NAME14,NAME15,NAME16) {#STATENAME,STATENAME,KRS_NAMED,{{#NAME1,NAME1},{#NAME2,NAME2},{#NAME3,NAME3},{#NAME4,NAME4},{#NAME5,NAME5},{#NAME6,NAME6},{#NAME7,NAME7},{#NAME8,NAME8},{#NAME9,NAME9},{#NAME10,NAME10},{#NAME11,NAME11},{#NAME12,NAME12},{#NAME13,NAME13},{#NAME14,NAME14},{#NAME15,NAME15},{#NAME16,NAME16}}},

#define MATRS_END() {"",0,KRS_BOOL,{{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0},{"",0}}}

KnownRenderStateDef matRenderState[] = { 
	MATRS_ADD_BOOL(D3DRS_ZENABLE)
	MATRS_ADD_NAMED(D3DRS_FILLMODE, D3DFILL_POINT, D3DFILL_WIREFRAME, D3DFILL_SOLID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
	MATRS_ADD_NAMED(D3DRS_SHADEMODE, D3DSHADE_FLAT , D3DSHADE_GOURAUD , D3DSHADE_PHONG , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
	MATRS_ADD_BOOL(D3DRS_ZWRITEENABLE)
	MATRS_ADD_NAMED(D3DRS_SRCBLEND, D3DBLEND_ZERO , D3DBLEND_ONE  , D3DBLEND_SRCCOLOR  , D3DBLEND_INVSRCCOLOR , D3DBLEND_SRCALPHA , D3DBLEND_INVSRCALPHA , D3DBLEND_DESTALPHA , D3DBLEND_INVDESTALPHA ,D3DBLEND_DESTCOLOR , D3DBLEND_INVDESTCOLOR , D3DBLEND_SRCALPHASAT , D3DBLEND_BOTHSRCALPHA , D3DBLEND_BOTHINVSRCALPHA , D3DBLEND_BLENDFACTOR , D3DBLEND_INVBLENDFACTOR , 0)
	MATRS_ADD_NAMED(D3DRS_DESTBLEND, D3DBLEND_ZERO , D3DBLEND_ONE  , D3DBLEND_SRCCOLOR  , D3DBLEND_INVSRCCOLOR , D3DBLEND_SRCALPHA , D3DBLEND_INVSRCALPHA , D3DBLEND_DESTALPHA , D3DBLEND_INVDESTALPHA ,D3DBLEND_DESTCOLOR , D3DBLEND_INVDESTCOLOR , D3DBLEND_SRCALPHASAT , D3DBLEND_BOTHSRCALPHA , D3DBLEND_BOTHINVSRCALPHA , D3DBLEND_BLENDFACTOR , D3DBLEND_INVBLENDFACTOR , 0)
	MATRS_ADD_NAMED(D3DRS_CULLMODE, D3DCULL_NONE  , D3DCULL_CW  , D3DCULL_CCW , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
	MATRS_END()
};

//-------------------------------------------------------------------------------------------------


// to load IDD_DIALOG
HINSTANCE g_hInstance = 0;

struct MaterialList : std::list<Material>
{

};

//----------------------------------------------------------------------------------------------

struct SetNode
{
	SetNode * next;
	DataNode * targetNode;
	union
	{
		struct FloatData
		{
			SINGLE value;
		}floatData;
		struct IntData
		{
			S32 value;
		}intData;
		struct ColorData
		{
			U8 redValue;
			U8 greenValue;
			U8 blueValue;
		}colorData;
		struct StateData
		{
			DWORD value;
		}stateData;
	}data;
};

struct ShaderNode : IShader
{
	ShaderNode * next;
	char filename[256];
	char displayName[64];

	char * data;
	DataNode * dataList;

	struct EnumNode
	{
		EnumNode * next;
		char name[32];
		struct EnumValue
		{
			EnumValue * next;
			char name[32];
			SetNode * setList;
		};
		EnumValue * valueList;
	};

	EnumNode * enumList;

	ShaderNode()
	{
		dataList = NULL;
		data = NULL;
		enumList = NULL;
	}

	~ShaderNode()
	{
		while(enumList)
		{
			EnumNode * tmp = enumList;
			enumList = enumList->next;
			while(tmp->valueList)
			{
				EnumNode::EnumValue * tmpV = tmp->valueList;
				tmp->valueList = tmp->valueList->next;
				while(tmpV->setList)
				{
					SetNode * tmpN = tmpV->setList;
					tmpV->setList = tmpV->setList->next;
					delete tmpN;
				}
				delete tmpV;
			}
			delete tmp;
		}
		while(dataList)
		{
			DataNode * tmp = dataList;
			dataList = dataList->next;
			delete tmp;
		}
		delete [] data;
		data = NULL;
	}

	void InsertData(DataNode * node)
	{
		DataNode * search = dataList;
		DataNode * prev = NULL;
		while(search)
		{
			prev = search;
			search = search->next;
		}
		node->next = NULL;
		if(prev)
			prev->next = node;
		else
			dataList = node;
	};
	//IShader

	virtual char * GetName()
	{
		return displayName;
	}

	virtual DataNode * GetDataList()
	{
		return dataList;
	}

	virtual DataNode * FindNode(const char * name)
	{
		DataNode * search = dataList;
		while(search)
		{
			if(strcmp(search->name,name) == 0)
			{
				return search;
			}
			search = search->next;
		}
		return NULL;
	}

	virtual U32 GetNumEnumStates(const char * enumName)
	{
		EnumNode * searchEnum = enumList;
		while(searchEnum)
		{
			if(strcmp(searchEnum->name,enumName) == 0)
			{
				U32 count = 0;
				EnumNode::EnumValue * valueSearch = searchEnum->valueList;
				while(valueSearch)
				{
					++count;
					valueSearch = valueSearch->next;
				}
				return count;
			}
			searchEnum = searchEnum->next;
		}
		return 0;
	}

	virtual const char * GetEnumStateName(const char * enumName,U32 index)
	{
		EnumNode * searchEnum = enumList;
		while(searchEnum)
		{
			if(strcmp(searchEnum->name,enumName) == 0)
			{
				U32 count = index;
				EnumNode::EnumValue * valueSearch = searchEnum->valueList;
				while(valueSearch)
				{
					if(!count)
					{
						return valueSearch->name;
					}
					--count;
					valueSearch = valueSearch->next;
				}
				return "";
			}
			searchEnum = searchEnum->next;
		}
		return "";
	}

	virtual bool TestEnumMatch(const char * enumName,const char * valueName,Material * mat)
	{
		EnumNode * searchEnum = enumList;
		while(searchEnum)
		{
			if(strcmp(searchEnum->name,enumName) == 0)
			{
				EnumNode::EnumValue * valueSearch = searchEnum->valueList;
				while(valueSearch)
				{
					if(strcmp(valueSearch->name,valueName) == 0)
					{
						SetNode * set = valueSearch->setList;
						while(set)
						{
							if(set->targetNode->dataType == MDT_FLOAT)
							{
								if(mat->GetFloat(set->targetNode->data.floatData.rType,set->targetNode->data.floatData.rValue) != set->data.floatData.value)
									return false;
							}
							else if(set->targetNode->dataType == MDT_INT)
							{
								if(mat->GetInt(set->targetNode->data.intData.rType,set->targetNode->data.intData.rValue) != set->data.intData.value)
									return false;
							}
							else if(set->targetNode->dataType == MDT_COLOR)
							{
								U8 red;
								U8 green;
								U8 blue;
								mat->GetColor(set->targetNode->data.colorData.rType,set->targetNode->data.colorData.rValue,red,green,blue);
								if(red != set->data.colorData.redValue || green != set->data.colorData.greenValue || blue != set->data.colorData.blueValue)
                                    return false;
							}
							else if(set->targetNode->dataType == MDT_STATE)
							{
								if(mat->GetRenderStateValue(set->targetNode->data.stateData.state) != set->data.stateData.value)
									return false;
							}
							set = set->next;
						}
						return true;
					}
					valueSearch = valueSearch->next;
				}
				return false;
			}
			searchEnum = searchEnum->next;
		}
		return false;
	}

	virtual void SetEnumActive(const char * enumName,const char * valueName,Material * mat)
	{
		EnumNode * searchEnum = enumList;
		while(searchEnum)
		{
			if(strcmp(searchEnum->name,enumName) == 0)
			{
				EnumNode::EnumValue * valueSearch = searchEnum->valueList;
				while(valueSearch)
				{
					if(strcmp(valueSearch->name,valueName) == 0)
					{
						SetNode * set = valueSearch->setList;
						while(set)
						{
							if(set->targetNode->dataType == MDT_FLOAT)
							{
								mat->SetFloat(set->targetNode->data.floatData.rType,set->targetNode->data.floatData.rValue,set->data.floatData.value);
							}
							else if(set->targetNode->dataType == MDT_INT)
							{
								mat->SetInt(set->targetNode->data.intData.rType,set->targetNode->data.intData.rValue,set->data.intData.value);
							}
							else if(set->targetNode->dataType == MDT_COLOR)
							{
								mat->SetColor(set->targetNode->data.colorData.rType,set->targetNode->data.colorData.rValue,
									set->data.colorData.redValue,set->data.colorData.greenValue,set->data.colorData.blueValue);
							}
							else if(set->targetNode->dataType == MDT_STATE)
							{
								mat->SetRenderStateValue(set->targetNode->data.stateData.state,set->data.stateData.value);
							}
							set = set->next;
						}
						return;
					}
					valueSearch = valueSearch->next;
				}
				return;
			}
			searchEnum = searchEnum->next;
		}
		return;
	}

	virtual const char * GetFilename()
	{
		return filename;
	}
};

//----------------------------------------------------------------------------------------------

#define CLSID_MaterialManager "IMaterialManager"

//----------------------------------------------------------------------------------------------
// DlgProc
INT_PTR CALLBACK MaterialDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
//----------------------------------------------------------------------------------------------

struct MaterialManager : IMaterialManager, public IAggregateComponent, IInternalMaterialManager
{
	BEGIN_DACOM_MAP_INBOUND(MaterialManager)
		DACOM_INTERFACE_ENTRY(IMaterialManager)
		DACOM_INTERFACE_ENTRY2(IID_IMaterialManager,IMaterialManager)
		DACOM_INTERFACE_ENTRY(IAggregateComponent)
		DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

	// IAggregateComponent 
	GENRESULT COMAPI Initialize(void) { return GR_OK; }
	GENRESULT COMAPI init( AGGDESC *desc );

	// IMaterialManager

	virtual GENRESULT COMAPI Initialize( InitInfo& );
	virtual GENRESULT COMAPI OpenEditWindow( IMaterialCallback* );
	virtual GENRESULT COMAPI IsEditWindowOpen( void );
	virtual GENRESULT COMAPI FindFirstMaterial( IMaterial** ppMaterial );
	virtual GENRESULT COMAPI FindNextMaterial( IMaterial* pThisMat, IMaterial** ppNextMat );

	//IInternalMaterialManager

	virtual IRenderPipeline * GetPipe();

	virtual ITManager * GetTManager();

	virtual void SetDialog(HWND hwnd);

	virtual struct IFileSystem * GetDirectory();

	virtual struct IFileSystem * GetMatDir();

	virtual struct IFileSystem * GetTextureDir();


	virtual struct IFileSystem * GetShaderDir();

	virtual struct Material * FindMaterial(const char * filename);

	virtual struct Material * GetFirstMaterial();

	virtual struct Material * GetNextMaterial(Material * mat);

	virtual struct Material *LoadNewMaterial(const char * filename);

	virtual void DeleteMaterial(Material * mat);

	virtual void DeleteModifier(Modifier * mod);

	virtual IShader * GetFirstShader();

	virtual IShader * GetNextShader(IShader * shader);

	virtual IShader * FindShader(const char * name);

	virtual KRS_ValueType GetMaterialStateValueType(U32 state);

	virtual U32 GetNumMaterialStateValues(U32 state);

	virtual char * GetMaterialStateValueName(U32 state, U32 index);

	virtual DWORD GetMaterialStateValue(U32 state, U32 index);

	virtual Material *  MakeNewMaterial(char * dirName);

	virtual void SaveAll();

	virtual void ReloadAll();

	virtual IMaterialCallback * GetCallback();


	// data (needs a real implementation)
	MaterialList       materialList;
	IMaterialCallback* callback;
	HWND               hDialog;
	IFileSystem*       MATDIR;
	IRenderPipeline * PIPE;

	ITManager * TMANAGER;

	COMPTR<IFileSystem> SHADERDIR;
	COMPTR<IFileSystem> TEXTUREDIR;
	COMPTR<IFileSystem> MATDEFDIR;

	ShaderNode *		shaders;

	// locals

	void enumerateMatDir(char * path,IFileSystem * matDir);

	GENRESULT getMaterials( void );

	void parseShaders();

	void parseShader(IFileSystem * inFile, const char * filename);

	void reloadShader(ShaderNode * node);

	void parseFloatParam(DataNode * node,char * currentLine);

	void parseIntParam(DataNode * node,char * currentLine);

	void parseColorParam(DataNode * node,char * currentLine);

	void parseAnimUVParam(DataNode * node,char * currentLine);

	void parseTextureParam(DataNode * node,char * currentLine);

	void parseStateParam(DataNode * node,char * currentLine);

	void parseParam(DataNode * node,char * currentLine);

	void parseEnumNode(DataNode * node,char * currentLine);

	void parseEnumValue(ShaderNode * node, char * currentLine);

	void parseEnumSet(ShaderNode * node, char * currentLine);

	bool parseCommand(ShaderNode * node,char * currentLine,bool & bFinished);

	bool isAtEnd(char * string);

	char * getNextLine(char * string);

	char * nextWord(char * string);

	void getNextWord(char * string, char * word);

	DWORD getRenderStateFromString(char * stateName);

	DWORD getRenderStateValueFromString(char * valueName, DWORD state);

	MaterialManager()
	{
		hDialog = NULL;
		callback = NULL;
		MATDIR = NULL;
		shaders = NULL;
	}

	~MaterialManager();

};

MaterialManager::~MaterialManager()
{
	while(shaders)
	{
		ShaderNode * tmp = shaders;
		shaders = shaders->next;
		delete tmp;
	}
}

//----------------------------------------------------------------------------------------------

GENRESULT COMAPI MaterialManager::init( AGGDESC *desc )
{
	return GR_OK;
}

//----------------------------------------------------------------------------------------------

GENRESULT COMAPI MaterialManager::Initialize( InitInfo& initInfo )
{
	MATDIR = initInfo.MATDIR;
	PIPE = initInfo.PIPE;
	TMANAGER = initInfo.TMANAGER;
	DAFILEDESC fdesc = "shaders";
	MATDIR->CreateInstance(&fdesc,SHADERDIR);
	fdesc.lpFileName = "textures";
	MATDIR->CreateInstance(&fdesc,TEXTUREDIR);
	fdesc.lpFileName = "materials";
	MATDIR->CreateInstance(&fdesc,MATDEFDIR);

	parseShaders();

	if( getMaterials() != GR_OK )
	{
		return GR_GENERIC;
	}

	return GR_OK;
}

//----------------------------------------------------------------------------------------------

GENRESULT COMAPI MaterialManager::OpenEditWindow( IMaterialCallback* _callback )
{
	if( _callback )
	{
		callback = _callback;
	}

	if( !hDialog )
	{
		hDialog = ::CreateDialogParam( g_hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, MaterialDialogProc, (DWORD)((IInternalMaterialManager*)this) );
		if( hDialog )
		{
			return GR_OK;
		}
	}

	return GR_GENERIC;
}

//----------------------------------------------------------------------------------------------

GENRESULT COMAPI MaterialManager::IsEditWindowOpen( void )
{
	if( hDialog )
	{
		return GR_OK;
	}
	return GR_GENERIC;
}

//----------------------------------------------------------------------------------------------

GENRESULT COMAPI MaterialManager::FindFirstMaterial( IMaterial** ppMaterial )
{
	if( ppMaterial && materialList.size() > 0 )
	{
		*ppMaterial = &materialList.front();
		return GR_OK;
	}
	return GR_GENERIC;
}

//----------------------------------------------------------------------------------------------

GENRESULT COMAPI MaterialManager::FindNextMaterial( IMaterial* pThisMat, IMaterial** ppNextMat )
{
	if( ppNextMat && pThisMat )
	{
		*ppNextMat = NULL;

		for( MaterialList::iterator it = materialList.begin(); it != materialList.end(); it++ )
		{
			Material& mat = *it;
			if( &mat == pThisMat )
			{
				// getting the "next" material
				it++;
				if( it == materialList.end() )
				{
					// done
					return GR_OK;
				}
				else
				{
					*ppNextMat = &(*it);
					return GR_OK;
				}
			}
		}
	}

	return GR_GENERIC;
}

//----------------------------------------------------------------------------------------------

IRenderPipeline* MaterialManager::GetPipe()
{
	return PIPE;
}

ITManager * MaterialManager::GetTManager()
{
	return TMANAGER;
}


void MaterialManager::SetDialog(HWND hwnd)
{
	hDialog = hwnd;
}
//----------------------------------------------------------------------------------------------
IFileSystem * MaterialManager::GetDirectory()
{
	return MATDIR;
}
//----------------------------------------------------------------------------------------------
//
struct IFileSystem * MaterialManager::GetMatDir()
{
	return MATDEFDIR;
}

struct IFileSystem * MaterialManager::GetTextureDir()
{
	return TEXTUREDIR;
}

//----------------------------------------------------------------------------------------------
//
struct IFileSystem * MaterialManager::GetShaderDir()
{
	return SHADERDIR;
}
//----------------------------------------------------------------------------------------------
//
struct Material * MaterialManager::FindMaterial(const char * filename)
{
	for( MaterialList::iterator it = materialList.begin(); it != materialList.end(); it++ )
	{
		Material& mat = *it;
		if(mat.szFileName == filename)
		{
			return &mat;
		}
	}
	return NULL;
}
//----------------------------------------------------------------------------------------------
//
struct Material * MaterialManager::GetFirstMaterial()
{
	if(materialList.size() > 0 )
	{
		return &materialList.front();
	}
	return NULL;
}
//----------------------------------------------------------------------------------------------
//
struct Material * MaterialManager::GetNextMaterial(Material * matCheck)
{
	if( matCheck )
	{
		for( MaterialList::iterator it = materialList.begin(); it != materialList.end(); it++ )
		{
			Material& mat = *it;
			if( &mat == matCheck )
			{
				// getting the "next" material
				it++;
				if( it == materialList.end() )
				{
					// done
					return NULL;
				}
				else
				{
					return &(*it);
				}
			}
		}
	}
	return NULL;
}
//----------------------------------------------------------------------------------------------
//
struct Material *MaterialManager::LoadNewMaterial(const char * filename)
{
	Material mat(this);//material list will make a copy.
	mat.Load(MATDEFDIR,filename);
	materialList.push_back(mat);
	if(callback)
		callback->Added(FindMaterial(filename));
	return &(materialList.back());
}
//----------------------------------------------------------------------------------------------
//
void MaterialManager::DeleteMaterial(Material * delMat)
{
	for( MaterialList::iterator it = materialList.begin(); it != materialList.end(); it++ )
	{
		Material& mat = *it;
		if((&mat) == delMat)
		{
			if(callback)
				callback->Removed(delMat);

			materialList.erase(it);
			return;
		}
	}
}
//----------------------------------------------------------------------------------------------
//
void MaterialManager::DeleteModifier(Modifier * mod)
{
	//may want to track these guys
	delete mod;
}
//----------------------------------------------------------------------------------------------
//
IShader * MaterialManager::GetFirstShader()
{
	return shaders;
}
//----------------------------------------------------------------------------------------------
//
IShader * MaterialManager::GetNextShader(IShader * shader)
{
	if(shader)
	{
		ShaderNode * sh = (ShaderNode *)shader;
		return sh->next;
	}
	return NULL;
}
//----------------------------------------------------------------------------------------------
//
IShader * MaterialManager::FindShader(const char * name)
{
	ShaderNode * search = shaders;
	while(search)
	{
		if(strcmp(name,search->displayName) == 0)
			return search;
		search = search->next;
	}
	return NULL;
}
//----------------------------------------------------------------------------------------------
//
KRS_ValueType MaterialManager::GetMaterialStateValueType(U32 state)
{
	U32 index = 0;
	while(matRenderState[index].name[0])
	{
		if(matRenderState[index].state == state)
		{
			return matRenderState[index].valueType;
		}
		++index;
	}
	return KRS_BOOL;
}
//----------------------------------------------------------------------------------------------
//
Material *  MaterialManager::MakeNewMaterial(char * dirName)
{
	char extension[] = ".txt";
	char testName[256];
	U32 testNum = 0;
	do
	{
		++testNum;
		sprintf(testName,"%sNewMat%d%s",dirName,testNum,extension);
	}while(FindMaterial(testName));

	Material mat(this);//material list will make a copy.
	mat.Rename(testName);
	mat.Save(MATDEFDIR);
	materialList.push_back(mat);
	return &(materialList.back());
}
//----------------------------------------------------------------------------------------------
//
void MaterialManager::SaveAll()
{
	for( MaterialList::iterator it = materialList.begin(); it != materialList.end(); it++ )
	{
		Material& mat = *it;
		mat.Save(MATDEFDIR);
	}
}
//----------------------------------------------------------------------------------------------
//
void MaterialManager::ReloadAll()
{
	ShaderNode * node = shaders;
	while(node)
	{
		reloadShader(node);
		node = node->next;
	}

	//ok now remap shaders to materials
	for( MaterialList::iterator it = materialList.begin(); it != materialList.end(); it++ )
	{
		Material& mat = *it;
		mat.SetShader(FindShader(mat.szShaderName.c_str()));
	}
}
//----------------------------------------------------------------------------------------------
//
IMaterialCallback * MaterialManager::GetCallback()
{
	return callback;
}
//----------------------------------------------------------------------------------------------
//
U32 MaterialManager::GetNumMaterialStateValues(U32 state)
{
	U32 index = 0;
	while(matRenderState[index].name[0])
	{
		if(matRenderState[index].state == state)
		{
			U32 count = 0;
			while(count < MAX_MAT_RENDER_STATE_VALUES && matRenderState[index].namedValues[count].name[0] != '0')
			{
				++count;
			}
			return count;
		}
		++index;
	}
	return 0;
}
//----------------------------------------------------------------------------------------------
//
char * MaterialManager::GetMaterialStateValueName(U32 state, U32 nameIndex)
{
	U32 index = 0;
	while(matRenderState[index].name[0])
	{
		if(matRenderState[index].state == state)
		{
			return matRenderState[index].namedValues[nameIndex].name;
		}
		++index;
	}
	return "";
}
//----------------------------------------------------------------------------------------------
//
DWORD MaterialManager::GetMaterialStateValue(U32 state, U32 nameIndex)
{
	U32 index = 0;
	while(matRenderState[index].name[0])
	{
		if(matRenderState[index].state == state)
		{
			return matRenderState[index].namedValues[nameIndex].value;
		}
		++index;
	}
	return 0;
}
//----------------------------------------------------------------------------------------------
//
void MaterialManager::enumerateMatDir(char * path,IFileSystem * matDir)
{
	WIN32_FIND_DATA findData;
	HANDLE hFile = matDir->FindFirstFile("*", &findData);
	if( hFile != INVALID_HANDLE_VALUE )
	{
		do
		{
			if( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY  && findData.cFileName[0] != '.')
			{
				COMPTR<IFileSystem> subDir;
				DAFILEDESC fdesc = findData.cFileName;
				if(matDir->CreateInstance(&fdesc,subDir) == GR_OK)
				{
					char buffer[512];
					sprintf(buffer,"%s%s\\",path,findData.cFileName);
					enumerateMatDir(buffer,subDir);
				}
			}
			else
			{
				if( strstr(findData.cFileName,".txt") )
				{
					Material mat(this);//material list will make a copy.
					char buffer[512];
					sprintf(buffer,"%s%s",path,findData.cFileName);
					mat.Load(MATDEFDIR,buffer);
					materialList.push_back(mat);
				}
			}
		}
		while( matDir->FindNextFile(hFile, &findData) );
		matDir->FindClose(hFile);
	}
}
//----------------------------------------------------------------------------------------------

GENRESULT MaterialManager::getMaterials( void )
{
	if(MATDEFDIR)
	{
		enumerateMatDir("",MATDEFDIR);
	}
	return GR_OK;
}
//----------------------------------------------------------------------------------------------
void MaterialManager::parseShaders()
{
	if(SHADERDIR)
	{
		WIN32_FIND_DATA findData;
		HANDLE hFile = SHADERDIR->FindFirstFile("*.fx", &findData);
		if( hFile != INVALID_HANDLE_VALUE )
		{
			do
			{
				if( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					//do nothing
				}
				else
				{
					DAFILEDESC fdesc = findData.cFileName;
					COMPTR<IFileSystem> shaderFile;
					if(SHADERDIR->CreateInstance(&fdesc,shaderFile) == GR_OK)
					{
						parseShader(shaderFile,findData.cFileName);
					}
				}
			}
			while( SHADERDIR->FindNextFile(hFile, &findData) );
			SHADERDIR->FindClose(hFile);
		}
	}
}
//----------------------------------------------------------------------------------------------
void MaterialManager::parseShader(IFileSystem * inFile, const char * filename)
{
	ShaderNode * node = new ShaderNode;

	strcpy(node->filename,filename);
	strcpy(node->displayName,"Error: No DisplayName");
	node->next = shaders;
	shaders = node;

	node->data = new char[inFile->GetFileSize()+1];
	DWORD dwBytesRead;
	inFile->ReadFile( NULL, node->data, inFile->GetFileSize(), &dwBytesRead );
	node->data[inFile->GetFileSize()] = EOF;

	char * currentLine = node->data;
	while(!isAtEnd(currentLine))
	{
		if(strncmp(currentLine,SC_BEGIN_SCRIPT,strlen(SC_BEGIN_SCRIPT)) == 0)
		{
			currentLine = getNextLine(currentLine);
			//ok now parse the shader lines
			bool bFinished = false;
			while(!bFinished)
			{
				char * lineEnd = getNextLine(currentLine);
				while(currentLine != lineEnd)
				{
					if(parseCommand(node,currentLine,bFinished))
						currentLine = lineEnd;
					else
						++currentLine;
				}
				++currentLine;
			}
			break;
		}
		++currentLine;
	}
}
//----------------------------------------------------------------------------------------------
void MaterialManager::reloadShader(ShaderNode * node)
{
	if(SHADERDIR)
	{
		DAFILEDESC fdesc = node->filename;
		COMPTR<IFileSystem> inFile;
		if(SHADERDIR->CreateInstance(&fdesc,inFile) == GR_OK)
		{
			strcpy(node->displayName,"Error: No DisplayName");
			while(node->dataList)
			{
				DataNode * tmp = node->dataList;
				node->dataList = node->dataList->next;
				delete tmp;
			}

			delete [] node->data;
			node->data = new char[inFile->GetFileSize()+1];
			DWORD dwBytesRead;
			inFile->ReadFile( NULL, node->data, inFile->GetFileSize(), &dwBytesRead );
			node->data[inFile->GetFileSize()] = EOF;

			char * currentLine = node->data;
			while(!isAtEnd(currentLine))
			{
				if(strncmp(currentLine,SC_BEGIN_SCRIPT,strlen(SC_BEGIN_SCRIPT)) == 0)
				{
					currentLine = getNextLine(currentLine);
					//ok now parse the shader lines
					bool bFinished = false;
					while(!bFinished)
					{
						char * lineEnd = getNextLine(currentLine);
						while(currentLine != lineEnd)
						{
							if(parseCommand(node,currentLine,bFinished))
								currentLine = lineEnd;
							else
								++currentLine;
						}
						++currentLine;
					}
					break;
				}
				++currentLine;
			}
		}
	}
}
//----------------------------------------------------------------------------------------------
void MaterialManager::parseFloatParam(DataNode * node,char * currentLine)
{
	node->data.floatData.rValue = atoi(currentLine);
	currentLine = nextWord(currentLine);
	node->data.floatData.min = (SINGLE)atof(currentLine);
	currentLine = nextWord(currentLine);
	node->data.floatData.max = (SINGLE)atof(currentLine);
	currentLine = nextWord(currentLine);
	node->data.floatData.def = (SINGLE)atof(currentLine);
}
//----------------------------------------------------------------------------------------------
void MaterialManager::parseIntParam(DataNode * node,char * currentLine)
{
	node->data.intData.rValue = atoi(currentLine);
	currentLine = nextWord(currentLine);
	node->data.intData.min = atoi(currentLine);
	currentLine = nextWord(currentLine);
	node->data.intData.max = atoi(currentLine);
	currentLine = nextWord(currentLine);
	node->data.intData.def = atoi(currentLine);
}
//----------------------------------------------------------------------------------------------
void MaterialManager::parseColorParam(DataNode * node,char * currentLine)
{
	node->data.colorData.rValue = atoi(currentLine);
	currentLine = nextWord(currentLine);
	node->data.colorData.defRed = atoi(currentLine);
	currentLine = nextWord(currentLine);
	node->data.colorData.defGreen = atoi(currentLine);
	currentLine = nextWord(currentLine);
	node->data.colorData.defBlue = atoi(currentLine);
}
//----------------------------------------------------------------------------------------------
void MaterialManager::parseAnimUVParam(DataNode * node,char * currentLine)
{
	node->data.animUVData.rValue = atoi(currentLine);
}
//----------------------------------------------------------------------------------------------
void MaterialManager::parseTextureParam(DataNode * node,char * currentLine)
{
	node->data.textureData.channel = atoi(currentLine);
}
//----------------------------------------------------------------------------------------------
void MaterialManager::parseStateParam(DataNode * node,char * currentLine)
{
	char renderStateString[256];
	char valueString[256];
	getNextWord(currentLine,renderStateString);
	currentLine = nextWord(currentLine);
	getNextWord(currentLine,valueString);
	node->data.stateData.state = getRenderStateFromString(renderStateString);
	node->data.stateData.defValue = getRenderStateValueFromString(valueString,node->data.stateData.state);
}
//----------------------------------------------------------------------------------------------
void MaterialManager::parseParam(DataNode * node,char * currentLine)
{
	char * str1 = strchr(currentLine,'\"');
	str1++;
	char * str2 = strchr(str1,'\"');
	if(str2-str1 >0)
	{
		strncpy(node->name,str1,str2-str1);
		node->name[str2-str1] = 0;
	}
	else
		node->name[0] = 0;

	currentLine = nextWord(str2);

	//pixel float
	if(strncmp(currentLine,SC_FLOATP,strlen(SC_FLOATP)) == 0)
	{
		node->dataType = MDT_FLOAT;
		node->data.floatData.rType = MRT_PIXEL;
		currentLine = nextWord(currentLine);
		parseFloatParam(node,currentLine);		
	}
	//vertex float
	else if(strncmp(currentLine,SC_FLOATV,strlen(SC_FLOATV)) == 0)
	{
		node->dataType = MDT_FLOAT;
		node->data.floatData.rType = MRT_VERTEX;
		currentLine = nextWord(currentLine);
		parseFloatParam(node,currentLine);		
	}
	//code float
	else if(strncmp(currentLine,SC_FLOATC,strlen(SC_FLOATC)) == 0)
	{
		node->dataType = MDT_FLOAT;
		node->data.floatData.rType = MRT_CODE;
		currentLine = nextWord(currentLine);
		parseFloatParam(node,currentLine);		
	}
	//pixel int
	else if(strncmp(currentLine,SC_INTP,strlen(SC_INTP)) == 0)
	{
		node->dataType = MDT_INT;
		node->data.intData.rType = MRT_PIXEL;
		currentLine = nextWord(currentLine);
		parseIntParam(node,currentLine);		
	}
	//vertex int
	else if(strncmp(currentLine,SC_INTV,strlen(SC_INTV)) == 0)
	{
		node->dataType = MDT_INT;
		node->data.intData.rType = MRT_VERTEX;
		currentLine = nextWord(currentLine);
		parseIntParam(node,currentLine);		
	}
	//code int
	else if(strncmp(currentLine,SC_INTC,strlen(SC_INTC)) == 0)
	{
		node->dataType = MDT_INT;
		node->data.intData.rType = MRT_CODE;
		currentLine = nextWord(currentLine);
		parseIntParam(node,currentLine);		
	}
	//pixel color
	else if(strncmp(currentLine,SC_COLORP,strlen(SC_COLORP)) == 0)
	{
		node->dataType = MDT_COLOR;
		node->data.colorData.rType = MRT_PIXEL;
		currentLine = nextWord(currentLine);
		parseColorParam(node,currentLine);		
	}
	//vertex color
	else if(strncmp(currentLine,SC_COLORV,strlen(SC_COLORV)) == 0)
	{
		node->dataType = MDT_COLOR;
		node->data.colorData.rType = MRT_VERTEX;
		currentLine = nextWord(currentLine);
		parseColorParam(node,currentLine);		
	}
	//code color
	else if(strncmp(currentLine,SC_COLORC,strlen(SC_COLORC)) == 0)
	{
		node->dataType = MDT_COLOR;
		node->data.colorData.rType = MRT_CODE;
		currentLine = nextWord(currentLine);
		parseColorParam(node,currentLine);		
	}
	//textue chanel
	else if(strncmp(currentLine,SC_TEXTURE,strlen(SC_TEXTURE)) == 0)
	{
		node->dataType = MDT_TEXTURE;
		currentLine = nextWord(currentLine);
		parseTextureParam(node,currentLine);		
	}
	//render state
	else if(strncmp(currentLine,SC_STATE,strlen(SC_STATE)) == 0)
	{
		node->dataType = MDT_STATE;
		currentLine = nextWord(currentLine);
		parseStateParam(node,currentLine);		
	}
	else if(strncmp(currentLine,SC_ANIMUV,strlen(SC_ANIMUV)) == 0)
	{
		node->dataType = MDT_ANIMUV;
		currentLine = nextWord(currentLine);
		parseAnimUVParam(node,currentLine);		
	}
}
//----------------------------------------------------------------------------------------------
void MaterialManager::parseEnumNode(DataNode * node,char * currentLine)
{
	char * str1 = strchr(currentLine,'\"');
	str1++;
	char * str2 = strchr(str1,'\"');
	if(str2-str1 >0)
	{
		strncpy(node->name,str1,str2-str1);
		node->name[str2-str1] = 0;
	}
	else
		node->name[0] = 0;

	node->dataType = MDT_ENUM;
}
//----------------------------------------------------------------------------------------------
void MaterialManager::parseEnumValue(ShaderNode * node, char * currentLine)
{
	if(node->enumList)
	{
		ShaderNode::EnumNode::EnumValue * value = new ShaderNode::EnumNode::EnumValue;
		value->setList = NULL;
		char * str1 = strchr(currentLine,'\"');
		str1++;
		char * str2 = strchr(str1,'\"');
		if(str2-str1 >0)
		{
			strncpy(value->name,str1,str2-str1);
			value->name[str2-str1] = 0;
		}
		else
			value->name[0] = 0;

		value->next = node->enumList->valueList;
		node->enumList->valueList = value;
	}
}
//----------------------------------------------------------------------------------------------
void MaterialManager::parseEnumSet(ShaderNode * shader, char * currentLine)
{
	if(shader->enumList && shader->enumList->valueList)
	{
		char buffer[32];
		char * str1 = strchr(currentLine,'\"');
		str1++;
		char * str2 = strchr(str1,'\"');
		if(str2-str1 >0)
		{
			strncpy(buffer,str1,str2-str1);
			buffer[str2-str1] = 0;
		}
		else
			buffer[0] = 0;

		DataNode * targetNode = shader->FindNode(buffer);
		if(targetNode)
		{
			SetNode * node = new SetNode;
			node->targetNode = targetNode;

			currentLine = nextWord(str2);

			//pixel float
			if(targetNode->dataType == MDT_FLOAT)
			{
				node->data.floatData.value = (SINGLE)atof(currentLine);
			}
			//pixel int
			else if(targetNode->dataType == MDT_INT)
			{
				node->data.intData.value = atoi(currentLine);
			}
			//pixel color
			else if(targetNode->dataType == MDT_COLOR)
			{
				node->data.colorData.redValue = atoi(currentLine);
				currentLine = nextWord(currentLine);
				node->data.colorData.greenValue = atoi(currentLine);
				currentLine = nextWord(currentLine);
				node->data.colorData.blueValue = atoi(currentLine);
			}
			//render state
			else if(targetNode->dataType == MDT_STATE)
			{
				char valueString[256];
				getNextWord(currentLine,valueString);
				node->data.stateData.value = getRenderStateValueFromString(valueString,targetNode->data.stateData.state);
			}
			node->next = shader->enumList->valueList->setList;
			shader->enumList->valueList->setList = node;
		}
	}
}
//----------------------------------------------------------------------------------------------
bool MaterialManager::parseCommand(ShaderNode * node,char * currentLine,bool & bFinished)
{
	//NAME commande
	if(strncmp(currentLine,SC_NAME,strlen(SC_NAME)) == 0)
	{
		char * str1 = strchr(currentLine,'\"');
		str1++;
		char * str2 = strchr(str1,'\"');
		if(str2-str1 >0)
		{
			strncpy(node->displayName,str1,str2-str1);
			node->displayName[str2-str1] = 0;
		}
		return true;
	}
	//parameter
	else if(strncmp(currentLine,SC_PARAM,strlen(SC_PARAM)) == 0)
	{
		DataNode * dataNode = new DataNode;
		dataNode->advanced = false;
		currentLine = nextWord(currentLine);
		parseParam(dataNode,currentLine);
		node->InsertData(dataNode);
		return true;
	}
	//advanced param
	else if(strncmp(currentLine,SC_ADVANCED,strlen(SC_ADVANCED)) == 0)
	{
		DataNode * dataNode = new DataNode;
		dataNode->advanced = true;
		currentLine = nextWord(currentLine);
		parseParam(dataNode,currentLine);
		node->InsertData(dataNode);
		return true;
	}
	else if(strncmp(currentLine,SC_BEGIN_ENUM,strlen(SC_BEGIN_ENUM)) == 0)
	{
		DataNode * dataNode = new DataNode;
		dataNode->advanced = false;
		currentLine = nextWord(currentLine);
		parseEnumNode(dataNode,currentLine);
		node->InsertData(dataNode);

		ShaderNode::EnumNode * eNode = new ShaderNode::EnumNode();
		strcpy(eNode->name,dataNode->name);
		eNode->valueList = NULL;
		eNode->next = node->enumList;
		node->enumList = eNode;

		return true;
	}
	else if(strncmp(currentLine,SC_ENUM_VALUE,strlen(SC_ENUM_VALUE)) == 0)
	{
		if(node->enumList)
		{
			currentLine = nextWord(currentLine);
			parseEnumValue(node,currentLine);
		}
		return true;
	}
	else if(strncmp(currentLine,SC_ENUM_SET,strlen(SC_ENUM_SET)) == 0)
	{
		if(node->enumList)
		{
			currentLine = nextWord(currentLine);
			parseEnumSet(node,currentLine);
		}
		return true;
	}
	else if(strncmp(currentLine,SC_END_ENUM,strlen(SC_END_ENUM)) == 0)
	{
		return true;
	}
	//endScript command
	else if(strncmp(currentLine,SC_END_SCRIPT,strlen(SC_END_SCRIPT)) == 0)
	{
		bFinished = true;
		return true;
	}
	return false;
}
//----------------------------------------------------------------------------------------------
bool MaterialManager::isAtEnd(char * string)
{
	if(!string)
		return true;
	if(string[0] == EOF)
		return true;
	return false;
}
//----------------------------------------------------------------------------------------------
char * MaterialManager::getNextLine(char * string)
{
	while((string[0] != EOF) && (string[0] != '\r'))
	{
		++string;
	}
	if(string[0] != EOF)
		++string;
	return string;
}
//----------------------------------------------------------------------------------------------
char * MaterialManager::nextWord(char * string)
{
	while(string[0] != EOF && !isspace(string[0]))
	{
		++string;
	}
	while(string[0] != EOF && isspace(string[0]))
	{
		++string;
	}
	return string;
}

//----------------------------------------------------------------------------------------------

void MaterialManager::getNextWord(char * string, char * word)
{
	U32 index = 0;
	while((!isspace(string[index])) && string[index] != EOF)
	{
		word[index] = string[index];
		++index;
	}
	word[index] = 0;
}

//----------------------------------------------------------------------------------------------

DWORD MaterialManager::getRenderStateFromString(char * stateName)
{
	U32 index = 0;
	while(matRenderState[index].name[0])
	{
		if(strcmp(matRenderState[index].name,stateName) == 0)
		{
			return matRenderState[index].state;
		}
		++index;
	}
	return 0;
}

//----------------------------------------------------------------------------------------------

DWORD MaterialManager::getRenderStateValueFromString(char * valueName, DWORD state)
{
	U32 index = 0;
	while(matRenderState[index].name[0])
	{
		if(matRenderState[index].state == state)
		{
			if(matRenderState[index].valueType == KRS_NAMED)
			{
				for(U32 valueIndex = 0; valueIndex < MAX_MAT_RENDER_STATE_VALUES; ++valueIndex)
				{
					if(strcmp(matRenderState[index].namedValues[valueIndex].name,valueName) == 0)
					{
						return matRenderState[index].namedValues[valueIndex].value;
					}
				}
			}
			else if(matRenderState[index].valueType == KRS_BOOL)
			{
				if(strcmp(valueName,"TRUE") == 0 || strcmp(valueName,"True") == 0 || strcmp(valueName,"true") == 0 ||
					strcmp(valueName,"1") == 0)
					return 1;
				return 0;
			}
			else if(matRenderState[index].valueType == KRS_INTEGER)
			{
				return atoi(valueName);
			}
			else if(matRenderState[index].valueType == KRS_FLOAT)
			{
				SINGLE value = (SINGLE)(atof(valueName));
				U32 retVal = *((U32*)(void*)(&value));
				return retVal;
			}
			return 0;
		}
		++index;
	}
	return 0;
}

//----------------------------------------------------------------------------------------------

BOOL COMAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
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

			InitCommonControls();

			// Register System aggragate factory
			if( DACOM && (server = new DAComponentFactory2<DAComponentAggregate<MaterialManager>, AGGDESC>(CLSID_MaterialManager)) != NULL ) 
			{
				g_hInstance = hinstDLL;
				DACOM->RegisterComponent( server, CLSID_MaterialManager, DACOM_NORMAL_PRIORITY );
				server->Release();
			}
			
			break;
		}

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}

