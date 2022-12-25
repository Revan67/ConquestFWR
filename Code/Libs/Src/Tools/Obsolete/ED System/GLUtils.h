//---------------------------------------------------------------------------
#ifndef GLUtilsH
#define GLUtilsH
//---------------------------------------------------------------------------
class Transform;
//---------------------------------------------------------------------------
namespace GL
{
//---------------------------------------------------------------------------
class ViewPort
{
	public:
		ViewPort(int x, int y, int w, int h)
		: mX(x), mY(y), mW(w), mH(h)
		{
		}

		int GetX() const { return mX; }
		int GetY() const { return mY; }
		int GetW() const { return mW; }
		int GetH() const { return mH; }
	private:
		int mX;
		int mY;
		int mW;
		int mH;
};
//---------------------------------------------------------------------------
const float unitXVec[3] = { 1.0, 0.0, 0.0 };
const float unitYVec[3] = { 0.0, 1.0, 0.0 };
const float unitZVec[3] = { 0.0, 0.0, 1.0 };
//---------------------------------------------------------------------------
void DrawCoordinateFrame (Transform &modelView, float length, float width);
void Draw3dAxes(float length, float width);
void DrawXZMesh(float xRange, float yRange, float xInterval, float yInterval);
// Returns true on success
bool WindowToWorld(float winX, float winY, float winZ, float& worldX, float& worldY, float& worldZ, 
				   const Transform& modelView, const Transform& projection, const ViewPort& viewPort);
bool WorldToWindow(float worldX, float worldY, float worldZ, float& winX, float& winY, float& winZ, 
				   const Transform& modelView, const Transform& projection, const ViewPort& viewPort);
void GLUPickMatrix(int x, int y, double width, double height);
void SolidCone(float base, float height, int slices, int stacks, const float color[3]);
void WireCube(float side, const float color[3]);
void SolidCube(float side, const float color[3]);
}
//---------------------------------------------------------------------------
#endif
