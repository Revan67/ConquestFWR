#ifndef _PREVIEWWIN_H_
#define _PREVIEWWIN_H_
//PreviewWin.h
struct IParticleInstance;

namespace PreviewWin
{
	void Create();

	void Open();

	void Close();

	void Update();

	void RenderAxis();

	void OnMouseDown(U32 xPos, U32 yPos);

	void OnMouseUp(U32 xPos, U32 yPos);

	void OnMouseMove(U32 xPos, U32 yPos);

	void Play();

	void PauseToggle();

	void Stop();

	void AddParticleEffect(IParticleInstance * inst);

	SINGLE GetGameTime();

	SINGLE GetRenderTime();
}

#endif