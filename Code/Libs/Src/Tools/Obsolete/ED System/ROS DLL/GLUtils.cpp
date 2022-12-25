//---------------------------------------------------------------------------
#include "PCH.h"
#include "DARenderPipeline.h"
#include "glu.h"
#include "GLUtils.h"
#include "CodeMsg.h"
#include "TransformUtil.h"
#include "RPUL.h"
//---------------------------------------------------------------------------
namespace GL
{
void DrawCoordinateFrame (Transform &modelView, float length, float width)
{
	ASSERT(PIPE);
    Transform	oldModelView;

	PIPE->get_modelview(oldModelView);
	PIPE->set_modelview(modelView);
	PIPE->set_render_state (D3DRS_TEXTUREHANDLE, 0);
	Draw3dAxes (length, width);
	PIPE->set_modelview(oldModelView);
}

void Draw3dAxes(float length, float width)
{
	ASSERT(PIPE);

	PIPE->set_render_state (D3DRS_TEXTUREHANDLE, 0);

	PrimitiveBuilder pb(PIPE);

	const float	halfLen = length/2;
    const float xColor[3] = { 1.0, 0.0, 0.0 };
    const float yColor[3] = { 0.0, 1.0, 0.0 };
    const float zColor[3] = { 0.0, 0.0, 1.0 };

    pb.Begin(PB_LINES);
    	// x-axis
        pb.Color3f(xColor[0], xColor[1], xColor[2]);
        pb.Vertex3f(-halfLen, 0.0, 0.0);
        pb.Vertex3f(halfLen, 0.0, 0.0);

        // y-axis
        pb.Color3f(yColor[0], yColor[1], yColor[2]);
        pb.Vertex3f(0.0, -halfLen, 0.0);
        pb.Vertex3f(0.0, halfLen, 0.0);

        // z-axis
        pb.Color3f(zColor[0], zColor[1], zColor[2]);
        pb.Vertex3f(0.0, 0.0, -halfLen);
        pb.Vertex3f(0.0, 0.0, halfLen);
    pb.End();

	// draw the arrow-heads
	Matrix		rot;
	Transform	trans;
	const float coneHeight = 0.05 * length;
    const float coneBase = coneHeight/4;
    const float coneSlices = 4;
    const float coneStacks = 1;

    Transform	oldModelView, currModelView;

	PIPE->get_modelview(oldModelView);

    // x-axis arrow-head
	rot.compose_rotation(Y_AXIS, 90);
	trans = Transform(rot, Vector(halfLen, 0.0, 0.0));

	currModelView = oldModelView * trans;

	PIPE->set_modelview(currModelView);

    GL::SolidCone(coneBase, coneHeight, coneSlices, coneStacks, xColor);

    // y-axis arrow-head
	rot.compose_rotation(X_AXIS, -90);
	trans = Transform(rot, Vector(0.0, halfLen, 0.0));

	currModelView = oldModelView * trans;

	PIPE->set_modelview(currModelView);

    GL::SolidCone(coneBase, coneHeight, coneSlices, coneStacks, yColor);

    // z-axis arrow-head
	rot.set_identity();
	trans = Transform(rot, Vector(0.0, 0.0, halfLen));

	currModelView = oldModelView * trans;

	PIPE->set_modelview(currModelView);

    GL::SolidCone(coneBase, coneHeight, coneSlices, coneStacks, zColor);

	// restore the model-view
	PIPE->set_modelview(oldModelView);
}
//---------------------------------------------------------------------------
void DrawXZMesh(float xRange, float zRange, float xInterval, float zInterval)
{
	ASSERT(PIPE);
	
	PIPE->set_render_state(D3DRS_TEXTUREHANDLE, 0);

	PrimitiveBuilder pb(PIPE);

	float halfXRange = xRange/2;
	float halfZRange = zRange/2;

    pb.Color3f(0.75, 0.75, 0.75);

    pb.Begin(PB_LINES);

	// Draw lines parallel to x-axis
    float z;
	for(z = -halfZRange; z < halfZRange; z += zInterval)
    {	pb.Vertex3f(-halfXRange, 0.0, z);
    	pb.Vertex3f(halfXRange, 0.0, z);
    }
    // Draw last line
    pb.Vertex3f(-halfXRange, 0.0, z);
    pb.Vertex3f(halfXRange, 0.0, z);

	// Draw lines parallel to z-axis
    float x;
	for(x = -halfXRange; x < halfXRange; x += xInterval)
    {	pb.Vertex3f(x, 0.0, -halfZRange);
    	pb.Vertex3f(x, 0.0, halfZRange);
    }
    // Draw last line
    pb.Vertex3f(x, 0.0, -halfZRange);
    pb.Vertex3f(x, 0.0, halfZRange);

    pb.End();
}
#if 0
//---------------------------------------------------------------------------
bool WindowToWorld(float winX, float winY, float winZ, float& worldX, float& worldY, float& worldZ, 
				   const Transform& modelView, const Transform& projection, const ViewPort& viewPort)
{
    GLdouble	modelMatrix[4][4];					// Storage for modelview matrix
    GLdouble	projMatrix[4][4];					// Storage for projection matrix
    GLint		viewport[4] = {	viewPort.GetX(),	// Storage for viewport coordinates
								viewPort.GetY(), 
								viewPort.GetW(), 
								viewPort.GetH()};

	GetGLMatrix(modelView, modelMatrix);
	GetGLMatrix(projection, projMatrix);

    // Account for difference between Windows (GDI) coordinates and OpenGL coordinates
    winY = viewport[3] - winY;

    // Get the coordinates
    double wX, wY, wZ;

    try
    {   if(GL_TRUE == gluUnProject(winX, winY, winZ, (double*)modelMatrix, (double*)projMatrix, viewport, &wX, &wY, &wZ))
        {   worldX = wX;
            worldY = wY;
            worldZ = wZ;
            return true;
        }
        else
        {   return false;
        }
    }
    catch(...)
    {	return false;
    }
}
//---------------------------------------------------------------------------
bool WorldToWindow(float worldX, float worldY, float worldZ, float& winX, float& winY, float& winZ, 
				   const Transform& modelView, const Transform& projection, const ViewPort& viewPort)
{
    GLdouble	modelMatrix[4][4];					// Storage for modelview matrix
    GLdouble	projMatrix[4][4];					// Storage for projection matrix
    GLint		viewport[4] = {	viewPort.GetX(),	// Storage for viewport coordinates
								viewPort.GetY(), 
								viewPort.GetW(), 
								viewPort.GetH()};

	GetGLMatrix(modelView, modelMatrix);
	GetGLMatrix(projection, projMatrix);

    // Get the coordinates
    double wX, wY, wZ;

    try
    {   if(GL_TRUE == gluProject(worldX, worldY, worldZ, (double*)modelMatrix, (double*)projMatrix, viewport, &wX, &wY, &wZ))
        {   winX = wX;
            winY = wY;
            winZ = wZ;

            // Account for difference between Windows (GDI) coordinates and OpenGL coordinates
            winY = viewport[3] - winY;

            return true;
        }
        else
        {   return false;
        }
    }
    catch(...)
    {	return false;
    }
}
//---------------------------------------------------------------------------
void GLUPickMatrix(int x, int y, double width, double height)
{
    GLint viewport[4];			// Storage for viewport coordinates

    // Get the viewport transforms
    glGetIntegerv(GL_VIEWPORT, viewport);

    GLdouble    winY = viewport[3] - y;

    gluPickMatrix (x, winY, width, height, viewport);
}
#endif
//---------------------------------------------------------------------------
void SolidCone(float base, float height, GLint slices, GLint stacks, const float color[3])
{
	ASSERT(PIPE);

	PIPE->set_render_state (D3DRS_TEXTUREHANDLE, 0);

	PrimitiveBuilder pb(PIPE);

    pb.Color3f(color[0], color[1], color[2]);

	GLfloat baseVert[4][3] = {	{-base, -base, 0},                     
    							{-base, +base, 0},                     
                                {+base, +base, 0},                     
                                {+base, -base, 0}};                    

	pb.Begin(PB_QUADS);
		for(int v1 = 0; v1 < 4; ++v1)
        {	pb.Vertex3f(baseVert[v1][0], baseVert[v1][1], baseVert[v1][2]);
        }
    pb.End();

    pb.Begin(PB_TRIANGLE_FAN);
    	pb.Vertex3f(0, 0, height);
		for(int v2 = 0; v2 < 4; ++v2)
        {	pb.Vertex3f(baseVert[v2][0], baseVert[v2][1], baseVert[v2][2]);
        }
    pb.End();
}
//---------------------------------------------------------------------------
void WireCube(float side, const float color[3])
{
	ASSERT(PIPE);

	PIPE->set_render_state (D3DRS_TEXTUREHANDLE, 0);

	PrimitiveBuilder pb(PIPE);

	pb.Color3f(color[0], color[1], color[2]);

	GLfloat halfSide = side / 2;
	GLfloat baseVert[4][3] = {	{-halfSide, -halfSide, -halfSide},                     
                                {+halfSide, -halfSide, -halfSide},                    
                                {+halfSide, -halfSide, +halfSide},                     
  								{-halfSide, -halfSide, +halfSide}};                     
 
	GLfloat topVert[4][3] = {	{-halfSide, +halfSide, -halfSide},                     
    							{+halfSide, +halfSide, -halfSide},                     
                                {+halfSide, +halfSide, +halfSide},                     
                                {-halfSide, +halfSide, +halfSide}};                    

	// Bottom
	pb.Begin(PB_LINE_LOOP);
		for(int v1 = 0; v1 < 4; ++v1)
		{	pb.Vertex3f(baseVert[v1][0], baseVert[v1][1], baseVert[v1][2]);
		}
	pb.End();
	// Top
	pb.Begin(PB_LINE_LOOP);
		for(int v2 = 0; v2 < 4; ++v2)
		{	pb.Vertex3f(topVert[v2][0], topVert[v2][1], topVert[v2][2]);
		}
	pb.End();
	// Vertical edges
	pb.Begin(PB_LINES);
		for(int v3 = 0; v3 < 4; ++v3)
		{	pb.Vertex3f(baseVert[v3][0], baseVert[v3][1], baseVert[v3][2]);
			pb.Vertex3f(topVert[v3][0], topVert[v3][1], topVert[v3][2]);
		}
	pb.End();
}
//---------------------------------------------------------------------------
void SolidCube(float side, const float color[3])
{
	ASSERT(PIPE);

	PIPE->set_render_state (D3DRS_TEXTUREHANDLE, 0);
	PrimitiveBuilder pb(PIPE);

	pb.Color3f(color[0], color[1], color[2]);

	GLfloat halfSide = side / 2;
	GLfloat baseVert[4][3] = {	{-halfSide, -halfSide, -halfSide},                     
    							{-halfSide, -halfSide, +halfSide},                     
                                {+halfSide, -halfSide, +halfSide},                     
                                {+halfSide, -halfSide, -halfSide}};                    

	GLfloat topVert[4][3] = {	{-halfSide, +halfSide, -halfSide},                     
    							{-halfSide, +halfSide, +halfSide},                     
                                {+halfSide, +halfSide, +halfSide},                     
                                {+halfSide, +halfSide, -halfSide}};                    

	pb.Begin(PB_QUADS);
		// x,z,-y plane
		pb.Vertex3f(baseVert[3][0], baseVert[3][1], baseVert[3][2]);
		pb.Vertex3f(baseVert[2][0], baseVert[2][1], baseVert[2][2]);
		pb.Vertex3f(baseVert[1][0], baseVert[1][1], baseVert[1][2]);
		pb.Vertex3f(baseVert[0][0], baseVert[0][1], baseVert[0][2]);
		// x,z,+y plane
		pb.Vertex3f(baseVert[0][0], baseVert[0][1], baseVert[0][2]);
		pb.Vertex3f(baseVert[1][0], baseVert[1][1], baseVert[1][2]);
		pb.Vertex3f(baseVert[2][0], baseVert[2][1], baseVert[2][2]);
		pb.Vertex3f(baseVert[3][0], baseVert[3][1], baseVert[3][2]);

		// x,y,-z plane
		pb.Vertex3f(baseVert[0][0], baseVert[0][1], baseVert[0][2]);
		pb.Vertex3f(baseVert[0][0], baseVert[0][1], baseVert[0][2]);
		pb.Vertex3f(baseVert[3][0], baseVert[3][1], baseVert[3][2]);
		pb.Vertex3f(baseVert[3][0], baseVert[3][1], baseVert[3][2]);
		// x,y,+z plane
		pb.Vertex3f(baseVert[1][0], baseVert[1][1], baseVert[1][2]);
		pb.Vertex3f(baseVert[2][0], baseVert[2][1], baseVert[2][2]);
		pb.Vertex3f(baseVert[2][0], baseVert[2][1], baseVert[2][2]);
		pb.Vertex3f(baseVert[1][0], baseVert[1][1], baseVert[1][2]);
	
		// y,z,-x plane
		pb.Vertex3f(baseVert[1][0], baseVert[1][1], baseVert[1][2]);
		pb.Vertex3f(baseVert[1][0], baseVert[1][1], baseVert[1][2]);
		pb.Vertex3f(baseVert[0][0], baseVert[0][1], baseVert[0][2]);
		pb.Vertex3f(baseVert[0][0], baseVert[0][1], baseVert[0][2]);
		// y,z,+x plane
		pb.Vertex3f(baseVert[3][0], baseVert[3][1], baseVert[3][2]);
		pb.Vertex3f(baseVert[3][0], baseVert[3][1], baseVert[3][2]);
		pb.Vertex3f(baseVert[2][0], baseVert[2][1], baseVert[2][2]);
		pb.Vertex3f(baseVert[2][0], baseVert[2][1], baseVert[2][2]);
	pb.End();
}
//---------------------------------------------------------------------------
}