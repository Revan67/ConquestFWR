//BEGIN_MAT_SCRIPT
//NAME "Flat Color"

//PARAM "Color" COLORP 76 255 255 255

//PARAM "SpecPower" FLOATP 52 0 60.0 15.0
//PARAM "SpecWhite" FLOATP 53 0 1.0 1
//PARAM "SpecMetal" FLOATP 54 0 1.0 0


//ADVANCED "SrcBlend" STATE D3DRS_SRCBLEND D3DBLEND_ONE
//ADVANCED "DstBlend" STATE D3DRS_DESTBLEND D3DBLEND_ZERO
//ADVANCED "ZWrite" STATE D3DRS_ZWRITEENABLE true
//ADVANCED "ZTest" STATE D3DRS_ZENABLE true
//BEGIN_ENUM "enum"
//ENUM_VALUE "Opaque"
//ENUM_SET "DstBlend" D3DBLEND_ZERO
//ENUM_SET "SrcBlend" D3DBLEND_ONE
//ENUM_VALUE "Transparent"
//ENUM_SET "DstBlend" D3DBLEND_INVSRCALPHA
//ENUM_SET "SrcBlend" D3DBLEND_SRCALPHA
//ENUM_VALUE "Additive"
//ENUM_SET "SrcBlend" D3DBLEND_ONE
//ENUM_SET "DstBlend" D3DBLEND_ONE
//ENUM_VALUE "SQUARED"
//ENUM_SET "DstBlend" D3DBLEND_ZERO
//ENUM_SET "SrcBlend" D3DBLEND_SRCCOLOR
//ENUM_END


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
#define VertexNormal	v1
#define VertexUV        v2 
#define VertexUV2       v3 
#define VertexBinorm	v4
#define VertexTangent	v5
#define VertexExtra	v6

#define RTANGENT r8
#define RBINORM r3


// t0 = light0 contrib

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

    add r9.xyz,  Light0ObjPos.xyz, -VertexPosition.xyz
   ; Normalize
   dp3 r9.w, r9.xyz, r9.xyz
   rsq r9.w, r9.w
   mul r9.xyz, r9.xyz, r9.w

   dp3 r0.xyz, r9.xyz, VertexNormal.xyz
   mov oT0.xyz, r0.xyz


   add r7.xyz,  cameraPositionObjSpace.xyz, -VertexPosition.xyz
   ; Normalize
   dp3 r7.w, r7.xyz, r7.xyz
   rsq r7.w, r7.w
   mul r7.xyz, r7.xyz, r7.w

   add r9.xyz, r9.xyz, r7.xyz
   mul r9.xyz, r9.xyz, Half

   dp3 r9.w, r9.xyz, r9.xyz
   rsq r9.w, r9.w
   mul r9.xyz, r9.xyz, r9.w
  
   dp3 r0.xyz, r9.xyz,VertexNormal.xyz
   mov oT1.xyz, r0.xyz

};

//co = useful constants
// c1 = light1 color


PixelShader LightingPS20 = asm
{
ps_2_0
def c0, 0, 1, -.5, 15

def c9, 2,-1,0,0

def c5, .1,.1,.15,0

def c6,0.09,0,0,0 //-0.03

dcl_2d s0
dcl_2d s1
dcl_2d s2
dcl_2d s3

dcl v0
dcl v1

dcl t0.xyzw
dcl t1.xyzw
dcl t2.xyzw
dcl t3.xyzw
dcl t4.xyzw
dcl t5.xyzw
dcl t6.xyzw
dcl t7.xyzw

mov r5, t0
mov r7, t1
pow r7, r7.g, c13.r

mul r6, r7, c13.g

mul r7, r7, c19

mad r6, r7, c13.b, r6

mad r6, r5, c19, r6
mov r6.a, c0.y
mov oC0, r6
};

technique PS20
{
	pass P0
	{
	  ADDRESSU[0] = WRAP;
	  ADDRESSV[0] = WRAP;
	  ADDRESSU[1] = WRAP;
	  ADDRESSV[1] = WRAP;

	  VertexShader = (NewVS);
	  PixelShader  = (LightingPS20);
	}
};



