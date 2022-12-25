#ifndef _MATHLAYOUTWIN_H_
#define _MATHLAYOUTWIN_H_
//MathLayoutWin.h

struct FloatType;
struct TransformType;

namespace MathLayoutWin
{
	FloatType * EditFloat(FloatType * input);

	TransformType * EditTrans(TransformType * input);
}

#endif