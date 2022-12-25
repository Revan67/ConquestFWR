#ifndef _INTERNAL_MATERIAL_MANAGER_H_
#define _INTERNAL_MATERIAL_MANAGER_H_

#include "dacom.h"

#include "rendpipeline.h"

struct Modifier;

enum KRS_ValueType
{
	KRS_BOOL,
	KRS_NAMED,
	KRS_INTEGER,
	KRS_FLOAT,
};
enum MatDataType
{
	MDT_FLOAT,
	MDT_INT,
	MDT_COLOR,
	MDT_TEXTURE,
	MDT_STATE,
	MDT_ENUM,
	MDT_ANIMUV,
};

struct DataNode
{
	DataNode * next;
	char name[32];
	bool advanced;
	MatDataType dataType;
	union
	{
		struct FloatData
		{
			MatRegisterType rType;
			U32 rValue;
			SINGLE min;
			SINGLE max;
			SINGLE def;
		}floatData;
		struct IntData
		{
			MatRegisterType rType;
			U32 rValue;
			S32 min;
			S32 max;
			S32 def;
		}intData;
		struct ColorData
		{
			MatRegisterType rType;
			U32 rValue;
			U8 defRed;
			U8 defGreen;
			U8 defBlue;
		}colorData;
		struct TextureData
		{
			U32 channel;
		}textureData;
		struct StateData
		{
			DWORD state;
			DWORD defValue;
		}stateData;
		struct AnimUVData
		{
			U32 rValue;
		}animUVData;
	}data;
};

struct DACOM_NO_VTABLE IShader
{
	virtual char * GetName() = 0;

	virtual DataNode * GetDataList() = 0;

	virtual DataNode * FindNode(const char * name) = 0;

	virtual U32 GetNumEnumStates(const char * enumName) = 0;

	virtual const char * GetEnumStateName(const char * enumName,U32 index) = 0;

	virtual bool TestEnumMatch(const char * enumName,const char * valueName,struct Material * mat) = 0;

	virtual void SetEnumActive(const char * enumName,const char * valueName,struct Material * mat) = 0;

	virtual const char * GetFilename() = 0;
};

struct DACOM_NO_VTABLE IInternalMaterialManager
{

	virtual IRenderPipeline * GetPipe() = 0;

	virtual ITManager * GetTManager() = 0;

	virtual void SetDialog(HWND hwnd) = 0;

	virtual struct IFileSystem * GetDirectory() = 0;

	virtual struct IFileSystem * GetMatDir() = 0;

	virtual struct IFileSystem * GetShaderDir() = 0;

	virtual struct IFileSystem * GetTextureDir() = 0;

	virtual struct Material * FindMaterial(const char * filename) = 0;

	virtual struct Material * GetFirstMaterial() = 0;

	virtual struct Material * GetNextMaterial(Material * mat) = 0;

	virtual struct Material * LoadNewMaterial(const char * filename) = 0;

	virtual void DeleteMaterial(Material * mat) = 0;

	virtual void DeleteModifier(Modifier * mod) = 0;

	virtual IShader * GetFirstShader() = 0;

	virtual IShader * GetNextShader(IShader * shader) = 0;

	virtual IShader * FindShader(const char * name) = 0;

	virtual KRS_ValueType GetMaterialStateValueType(U32 state) = 0;

	virtual U32 GetNumMaterialStateValues(U32 state) = 0;

	virtual char * GetMaterialStateValueName(U32 state, U32 index) = 0;

	virtual DWORD GetMaterialStateValue(U32 state, U32 index) = 0;

	virtual Material * MakeNewMaterial(char * dirName) = 0;

	virtual void SaveAll() = 0;

	virtual void ReloadAll() = 0;

	virtual struct IMaterialCallback * GetCallback() = 0;
};

#endif
