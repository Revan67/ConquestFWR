//---------------------------------------------------------------------------
#include "PCH.h"

#ifdef __BCPLUSPLUS__
#include <vcl\vcl.h>
#pragma hdrstop

#include <stdlib>

#else

#include <string.h>
#include <stdlib.h>
#endif

#include "CodeMsg.h"
//---------------------------------------------------------------------------
void AssertMessage(const char* kAssertionP, int lineNum, const char* kFileNameP)
{
	char	messageP[255], lineNumP[10];

    strcpy(messageP, "Assert : ");
    strcat(messageP, kAssertionP);
    strcat(messageP, "\nLine : ");
    strcat(messageP, itoa(lineNum, lineNumP, 10));
    strcat(messageP, "\nFile : ");
	strcat(messageP, kFileNameP);
	strcat(messageP, "\n\n Signal debugger?");

	if(IDYES == MessageBox(NULL, messageP, "Assertion Failed!", MB_YESNO))
    {	DebugBreak();
    }
}
