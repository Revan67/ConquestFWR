//---------------------------------------------------------------------------
#ifndef GLSceneViewUIH
#define GLSceneViewUIH
//---------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <afxwin.h>
#include <Exception>

#include "xform.h"
//---------------------------------------------------------------------------
namespace ROS
{
class ADynamicCamera;
class DABaseCamera;
class ASceneEntity;
class AStaticSceneEntity;
class CompoundSceneEntity;
class DeformableSceneEntity;
class ConstMotionStateAccessor;
class PositionMarker;
}

class SceneView;
class SceneViewSoundListener;
class ASoundListener;
struct RPBITMAPINFO;
//---------------------------------------------------------------------------
class TGLSceneViewUIForm
{
	friend class SceneViewSoundListener;

    public:		// User declarations
        class ExGLCreateContextFailed: public std::exception
        {
        	public:
                virtual const char* what() const throw()
                {
                  return "Failed to create GL Context";
                }
        };

        class ExCameraCreationFailed: public std::exception
        {	
			public:
        		ExCameraCreationFailed()
				{
                };

                virtual const char* what() const throw()
                {
                  return "Failed to create camera";
                }
        };

		class ExRenderingBufferCreationFailed: public std::exception
		{
			public:
        		ExRenderingBufferCreationFailed()
				{
                };

                virtual const char* what() const throw()
                {
					return "Failed to create rendering buffer. Reduce desktop resolution and try again.";
                }
		};

        TGLSceneViewUIForm(SceneView& associatedSceneView, int top, int left, int height, int width, HWND parentWndH);

        void AssociateCamera(ROS::ADynamicCamera* camera);
		const ROS::ADynamicCamera* GetAssociatedCamera() const;
        void AssociatedCameraUpdated();

		~TGLSceneViewUIForm();

        void Repaint();

		// Static methods
		static void InitPipeline(HWND hMainWnd);

	private:	// User declarations
    	enum Mode
        {
			neutralMode,
			rollViewMode,
        	rotateViewWithoutRollMode,
            translateViewMode,
            dollyViewMode,
            translateEntityMode,
			rollEntityMode,
            rotateEntityWithoutRollMode,
			rotateViewAboutEntityMode,
			viewRectSelectMode
        };

		enum TMouseButton 
		{	
			mbLeft, 
			mbRight, 
			mbMiddle 
		};

		enum TInputState 
		{
			ssShift = 1,
			ssAlt = 2,
			ssCtrl = 4,
			ssLeft = 8,
			ssMiddle = 16,
			ssRight = 32,
			ssDouble = 64
		};

		enum CommandType
		{	
			kCamera,
			kCompound,
			kDeformable
		};

		typedef unsigned int TShiftState;

        void __fastcall SelectionXYPlaneClick();
        void __fastcall SelectionYZPlaneClick();
        void __fastcall SelectionZXPlaneClick();
        void __fastcall XYPlaneClick();
        void __fastcall YZPlaneClick();
        void __fastcall ZXPlaneClick();
		void __fastcall MeshClick();
		void __fastcall AxesClick();
		void __fastcall RenderTexturedClick();
        void __fastcall AssociateCameraClick(unsigned int cameraIdx);
		void __fastcall SelectCompoundPopupClick(unsigned int hardpointIdx);
        void __fastcall StartClick();
        void __fastcall LoopClick();
        void __fastcall PauseClick();
        void __fastcall ResumeClick();
        void __fastcall StopClick();
        void __fastcall SetCameraClick();
        void __fastcall HardPointsClick();
		void __fastcall SkeletonClick();
		void __fastcall MotionPathClick();
        void __fastcall PlaySoundClick();
        void __fastcall SetToRememberedPositionClick();
		void __fastcall SetIKTarget();
		void __fastcall ShowLetterboxClick();

        void CreateAppWindow (HINSTANCE hAppInstance, const char* title, HWND parentWndH);
		void SetStyleInWindow(unsigned int left, unsigned int top, unsigned int width, unsigned int height);
        void StoreForm();
  
        static long WINAPI WndProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
		static void ProcessCommand(int notifyCode, int iD, HWND ctrlWndH, TGLSceneViewUIForm* sceneView);

