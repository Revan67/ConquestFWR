#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "dacom.h"
#include <string>

struct MatFloat
{
	MatFloat * next;
	char name[32];
	SINGLE value;
	U32 reg;
};

struct MatInt
{
	MatInt * next;
	char name[32];
	S32 value;
	U32 reg;
};

struct MatColor
{
	MatColor * next;
	char name[32];
	U8 red;
	U8 green;
	U8 blue;
	U32 reg;
};

struct MatTexture
{
	MatTexture * next;
	char name[32];
	char filename[64];
	U32 channel;
	U32 textureID;
};

struct MatState
{
	MatState * next;
	char name[32];
	DWORD state;
	DWORD value;
};

struct MatAnimUV
{
	char name[32];
	U32 reg;
	U32 rows;
	U32 columns;
	SINGLE frameRate;
};

//----------------------------------------------------------------------------------------------

struct Material : public IMaterial
{
	//IMaterial
	virtual void DrawVB(IDirect3DVertexBuffer9* VB, IDirect3DIndexBuffer9* IB, int start_vertex, int num_verts, int start_index, int num_indices, IModifier * modList,SINGLE frameTime);
	
	virtual void DrawUP(void* vertices, U16* indices, int num_verts, int num_indices, IModifier * modList);

	virtual CQ_VertexType GetVertexType();
	
	virtual const char* GetDefaultBaseTexture( void );

	virtual const char* GetName( void );

	virtual SINGLE GetFloat(MatRegisterType type, U32 index);

	virtual SINGLE GetFloat(const char * name);

	virtual void SetFloat(MatRegisterType type, U32 index, SINGLE value);

	virtual void SetFloat(const char * name, SINGLE value);

	virtual S32 GetInt(MatRegisterType type, U32 index);

	virtual S32 GetInt(const char * name);

	virtual void SetInt(MatRegisterType type, U32 index, S32 value);

	virtual void SetInt(const char * name, S32 value);

	virtual void GetColor(MatRegisterType type, U32 index,U8 & red, U8 & green, U8 & blue);

	virtual void GetColor(const char * name,U8 & red, U8 & green, U8 & blue);

	virtual void SetColor(MatRegisterType type, U32 index, U8 red, U8 green, U8 blue);

	virtual void SetColor(const char * name, U8 red, U8 green, U8 blue);

	virtual const char * GetTextureName(U32 index);

	virtual const char * GetTextureName(const char * name);

	virtual void SetTextureName(U32 index, const char * newName);

	virtual void SetTextureName(const char * name, const char * newName);

	virtual DWORD GetRenderStateValue(U32 state);

	virtual DWORD GetRenderStateValue(const char * name);

	virtual void SetRenderStateValue(U32 state, DWORD value);

	virtual void SetRenderStateValue(const char * name, DWORD value);

	virtual U32 GetAnimUVColumns();

	virtual void SetAnimUVColumns(U32 numColumns);

	virtual U32 GetAnimUVRows();

	virtual void SetAnimUVRows(U32 numRows);

	virtual SINGLE GetAnimUVFrameRate();

	virtual void SetAnimUVFrameRate(SINGLE newFrameRate);

	virtual void Realize();

	virtual bool IsRealized();

	virtual IModifier * CreateModifierInt(char * name,S32 value,IModifier * oldModifier);

	virtual IModifier * CreateModifierFloat(char * name,SINGLE value,IModifier * oldModifier);

	virtual IModifier * CreateModifierColor(char * name,U8 red, U8 green, U8 blue,IModifier * oldModifier);

	virtual IModifier * CreateModifierRenderState(U32 state, DWORD value,IModifier * oldModifier);

	//Material

	Material(struct IInternalMaterialManager * _owner);

	Material(const Material & source);

	~Material();

	void SetShaderConstants(IModifier * modList,SINGLE frameTime);

	bool PreparePass(int pass);

	bool EndPass(int pass);

	void Load(struct IFileSystem * matDir, const char * filename);

	void Save(struct IFileSystem * matDir);

	void Rename(const char * filename);

	void SetShader(struct IShader * newShader);

	void RemapParameters();

	//data
	bool bChanged;
	bool bRealized;

	std::string szMaterialName;//material name
	std::string szFileName;//filename name
	std::string szShaderName;

	ID3DXEffect** shaderEffect;
	struct IShader * shader;
	struct IInternalMaterialManager * owner;

	MatFloat * floatList[3];//three list, one for each register type
	MatInt * intList[3];
	MatColor * colorList[3];
	MatTexture * textureList;
	MatState * stateList;

	MatAnimUV * animUV;

	float * pixelConst;
	U32 startPixelIndex;
	U32 pixelConstCount;

	float * vertexConst;
	U32 startVertexIndex;
	U32 vertexConstCount;
};

#endif
