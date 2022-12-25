// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Links_h
#define Links_h

#ifdef _MSC_VER
#include <vector>
#else
#include <Include\vector>
#endif
#include <list>
#include <memory>
#include "ManualPtr.h"
// --------------------------------------------------------------------------
template <class T>
class AssPointer: public std::manual_ptr<T>
{
	public:
        explicit AssPointer (T* p = 0)
        :BaseClass(p)
        {
        }
        AssPointer (AssPointer<T>& a)
        :BaseClass(a)
        {
        }

    private:
    	typedef std::manual_ptr<T> BaseClass;
};
// --------------------------------------------------------------------------
template <class T>
class AssVector: public std::vector<T>
{
};
// --------------------------------------------------------------------------
template <class T>
class AggPointer: public std::manual_ptr<T>
{
	public:
        explicit AggPointer (T* p = 0)
        :BaseClass(p)
        {
        }
        AggPointer (AggPointer<T>& a)
        :BaseClass(a)
        {
        }

    private:
    	typedef std::manual_ptr<T> BaseClass;
};
// --------------------------------------------------------------------------
template <class T>
class AggAPointer: public std::auto_ptr<T>
{
	public:
        explicit AggAPointer (T* p = 0)
        :BaseClass(p)
        {
        }
        AggAPointer (AggAPointer<T>& a)
        :BaseClass(a)
        {
        }
		AggAPointer<T>& operator=(const AggAPointer& a)
		{
			BaseClass::operator=(a);

			return *this;
		}
    private:
    	typedef std::auto_ptr<T> BaseClass;
};
// --------------------------------------------------------------------------
template <class T>
class AggVector: public std::vector<T>
{
    public:
        explicit AggVector(size_type n)
        : BaseClass(n)
        {
        }

    private:
    	typedef std::vector<T> BaseClass;
};
// --------------------------------------------------------------------------
template <class T>
class AggList: public std::list<T>
{
};
// --------------------------------------------------------------------------
template <class T>
class AggAList: public std::list< std::auto_ptr<T> >
{
};
// --------------------------------------------------------------------------
#endif
