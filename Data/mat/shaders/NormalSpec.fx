//BEGIN_MAT_SCRIPT
//NAME "NormSpec Test"
//PARAM "Diffuse + Opacity" TEXTURE 0
//PARAM "Normal + Specular" TEXTURE 1
//PARAM "SpecColor" COLORP 48 0 255 0
//PARAM "SpecPower" FLOATP 52 0 60.0 15.0
//PARAM "SpecMetal" FLOATP 56 0 2.0 0.0
//PARAM "SpecMap" FLOATP 60 0 2.0 0.0
//PARAM "Displace Str" FLOATP 64 0 .8 0
//PARAM "Displace Tweak" FLOATP 65 -1 1 -.5
//PARAM "Displace Max" FLOATP 66 0 1 1
//PARAM "Displace Min" FLOATP 67 0 1 0
//PARAM "Normal Str" FLOATP 68 0 3 0

//END_MAT_SCRIPT



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
    mov oT0, VertexUV
    mov oT1, VertexUV2

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

	mov oT5.xyz, r0.xyz
	mad oD0.xyz, r0.xyz, Half, Half


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

	mov oT6.xyz, r0.xyz
	mad oD1.xyz, r0.xyz, Half, Half


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

	mov oT7.xyz, r0.xyz

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




    add r9.xyz,  -cameraPositionObjSpace.xyz, VertexPosition.xyz
   ; Normalize
   dp3 r9.w, r9.xyz, r9.xyz
   rsq r9.w, r9.w
   mul r9.xyz, r9.xyz, r9.w

	// take into local space
	dp3 r0.x, RTANGENT.xyz , -r9.xyz
	dp3 r0.y, RBINORM.xyz , -r9.xyz
	dp3 r0.z, VertexNormal.xyz, -r9.xyz
	mov oT4.xyz, r0.xyz
};

PixelShader LightingPS20 = asm
{
ps_2_0
def c0, 0, 1, -.5, 15

def c10, -.5,-.5,-.5,.5
def c11, 2,2,2,2

def c5, .1,.1,.15,0

def c6,0.09,0,0,0 //-0.03
def c7,.3,.3,0,0

dcl_2d s0
dcl_2d s1
dcl_2d s2
dcl_2d s4

dcl v0
dcl v1

dcl t0.xyzw
dcl t1.xyzw
dcl t3.xyzw
dcl t4.xyzw
dcl t5.xyzw
dcl t6.xyzw
dcl t7.xyzw

nrm r1, t4
texld r2, t0, s0

min r2, r2, c16.b
max r2, r2, c16.a

mov r2, r2.r


//mad r2, r2.r, c6.r, c6.a

mul r0.r, c16.r, c16.g

mad r2, r2.r, c16.r, r0.r

mul r2, r2, c7

mul r2, r2, r1

add r11, r2, t0;

texld r0, r11, s0
texld r3, r11, s1


//texld r0, t0, s0
//texld r3, t0, s1

mov r8, r3.w
add r3, r3, c10
mul r3, r3, c11
mul r3.z, r3.z, c17.r
nrm r6, r3

mov r4, t5
dp3 r5, r4, r6
max r5, r5, c0.x
min r5, r5, c0.y


mov r4, t7
dp3 r4, r4, r6
mul r4, r4, c1

max r4, r4, c0.x
min r4, r4, c0.y

add r5, r5, r4
max r5, r5, c0.x
min r5, r5, c0.y


mov r4, t6
dp3 r7, r4, r6
max r7, r7, c0.x
min r7, r7, c0.y

mov r4, t3
dp3 r2, r4, r6
max r2, r2, c0.x
min r2, r2, c0.y
mul r2, r2, c1

add r7, r2, r7

max r7, r7, c0.x
min r7, r7, c0.y


pow r7, r7.g, c13.r


add r5, r5, c5
add r5, r5, c4
add r5, r5, c2
mul r5, r0, r5
//add r5, r5, r1


mad r5, r7, c12, r5
mul r1, r0, c14.r
mad r5, r7, r1, r5
mul r8, r8, c15.r
mad r5, r7, r8, r5

mov r5.a, r0.a
mul r5, r5, c3

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

	  VertexShader = (NewVS);
	  PixelShader  = (LightingPS20);
	}
};

