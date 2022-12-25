#ifndef _INFOAREA_H_
#define _INFOAREA_H_
//InfoArea.h
struct IEffectTarget;
struct IEffectEvent;
struct IEffectAction;
struct ITargetAnim;

namespace InfoArea
{
	void Deselect();

	void SelectTargetList();

	void SelectTarget(IEffectTarget * selTarg);

	void SelectParamList();

	void SelectEvent(IEffectEvent * targ);

	void SelectAction(IEffectAction * targ);

	void SelectTargetAnim(IEffectTarget * selectedTarg,ITargetAnim * targ);
}

#endif