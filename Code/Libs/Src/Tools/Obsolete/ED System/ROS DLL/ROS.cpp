//---------------------------------------------------------------------------
#include "PCH.h"
#ifdef _MSC_VER
#include <windows.h>
#else
#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------
//   Important note about DLL memory management when your DLL uses the
//   static version of the RunTime Library:
//
//   If your DLL exports any functions that pass String objects (or structs/
//   classes containing nested Strings) as parameter or function results,
//   you will need to add the library MEMMGR.LIB to both the DLL project and
//   any other projects that use the DLL.  You will also need to use MEMMGR.LIB
//   if any other projects which use the DLL will be perfomring new or delete
//   operations on any non-TObject-derived classes which are exported from the
//   DLL. Adding MEMMGR.LIB to your project will change the DLL and its calling
//   EXE's to use the BORLNDMM.DLL as their memory manager.  In these cases,
//   the file BORLNDMM.DLL should be deployed along with your DLL.
//
//   To avoid using BORLNDMM.DLL, pass string information using "char *" or
//   ShortString parameters.
//
//   If your DLL uses the dynamic version of the RTL, you do not need to
//   explicitly add MEMMGR.LIB as this will be done implicitly for you
//---------------------------------------------------------------------------
USEUNIT("Actor.cpp");
USEUNIT("ASceneEntity.cpp");
USEUNIT("AStaticSceneEntity.cpp");
USEUNIT("ADynamicSceneEntity.cpp");
USEUNIT("CodeMsg.cpp");
USELIB("..\..\Libs\Static\DISPLAY_B.lib");
USEUNIT("ConstMotionStateAccessor.cpp");
USEUNIT("StaticsStateAccessor.cpp");
USEUNIT("Position.cpp");
USEUNIT("Location.cpp");
USEUNIT("Orientation.cpp");
USEUNIT("Scene.cpp");
USEUNIT("DynamicSceneEntity.cpp");
USEUNIT("GLUtils.cpp");
USEUNIT("StringType.cpp");
USEUNIT("DeformableSceneEntity.cpp");
USEUNIT("CompoundSceneEntity.cpp");
USEUNIT("StaticsState.cpp");
USEUNIT("AStaticsState.cpp");
USEUNIT("DynamicsState.cpp");
USEUNIT("ADynamicsState.cpp");
USEUNIT("APhysicalState.cpp");
USEUNIT("SingularStateVariable.cpp");
USEUNIT("AStateVariable.cpp");
USEUNIT("Role.cpp");
USEUNIT("ARole.cpp");
USEUNIT("MatrixUtil.cpp");
USEUNIT("AngularVelocity.cpp");
USEUNIT("Force.cpp");
USEUNIT("LinearVelocity.cpp");
USEUNIT("Torque.cpp");
USELIB("..\..\CHAR\Char_B\Release\char_b_b.lib");
USEUNIT("TimeType.cpp");
USEUNIT("glut_util.cpp");
USEUNIT("glut_shapes.cpp");
USEUNIT("SceneEntityList.cpp");
USEUNIT("Light.cpp");
USEUNIT("Camera.cpp");
USEUNIT("Article.cpp");
USEUNIT("Color.cpp");
USEUNIT("DACameraDynamicsState.cpp");
USEUNIT("DACamera.cpp");
USEUNIT("DeformableEntityStaticsState.cpp");
USEUNIT("ROSSceneModel.cpp");
USEUNIT("SceneModel.cpp");
USEUNIT("Observer.cpp");
USEUNIT("Model.cpp");
USEUNIT("AView.cpp");
USEUNIT("AController.cpp");
USEUNIT("SceneController.cpp");
USEUNIT("ADynamicCamera.cpp");
USEUNIT("DynamicsStateAccessor.cpp");
USEUNIT("CameraDynamicsState.cpp");
USEUNIT("ConstCameraStateAccessor.cpp");
USEUNIT("CameraStateAccessor.cpp");
USEUNIT("ACompoundSceneEntity.cpp");
USEUNIT("MotionStateAccessor.cpp");
USEUNIT("CompoundEntityStaticsState.cpp");
USERES("ROS.res");
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void*)
{
	return 1;
}
//---------------------------------------------------------------------------
