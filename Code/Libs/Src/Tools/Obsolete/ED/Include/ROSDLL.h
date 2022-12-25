//---------------------------------------------------------------------------
#ifndef ROSDLL_H
#define ROSDLL_H
//---------------------------------------------------------------------------
#ifdef NO_DLL_EXPORTS

#define CPP_DECL 
#define CPP_DEFN 

#else

	#undef CPP_DECL
	#undef CPP_DEFN

	#ifdef BUILD_ROS_DLL
		#define CPP_DECL __declspec(dllexport)
		#define CPP_DEFN __declspec(dllexport)
	#else
		#define CPP_DECL __declspec(dllimport)
	#endif

#endif
//---------------------------------------------------------------------------
#endif

