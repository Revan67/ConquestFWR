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
    mov oT2, VertexUV2

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
	dp3 r0.x, RTANGENT.xyz ,-r9.xyz
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
	dp3 r0.x, RTANGENT.xyz ,-r9.xyz
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
	dp3 r0.x, RTANGENT.xyz ,-r9.xyz
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
	dp3 r0.x, RTANGENT.xyz ,-r9.xyz
	dp3 r0.y, RBINORM.xyz , -r9.xyz
	dp3 r0.z, VertexNormal.xyz, -r9.xyz

	mov oT3.xyz, r0.xyz




    add r9.xyz,  -cameraPositionObjSpace.xyz, VertexPosition.xyz
   ; Normalize
   dp3 r9.w, r9.xyz, r9.xyz
   rsq r9.w, r9.w
   mul r9.xyz, r9.xyz, r9.w

	// take into local space
	dp3 r0.x, RTANGENT.xyz ,-r9.xyz
	dp3 r0.y, RBINORM.xyz , -r9.xyz
	dp3 r0.z, VertexNormal.xyz, -r9.xyz
	mov oT4.xyz, r0.xyz
};



PixelShader LightingPS20 = asm
{
ps_2_0
def c0, 0, 1, 1.0, 15
def c10, -.5,-.5,-.5,0
def c11, 2,2,2,2

def c6,0.03,0,0,0 //-0.03
def c7,.3,.3,0,0

dcl_2d s0
dcl_2d s1
dcl_2d s2
dcl_2d s4

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

//nrm r1, t4
//texld r2, t0, s0
//mov r2, r2.r
//mad r2, r2.r, c6.r, c6.a
//mul r2, r2, c7
//mad r11, r2, r1, t0;

//texld r0, r11, s0
//texld r1, t2, s2
//texld r3, r11, s1
texld r0, t0, s0
texld r1, t2, s2
texld r3, t0, s1
mov r8, r3.w
add r3, r3, c10
mul r3, r3, c11
nrm r6, r3

mov r4, t5
dp3 r5, r4, r6

max r5, r5, c0.x
min r5, r5, c0.y

add r3, r0, r0
mul r3, r3, r5

sub r3, c0.z, r3

mul r1, r1, r3


mov r4, t7
dp3 r4, r4, r6
mul r4, r4, c1

add r5, r5, r4
max r5, r5, c0.x


mov r4, t6
dp3 r7, r4, r6
//max r7, r7, c0.x

mov r4, t3
dp3 r2, r4, r6
mul r2, r2, c1

add r7, r2, r7

//max r7, r7, c0.x
pow r7, r7.g, c0.a

add r5, r5, c4
add r5, r5, c2
mul r5, r5, r0

add r5, r5, r1

mul r7, r7, r8
//mul r7, r7, r0
add r5, r5, r7
mov r5.a, r0.a
mul r5, r5, c3

mov oC0, r5


};




PixelShader LightingPS11 = asm
{
	ps.1.1
	def c7, 0, 0, 1, 0
	def c5, .1,.1,.15,0
	def c6, -.5,-.5,0,0
	tex t0
	tex t1
	tex t2
	dp3 r0, t1_bx2, v0_bx2
	dp3 r1, t1_bx2, v1_bx2
	mul r1, r1, r1
	mad r1, r1, r1, c2
	mul r1, r1, t0
	mad r0, r0, t0, r1
	mad r0, r0, c3, t2
	mul r0.a, t0.a, c3.a
};

technique PS20
{
	pass P0
	{
	  VertexShader = (NewVS);
	  PixelShader  = (LightingPS20);
	}
};

technique PS11
{
	pass P0
	{
	  VertexShader = (NewVS);
	  PixelShader  = (LightingPS11);
	}
};

technique PS00
{
	pass P0
	{
	  VertexShader = (NewVS);
          PixelShader  = NULL;
	  ColorOp[0]   = DOTPRODUCT3;
          ColorArg1[0] = TEXTURE;
          ColorArg2[0] = DIFFUSE;
	  ColorOp[1]   = MODULATE;
          ColorArg1[1] = TEXTURE;
          ColorArg2[1] = CURRENT;
	  AlphaOp[1]   = SELECTARG1;
          AlphaArg1[1] = TEXTURE;
          AlphaArg2[1] = CURRENT;
	  ColorOp[2]   = DISABLE;
	}
	pass P1
	{
	  VertexShader = (NewVS);
          PixelShader  = NULL;

	  ColorOp[0]   = DOTPRODUCT3;
          ColorArg1[0] = TEXTURE;
	  ColorArg2[0] = SPECULAR;

	  ColorOp[1]   = SELECTARG1;
          ColorArg1[1] = TEXTURE;
	  ResultArg[1] = TEMP;

	  ColorOp[2] = MODULATE;
          ColorArg1[2] = CURRENT;
	  ColorArg2[2] = CURRENT;

	  ColorOp[3] = MODULATE;
          ColorArg1[3] = CURRENT;
	  ColorArg2[3] = CURRENT;

	  ColorOp[4] = MODULATE;
          ColorArg1[4] = CURRENT;
	  ColorArg2[4] = CURRENT;

	  ColorOp[5] = MODULATE;
          ColorArg1[5] = CURRENT;
	  ColorArg2[5] = TFACTOR;

	  ColorOp[6] = ADD;
          ColorArg1[6] = TEMP;
	  ColorArg2[6] = CURRENT;

	}
};




