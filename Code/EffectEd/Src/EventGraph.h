#ifndef _EVENTGRAPH_H_
#define _EVENTGRAPH_H_
//EventGraph.h
namespace EventGraph
{
	void Deselect();

	void SelectEvent(IEffectEvent * event);

	void SelectAction(IEffectAction * action);

	void InvalidateEventGraph();
}

#endif