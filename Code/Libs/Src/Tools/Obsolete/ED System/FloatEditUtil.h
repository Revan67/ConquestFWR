// Author: Shaival Varma
// --------------------------------------------------------------------------

#ifndef FloatEditUtil_h
#define FloatEditUtil_h
//---------------------------------------------------------------------------
#include <afx.h>
//---------------------------------------------------------------------------
static float GetFloatValue(const CEdit& edit)
{
	CString	str;
	
	edit.GetWindowText(str);

	return atof(str);
}
//---------------------------------------------------------------------------
static void SetFloatValue(CEdit& edit, float value, unsigned int decimalPlaces)
{
	int	dec, sign;

	CString	str = _fcvt(value, 4, &dec, &sign);

	if(dec >= 0)
	{	
		str = str.Left(dec) + "." + str.Right(str.GetLength() - dec);
	}
	else
	{	
		// got to add leading 0s
		CString	string("0.");

		for(int idx = 0; idx > dec; --idx)
		{
			string += "0";
		}

		str = string + str;
	}

	if(dec == 0)
	{
		str = "0" + str;
	}

	if(sign != 0)
	{
		str = "-" + str;
	}

	edit.SetWindowText(str);
}
//---------------------------------------------------------------------------
#endif