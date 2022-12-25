#ifndef _PARTICLEVIEW_H_
#define _PARTICLEVIEW_H_
//PArticleView.h

namespace ParticleView
{
	HWND Open();

	void Null();

	void Resize();

	void EmmiterWinSelect(HWND hWindow,S32 xPos, S32 yPos);

	void EmmiterWinUpdateMove(HWND hWindow,S32 xPos, S32 yPos);

	void EmmiterWinRelease(HWND hWindow);

	void OutputClicked(HWND hWindow, U32 id);

	void InputClicked(HWND hWindow);

	void InvalidateView();
}

#endif