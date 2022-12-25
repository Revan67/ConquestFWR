//BEGIN_MAT_SCRIPT
//NAME "Vertex Color With Lighting"

//PARAM "SpecColor" COLORP 48 0 255 0
//PARAM "TeamColor" COLORP 72 0 255 0
//PARAM "DiffColor" COLORP 76 255 255 255

//PARAM "SpecPower" FLOATP 52 0 60.0 15.0
//PARAM "SpecMetal" FLOATP 56 0 2.0 0.0

//PARAM "SpecMap" FLOATP 60 0 2.0 0.0
//PARAM "Displace Str" FLOATP 64 0 .8 0
//PARAM "Displace Tweak" FLOATP 65 -1 1 -.5
//PARAM "Displace Max" FLOATP 66 0 1 1
//PARAM "Displace Min" FLOATP 67 0 1 0
//PARAM "Normal Str" FLOATP 68 0 3 0

//PARAM "Diffuse + Opacity" TEXTURE 0
//PARAM "Normal + Height" TEXTURE 1
//PARAM "Emissive + Specularity" TEXTURE 2
//PARAM "Team Color" TEXTURE 3


//ADVANCED "SrcBlend" STATE D3DRS_SRCBLEND D3DBLEND_SRCALPHA
//ADVANCED "DstBlend" STATE D3DRS_DESTBLEND D3DBLEND_INVSRCALPHA
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

//PARAM "TexTransX" FLOATV 120 -3  3  0
//PARAM "TexTransY" FLOATV 121 -3  3  0
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
#define VertexNormal	v1
#define VertexUV        v2 
#define VertexUV2       v3 
#define VertexBinorm	v4
#define VertexTangent	v5
#define VertexExtra	v6
#define VertexColor	v7

#define RTANGENT r8
#define RBINORM r3

// t0 = texture coords
// t1 = tangentspace view (for displacement) + vr
// t2 = light0 dir			     + vg	
// t3 = light0 specdir			     + vb
// t4 = light1 dir			     + va
// t5 = light1 specdir
// t6 = light2 dir
// t7 = light2 specdir
// d0 = light3 dir
// d1 = light3 specdir




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
    dcl_color0 VertexColor

    ;transform position to the projection space 
    m4x4 oPos, VertexPosition, Mproj
    mad oT0.x, VertexUV.x, c30.z, c30.x
    mad oT0.y, VertexUV.y, c30.w, c30.y

    mov RTANGENT.x, VertexTangent.x
    mov RTANGENT.y, VertexTangent.y
    mov RTANGENT.z, VertexExtra.x

    mov RBINORM.x, VertexBinorm.x
    mov RBINORM.y, VertexBinorm.y
    mov RBINORM.z, VertexExtra.y


    add r9.xyz,  -Light0ObjPos.xyz, VertexPosition.xyz
   ; Normalize
   dp3 r9.w, r9.xyz, r9.xyz
   rsq r9.w, r9.w
   mul r9.xyz, r9.xyz, r9.w

	// take into local space
	dp3 r0.x, RTANGENT.xyz , -r9.xyz
	dp3 r0.y, RBINORM.xyz , -r9.xyz
	dp3 r0.z, VertexNormal.xyz, -r9.xyz

	mov oT2.xyz, r0.xyz

    add r7.xyz,  -cameraPositionObjSpace.xyz, VertexPosition.xyz


   ; Normalize
   dp3 r7.w, r7.xyz, r7.xyz
   rsq r7.w, r7.w
   mul r7.xyz, r7.xyz, r7.w


   add r9.xyz, r9.xyz, r7.xyz
   mul r9.xyz, r9.xyz, Half
   
	// take into local space
	dp3 r0.x, RTANGENT.xyz , -r9.xyz
	dp3 r0.y, RBINORM.xyz , -r9.xyz
	dp3 r0.z, VertexNormal.xyz, -r9.xyz

	mov oT3.xyz, r0.xyz

