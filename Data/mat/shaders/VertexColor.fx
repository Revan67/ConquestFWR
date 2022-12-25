
//BEGIN_MAT_SCRIPT
//NAME "Vertex Color with Texture"

// VERTEXTYPE VT_COLORED_TEXTURE

//PARAM "ColorMod" COLORV 124 255 255 255
//PARAM "Opacity" FLOATV 127 0 1 1

//PARAM "Diffuse + Opacity" TEXTURE 0

//ADVANCED "SrcBlend" STATE D3DRS_SRCBLEND D3DBLEND_ONE
//ADVANCED "DstBlend" STATE D3DRS_DESTBLEND D3DBLEND_ZERO
//ADVANCED "ZWrite" STATE D3DRS_ZWRITEENABLE true
//ADVANCED "ZTest" STATE D3DRS_ZENABLE true
//BEGIN_ENUM "enum"
//ENUM_VALUE "Opaque"
//ENUM_SET "DstBlend" D3DBLEND_ZERO
//ENUM_SET "SrcBlend" D3DBLEND_ONE
//ENUM_SET "ZTest" true
//ENUM_VALUE "Transparent"
//ENUM_SET "DstBlend" D3DBLEND_INVSRCALPHA
//ENUM_SET "SrcBlend" D3DBLEND_SRCALPHA
//ENUM_SET "ZTest" true
//ENUM_VALUE "Additive"
//ENUM_SET "SrcBlend" D3DBLEND_ONE
//ENUM_SET "DstBlend" D3DBLEND_ONE
//ENUM_SET "ZTest" false
//ENUM_VALUE "SQUARED"
//ENUM_SET "DstBlend" D3DBLEND_ZERO
//ENUM_SET "SrcBlend" D3DBLEND_SRCCOLOR
//ENUM_SET "ZTest" true
//ENUM_END


//PARAM "AnimateUV" ANIM_UV 120
//PARAM "TexScaleX" FLOATV 122 -30 30 1
//PARAM "TexScaleY" FLOATV 123 -30 30 1

//END_MAT_SCRIPT




#define Light0ObjPos    c7
#define Light1ObjPos    c10
#define Light2ObjPos    c11
#define Light3ObjPos    c12
#define Light4ObjPos    c13
#define Light5ObjPos    c14
#define Light6ObjPos    c15

#define Mproj        	c66

#define MyConstants 	c78
#define Zero            c78.x
#define One             c78.y
#define Two		c78.z 
#define Half		c78.w 

#define cameraPositionObjSpace	   c79






#define VertexPosition  v0 
#define VertexColor	v1
#define VertexUV        v2

VertexShader NewVS = asm 
{ 
    vs.1.1
    def MyConstants, 0,1,2,.5
    ; input declaration
    dcl_position0 VertexPosition
    dcl_color0 VertexColor
    dcl_texcoord0 VertexUV

    ;transform position to the projection space 
    m4x4 oPos, VertexPosition, Mproj
    mad oT0.x, VertexUV.x, c30.z, c30.x
    mad oT0.y, VertexUV.y, c30.w, c30.y
    mul oD0, VertexColor, c31
    //mov oD0.a, c31.a  // a fix, because vertex color alpha isn't being set in the effect editor. 
};

technique fixed
{
	pass P0
	{
	  ADDRESSU[0]  = WRAP;
	  ADDRESSV[0]  = WRAP;
	  VertexShader = (NewVS);
	  PixelShader  = 0;

          ColorOp[0]   = Modulate;
          ColorArg1[0] = Texture;
          ColorArg2[0] = Diffuse;
          AlphaOp[0]   = Modulate;
          AlphaArg1[0] = Texture;
          AlphaArg2[0] = Diffuse;

	  ColorOp[1]  = Disable;
	  AlphaOp[1]  = Disable;
	}
};
