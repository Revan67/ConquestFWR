// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ManualPtr_h
#define ManualPtr_h
// --------------------------------------------------------------------------
// Exactly like auto_ptr except that an instance of manual_ptr does not delete
// the pointer it holds when the instance of manual_ptr is deleted.
namespace std
{
template<class X> class manual_ptr
{
  public:
    //
    // construct/copy/destroy
    //
    explicit manual_ptr (X* p = 0) : the_p(p)           {}
    manual_ptr (const manual_ptr<X>& a)    : the_p(a.release()) {}
    void operator= (const manual_ptr<X>& rhs) { reset(rhs.release()); }

//    ~auto_ptr () { delete the_p; }	// Left out intentionally

    //
    // members
    //
    X& operator*  ()            const { return *the_p;   }
    X* operator-> ()            const { return the_p;    }
    X* get        ()            const { return the_p;    }
    X* release    ()            const { return reset(0); }
    X* reset      (X* p = 0)    const { X* tmp = the_p; the_p = p; return tmp; }

private:

    mutable X* the_p;
};
}
// --------------------------------------------------------------------------
#endif