		void PaintScene();
        void Render(const ROS::DABaseCamera* camera) const;
		void SetGLRenderVolume() const;
		void DisplayStartup();
        void DrawScene(const ROS::DABaseCamera* camera) const;

		void OpenGLMouseDown(TMouseButton Button, TShiftState Shift, int X, int Y);
		void OpenGLMouseMove(TShiftState Shift, int X, int Y);
		void OpenGLMouseUp(TMouseButton Button, TShiftState Shift, int X, int Y);
		void OpenGLKeyDown(unsigned int virtualKey);
		void OpenGLKeyUp(unsigned int virtualKey);
		void OverlayPaint (HDC paintDC);
		bool OnTimer (UINT wTimerId, TIMERPROC *proc);

#ifdef PORTED
        void OpenGLMouseLeave();
        void OpenGLUpdateHint(UINT message);
#endif

		ROS::ASceneEntity* GetEntity(int winX, int winY) const;
		
		// This gets nearest and furthest distance from the camera to the objects
		// contained by the camera sub-frustum that results from the given screen
		// rectangle. Returns false if no objects project into the given rectangle.
		// Just the position of the object is checked.
		bool GetDepthRangeOfObjects (RECT &screenRect, float &nearest, float &furthest);

		void CreateOpenGLPopupMenu();
		void CreateSceneEntityPopupMenu();
		void CreateCompoundSceneEntityPopupMenu();
        void CreateDeformableSceneEntityPopupMenu();
		void CreatePositionMarkerPopupMenu();
        void CreateHint();

		CMenu* GetSceneViewPopupMenu();
		CMenu* GetCompoundEntityPopupMenu(const ROS::CompoundSceneEntity& compoundEntity);
		CMenu* GetDeformableEntityPopupMenu(const ROS::DeformableSceneEntity& deformableEntity);
		CMenu* GetStaticSceneEntityPopupMenu(const ROS::AStaticSceneEntity& staticEntity);
		CMenu* GetPositionMarkerPopupMenu(const ROS::PositionMarker& positionMarker);

		void SetInternalCameraToAssociatedCamera();
		void SetAssociatedCameraToInternalCamera();

		void Resize(int width, int height);

		void CreateHardpointPopupMenu(CMenu& popupMenu, const std::auto_ptr<ROS::ConstMotionStateAccessor>& motAccessor);

		void GenerateAVI();

		SceneView&								mAssociatedSceneView;
        HWND                       				mWndH;
	    const ROS::DABaseCamera* 				mCamera;
		std::auto_ptr<ASoundListener>			mSoundListener;
        ROS::ADynamicCamera*					mAssociatedCamera;
		bool									mShowAxes;
        bool									mShowMesh;
		bool									mRenderTextured;
        int										mHeight;
        int										mWidth;
		Mode									mCurrentMode;
        float									mRotateAboutX;
        float									mRotateAboutY;
        int										mLastMouseX;
        int										mLastMouseY;
        CMenu									mPopupMenus;
		Transform								mStartTr;
		int										mStartMouseX;
		int										mStartMouseY;
		HMENU									mSysMenu;
#ifdef PORTED									
        THintWindow*							mHint;
#endif											
		unsigned int							mMinCommandID;
		CommandType								mCommandType;
		bool									mIsControlKeyDown;
		float                                   mLetterboxHToV;      // default is 1.85/1
		bool                                    mShowLetterbox;

		// Members and methods for properly handling multiple views.
		// This works by using a single, shared, rendering surface with multiple windows.
		// The size of the buffer is the size of the desktop if possible, smaller otherwise.
		// If the view window is bigger than the rendering surface, it is scaled up, otherwise
		// rendering occurs at the window's resolution.

		// Static members
		static BOOL                 mPipeInitialized;
		static int                  mPipeWidth;
		static int                  mPipeHeight;
		static TGLSceneViewUIForm * mBufferOwner;

		// Per-view members
		BOOL                        mIsActive;

		// Per-view methods
		void BltRenderBuffer(HDC hDC, RECT *srcRect, RECT *destRect); // blits the current contents of the render buffer to the client rect.
		HBITMAP CreateBufferBitmap(HDC hDC, RECT *srcRect, RPBITMAPINFO* bmp_info);
};
//---------------------------------------------------------------------------
#endif