///


    add r9.xyz,  -Light1ObjPos.xyz, VertexPosition.xyz
   ; Normalize
   dp3 r9.w, r9.xyz, r9.xyz
   rsq r9.w, r9.w
   mul r9.xyz, r9.xyz, r9.w

	// take into local space
	dp3 r0.x, RTANGENT.xyz , -r9.xyz
	dp3 r0.y, RBINORM.xyz , -r9.xyz
	dp3 r0.z, VertexNormal.xyz, -r9.xyz
	mov oT4.xyz, r0.xyz

    add r7.xyz,  -cameraPositionObjSpace.xyz, VertexPosition.xyz


   ; Normalize
   dp3 r7.w, r7.xyz, r7.xyz
   rsq r7.w, r7.w
   mul r7.xyz, r7.xyz, r7.w


   add r9.xyz, r9.xyz, r7.xyz
   mul r9.xyz, r9.xyz, Half
   
	// take into local space
	dp3 r0.x, RTANGENT.xyz , -r9.xyz
	dp3 r0.y, RBINORM.xyz , -r9.xyz
	dp3 r0.z, VertexNormal.xyz, -r9.xyz

	mov oT5.xyz, r0.xyz



    add r9.xyz,  -Light2ObjPos.xyz, VertexPosition.xyz
   ; Normalize
   dp3 r9.w, r9.xyz, r9.xyz
   rsq r9.w, r9.w
   mul r9.xyz, r9.xyz, r9.w

	// take into local space
	dp3 r0.x, RTANGENT.xyz , -r9.xyz
	dp3 r0.y, RBINORM.xyz , -r9.xyz
	dp3 r0.z, VertexNormal.xyz, -r9.xyz
	mov oT6.xyz, r0.xyz

    add r7.xyz,  -cameraPositionObjSpace.xyz, VertexPosition.xyz


   ; Normalize
   dp3 r7.w, r7.xyz, r7.xyz
   rsq r7.w, r7.w
   mul r7.xyz, r7.xyz, r7.w


   add r9.xyz, r9.xyz, r7.xyz
   mul r9.xyz, r9.xyz, Half
   
	// take into local space
	dp3 r0.x, RTANGENT.xyz , -r9.xyz
	dp3 r0.y, RBINORM.xyz , -r9.xyz
	dp3 r0.z, VertexNormal.xyz, -r9.xyz

	mov oT7.xyz, r0.xyz




    add r9.xyz,  -Light3ObjPos.xyz, VertexPosition.xyz
   ; Normalize
   dp3 r9.w, r9.xyz, r9.xyz
   rsq r9.w, r9.w
   mul r9.xyz, r9.xyz, r9.w

	// take into local space
	dp3 r0.x, RTANGENT.xyz , -r9.xyz
	dp3 r0.y, RBINORM.xyz , -r9.xyz
	dp3 r0.z, VertexNormal.xyz, -r9.xyz
	mad oD0.xyz, r0.xyz, Half, Half
	//mov oD0.xyz, r0.xyz

    add r7.xyz,  -cameraPositionObjSpace.xyz, VertexPosition.xyz


   ; Normalize
   dp3 r7.w, r7.xyz, r7.xyz
   rsq r7.w, r7.w
   mul r7.xyz, r7.xyz, r7.w


   add r9.xyz, r9.xyz, r7.xyz
   mul r9.xyz, r9.xyz, Half
   
	// take into local space
	dp3 r0.x, RTANGENT.xyz , -r9.xyz
	dp3 r0.y, RBINORM.xyz , -r9.xyz
	dp3 r0.z, VertexNormal.xyz, -r9.xyz

	mad oD1.xyz, r0.xyz, Half, Half
	//mov oD1.xyz, r0.xyz




    add r9.xyz,  -cameraPositionObjSpace.xyz, VertexPosition.xyz
   ; Normalize
   dp3 r9.w, r9.xyz, r9.xyz
   rsq r9.w, r9.w
   mul r9.xyz, r9.xyz, r9.w

	// take into local space
	dp3 r0.x, RTANGENT.xyz , -r9.xyz
	dp3 r0.y, RBINORM.xyz , -r9.xyz
	dp3 r0.z, VertexNormal.xyz, -r9.xyz
	mov oT1.xyz, r0.xyz

    mov oT1.w, VertexColor.r
    mov oT2.w, VertexColor.g
    mov oT3.w, VertexColor.b
    mov oT4.w, VertexColor.a
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

nrm r1, t1
texld r2, t0, s1

min r2.a, r2.a, c16.b
max r2.a, r2.a, c16.a

mov r2, r2.a

mul r0.r, c16.r, c16.g
mad r2, r2.r, c16.r, r0.r

mad r11, r2, r1, t0

texld r0, r11, s0
mul r0.r, r0.r, c19
//mov r0.a, r3.a

texld r3, r11, s1
texld r4, r11, s3
mov r2.a, r4.r
texld r4, r11, s2
mov r4.a, r2.a

//texld r0, t0, s0
//texld r3, t0, s1

mov r8, r3.w
mad r3, r3, c9.x, c9.y
mul r3.z, r3.z, c17.r
nrm r6, r3

// light0
dp3 r5.r, t2, r6
max r5, r5.r, c0.x

// light1
dp3 r3, t4, r6
max r3, r3, c0.x
mad r5, r3, c1, r5

// light2
dp3 r3, t6, r6
max r3, r3, c0.x
mad r5, r3, c2, r5

// light3
mad r3, v0, c9.x, c9.y
dp3 r3, r3, r6
max r3, r3, c0.x
mad r5, r3, c3, r5


// light0 spec
dp3 r7, t3, r6
max r7, r7, c0.x
pow r7, r7.g, c13.r

// light1 spec
dp3 r2, t5, r6
max r2, r2, c0.x
pow r2, r2.g, c13.r
mad r7, r2, c1, r7

// light2 spec
dp3 r2, t7, r6
max r2, r2, c0.x
pow r2, r2.g, c13.r
mad r7, r2, c2, r7

 // light3 spec
// mad r2, v1, c9.x, c9.y
// dp3 r2, r2, r6
// max r2, r2, c0.x
// pow r2, r2.g, c13.r
// mad r7, r2, c3, r7


 mul r5, r0, r5
 add r5, r4, r5
 mad r5, r4.a, c18,r5

 mul r7, r7, c12
 mul r1, r0, c14.r
 mad r5, r7, r1, r5
 mul r8, r8, c15.r
 mad r5, r7, r8, r5

mul r5.r, r5.r, t1.a
mul r5.g, r5.g, t2.a
mul r5.b, r5.b, t3.a
mul r5.a, r0.a, t4.a

 mov oC0, r5
};


technique PS20
{
	pass P0
	{
	  ADDRESSU[0] = WRAP;
	  ADDRESSV[0] = WRAP;
	  ADDRESSU[1] = WRAP;
	  ADDRESSV[1] = WRAP;
	  ADDRESSU[2] = WRAP;
	  ADDRESSV[2] = WRAP;
	  ADDRESSU[3] = WRAP;
	  ADDRESSV[3] = WRAP;

	  VertexShader = (NewVS);
	  PixelShader  = (LightingPS20);
	}
};

