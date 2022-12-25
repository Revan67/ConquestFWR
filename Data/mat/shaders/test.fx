//BEGIN_MAT_SCRIPT
//NAME "Test Shader"
//PARAM "float1" FLOATP 0 1.0 2.0 1.5
//PARAM "float2" FLOATP 1 1.0 10000.0 1.5
//PARAM "float3" FLOATP 2 0.0 10.0 1.5
//PARAM "floatV" FLOATV 1 0.0 100.0 1.5
//PARAM "floatC" FLOATC 5 0.0 50.0 1.5
//ADVANCED "int1" INTP 0 0 10 1
//ADVANCED "int2" INTP 1 10 15 10
//PARAM "int3" INTP 2 -5 40 1
//PARAM "intV" INTV 1 0 10 1
//PARAM "intC" INTC 43 0 10 1
//ADVANCED "blend1" STATE D3DRS_SRCBLEND D3DBLEND_ONE
//ADVANCED "blend2" STATE D3DRS_DESTBLEND D3DBLEND_ONE
//ADVANCED "boolTest" STATE D3DRS_ZWRITEENABLE true
//PARAM "ColorP" COLORP 10 255 0 0
//PARAM "ColorV" COLORV 10 0 255 0
//ADVANCED "ColorC" COLORC 10 0 0 255
//PARAM "Normal/Spec" TEXTURE 2
//BEGIN_ENUM "enum"
//ENUM_VALUE "Addative"
//ENUM_SET "blend1" D3DBLEND_ONE
//ENUM_SET "blend2" D3DBLEND_ONE
//ENUM_VALUE "Multiply"
//ENUM_SET "blend1" D3DBLEND_INVSRCALPHA
//ENUM_SET "blend2" D3DBLEND_SRCALPHA
//ENUM_END
//END_MAT_SCRIPT

#define InputFlatColor c42

#define Light0ObjPos    c7
#define Light0Color     c8
#define Light0Atten     c9
#define Light1ObjPos    c10
#define Light1Color     c11
#define Light1Atten	c12

#define Mproj        	c66

#define MyConstants 	c78
#define Zero            c78.x
#define One             c78.y
#define Two		c78.z 
#define Half		c78.w 

#define cameraPositionObjSpace	   c79



#define VertexPosition  v0 
#define VertexNormal	v1
#define VertexUV        v2 
#define VertexUV2       v3 
#define VertexBinorm	v4
#define VertexTangent	v5
#define VertexExtra	v6

#define RTANGENT r8
#define RBINORM r3

VertexShader NewVS = asm 
{ 
    vs.1.1
    def MyConstants, 0,1,2,.5
    ; input declaration
    dcl_position0 VertexPosition
    dcl_normal0 VertexNormal
    dcl_texcoord0 VertexUV
    dcl_texcoord1 VertexUV2
    dcl_texcoord3 VertexBinorm
    dcl_texcoord2 VertexTangent
    dcl_texcoord4 VertexExtra

    ;transform position to the projection space 
    m4x4 oPos, VertexPosition, Mproj
    mov r1, InputFlatColor
    mov oD0, r1
    mov oT0, VertexUV
};

PixelShader psone = asm
{
	ps.1.1
	tex t0
	mad r0, t0, v0, c0
};

technique PS11
{
	pass P0
	{
	  VertexShader = (NewVS);
          PixelShader  = (psone);
	  ADDRESSU[0] = WRAP;
	  ADDRESSV[0] = WRAP;

	}
};







