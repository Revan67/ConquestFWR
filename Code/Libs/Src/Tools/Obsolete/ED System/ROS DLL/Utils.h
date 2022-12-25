//---------------------------------------------------------------------------
#ifndef Utils_h
#define Utils_h

#include <memory>
#include "ManualPtr.h"
//---------------------------------------------------------------------------
template<class T>
inline bool IsNull(const T* kObjP)
{
	return kObjP == NULL;
}
//---------------------------------------------------------------------------
template<class T>
inline bool IsNotNull(const T* kObjP)
{
	return kObjP != NULL;
}
//---------------------------------------------------------------------------
template<class T>
inline bool IsNull(const std::auto_ptr<T>& kObjSPR)
{
	return kObjSPR.get() == NULL;
}
//---------------------------------------------------------------------------
template<class T>
inline bool IsNotNull(const std::auto_ptr<T>& kObjSPR)
{
	return kObjSPR.get() != NULL;
}
//---------------------------------------------------------------------------
template<class T>
inline bool IsNull(const std::manual_ptr<T>& kObjSPR)
{
	return kObjSPR.get() == NULL;
}
//---------------------------------------------------------------------------
template<class T>
inline bool IsNotNull(const std::manual_ptr<T>& kObjSPR)
{
	return kObjSPR.get() != NULL;
}
//---------------------------------------------------------------------------
#endif
