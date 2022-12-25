//---------------------------------------------------------------------------
#ifndef CodeMsgH
#define CodeMsgH
//---------------------------------------------------------------------------

void AssertMessage(const char* kAssertionP, int lineNum, const char* kFileNameP);

#if defined(CODE_MSG)
	#define ASSERT(assertion) 								\
		if(!(assertion))									\
        {	AssertMessage(#assertion, __LINE__, __FILE__);	\
        };
#else
	#define ASSERT(assertion)
#endif

#endif
