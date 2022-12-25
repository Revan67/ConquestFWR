//---------------------------------------------------------------------------
// Module Name:
//    undo.h
//
// Abstract:
//    This is a basic undo/redo framework.
//
// Revision History:
//    o 12/1/2001	Yingle Jia (andygood@163.com)
//			Created it
//    o 6/19/2002	Yingle Jia (andygood@163.com)
//			Modified it
//

#ifndef _UNDO_H_
#define _UNDO_H_

#pragma once

#include <list>
#include <string>
using namespace std;

namespace UNDO
{

//---------------------------------------------------------------------------
// forward declarations
//
class Command;
template <class T = Command> class CommandManagerT;

//---------------------------------------------------------------------------
// class Command - abstract class for a command

class Command
{
public:
	virtual const char* GetName() = 0;
	virtual bool Execute() = 0;		// do the command
	virtual bool Unexecute() = 0;	// undo the command
	virtual ~Command() { }			// free your resources in subclasses
};

//---------------------------------------------------------------------------
// class CommandManagerT

template <class T = Command>
class CommandManagerT
{
// Data members
private:
	typedef list<T*>	CommandList;
	CommandList m_undoList;
	CommandList m_redoList;
	unsigned int m_nUndoLevel;
	int m_nCleanCount;

// Constructor & destructor
public:
	CommandManagerT(int nUndoLevel = 100) : 
		m_nUndoLevel(nUndoLevel), 
		m_nCleanCount(0)
	{ }

	~CommandManagerT()
	{
		Clear();
	}

// Attributes
public:
	bool CanUndo() const
	{
		return (m_undoList.size() > 0);
	}

	bool CanRedo() const
	{
		return (m_redoList.size() > 0);
	}

	unsigned int getUndoLevel() const
	{
		return m_nUndoLevel;
	}

	void setUndoLevel(unsigned int newValue)
	{
		m_nUndoLevel = newValue;
	}

	bool IsDirty() const
	{
		return (m_nCleanCount != 0);
	}

	T* getLastUndoCommand() const
	{
		return m_undoList.back();
	}

	T* getLastRedoCommand() const
	{
		return m_redoList.back();
	}

// Methods
public:
	bool DoCommand(T* pCommand)
	{
		// Clear redo list
		ClearRedoList();
		// Execute the command and add it to undo list if succeeded
		if (pCommand->Execute())
		{
			AddUndo(pCommand);
			return true;
		}
		return false;
	}

	void Undo()
	{
		if (CanUndo())
		{
			m_nCleanCount--;
			T* pCommand = m_undoList.back();
			m_undoList.pop_back();
			if (pCommand->Unexecute())
				AddRedo(pCommand);
			else
				delete pCommand;
		}
	}

	void Redo()
	{
		if (CanRedo())
		{
			m_nCleanCount++;
			T* pCommand = m_redoList.back();
			m_redoList.pop_back();
			if (pCommand->Execute())
				AddUndo(pCommand);
			else
				delete pCommand;
		}
	}

	void Clear()
	{
		ClearUndoList();
		ClearRedoList();
	}

	void SetClean()
	{
		m_nCleanCount = 0;
	}

// Implementations
	void AddUndo(T* pCommand)
	{
		if (m_undoList.size() >= m_nUndoLevel)
		{
			delete m_undoList.front();
			m_undoList.pop_front();
		}
		m_undoList.push_back(pCommand);
		if (m_nCleanCount < 0 && m_redoList.size() > 0)
			m_nCleanCount = m_undoList.size() + m_redoList.size() + 1;
		else
			m_nCleanCount++;
	}

	void AddRedo(T* pCommand)
	{
		m_redoList.push_back(pCommand);
	}

	void ClearUndoList()
	{
		ClearCommandList(&m_undoList);
	}

	void ClearRedoList()
	{
		ClearCommandList(&m_redoList);
	}

	void ClearCommandList(CommandList* pList)
	{
		CommandList::iterator it;
		for (it = pList->begin(); it != pList->end(); it++)
			delete *it;
		pList->erase(pList->begin(), pList->end());
	}
};

typedef CommandManagerT<>	CommandManager;

} // namespace UNDO

#endif // ifndef _UNDO_H_
