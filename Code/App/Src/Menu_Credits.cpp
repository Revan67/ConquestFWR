//--------------------------------------------------------------------------//
//                                                                          //
//                             Menu_Credits.cpp                              //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_Credits.cpp 10    6/26/01 4:21p Tmauer $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "Frame.h"
#include "IStatic.h"
#include "MusicManager.h"
#include "DrawAgent.h"

#include <DMenu_Credits.h>

#define TEXT_XPOS	0
#define TEXT_HEIGHT	25
#define TEXT_WIDTH  800

#define UPDATE_SCROLL 33

//--------------------------------------------------------------------------//
//
struct CreditText
{
	COMPTR<IFontDrawAgent> fontCredit;
	COMPTR<IFontDrawAgent> fontBack;
	POINT pt;
	wchar_t szText[64];

	CreditText * pNext;

	CreditText (void)
	{
		memset(szText, 0, sizeof(szText));
	}

	void DuplicateFontAgent (IFontDrawAgent * pFont, U32 fontID)
	{
		pFont->CreateDuplicate(fontCredit);

		// create the background font
		HFONT hFont;
		COLORREF background;

		background	= RGB(0,0,0);
		hFont = CQCreateFont(fontID);

		CreateFontDrawAgent(hFont, 0, background | 0xFF000000, background, fontBack);
	}
};

//--------------------------------------------------------------------------//
//
void add_credit_line (CreditText *& list, CreditText * pAdd)
{
	// add the credit to the end of the list
	if (list == NULL)
	{
		list = pAdd;
	}
	else
	{
		CreditText * p = list;
		CreditText * pLast = list;

		while (p)
		{
			if (p->pNext == NULL)
			{
				// p is last in list
				pLast = p;
				break;
			}
			p = p->pNext;
		}
		
		pLast->pNext = pAdd;
	}

	pAdd->pNext = NULL;
}
//--------------------------------------------------------------------------//
//
void delete_first_line (CreditText *&list)
{
	if (list)
	{
		CreditText * p = list;
		list = list->pNext;
		delete p;
	}
}
//--------------------------------------------------------------------------//
//
void delete_whole_fucker (CreditText *& list)
{
	// delete everything in the temp objectives list too
	CreditText * p = list;
	CreditText * pNext = NULL;
	
	while (p)
	{
		pNext = p->pNext;
		delete p;
		p = pNext;
	}

	list = NULL;
}
//--------------------------------------------------------------------------//
//
struct Menu_Credits : public DAComponent<Frame>
{
	//
	// data items
	//
	GT_CREDITS data;

	COMPTR<IStatic> staticBackground;

	COMPTR<IFontDrawAgent> fontTitleTemplate, fontNameTemplate;

	// the list of lines to draw
	CreditText * pCreditsList;
	char * credits_buffer;
	char * bufferPtr;
	
	int fileSize;
	bool bEndOfFile;

	int yOffset;
	int yClimb;

	U32 timer; 

	PANE pane;

	//
	// instance methods
	//

	Menu_Credits (Frame * _parent)
	{
		initializeFrame(_parent);
		init();
	}

	~Menu_Credits (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message, void *parm);

	/* IEventCallback methods */
	
	virtual GENRESULT __stdcall	Notify (U32 message, void *param);


	/* Menu_Credits methods */

	virtual void setStateInfo (void);

	virtual bool onTabPressed (void)
	{
		if (childFrame!=0)
			return false;
		return Frame::onTabPressed();
	}

	virtual void onUpdate (U32 dt)
	{
		timer += dt;

		if (timer >= UPDATE_SCROLL)
		{
			yClimb += 2 * (timer/UPDATE_SCROLL);
			timer = 0;
		}

		// delete any lines that shouldn't be in the list anymore
		U32 linesDeleted = 0;
		CreditText * p = pCreditsList;
		
		while (p)
		{
			if ((p->pt.y - yClimb) > -10)
			{
				break;
			}
			else
			{
				// we've got something to erase
				linesDeleted++;
				delete_first_line(pCreditsList);

				// effectively sets p to p->pNext
				p = pCreditsList;
			}
		}

		// add as many lines as we deleted
		if (linesDeleted && bEndOfFile == false)
		{
			parseCredits(linesDeleted);
		}
	}

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		endDialog(0);
		return true;
	}

	void init (void);

	void drawCredits (void);

	void parseCredits (U32 nLines);

	void alignText (CreditText & credit);
};
//----------------------------------------------------------------------------------//
//
Menu_Credits::~Menu_Credits (void)
{
	// delete the buffer
	if (credits_buffer)
	{
		delete [] credits_buffer;
		credits_buffer = NULL;
	}

	// delete the list of credits
	delete_whole_fucker(pCreditsList);
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_Credits::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT Menu_Credits::Notify (U32 message, void *param)
{
	GENRESULT result = GR_OK;

	switch (message)
	{
	case CQE_ENDFRAME:
		result = Frame::Notify(message, param);
		drawCredits();
		return result;		

	case WM_LBUTTONUP:
		onEscPressed();
		break;

	default:
		break;
	}

	return Frame::Notify(message, param);
}
//----------------------------------------------------------------------------------//
//
void Menu_Credits::setStateInfo (void)
{
	//
	// create members if not done already
	//
	screenRect.left		= IDEAL2REALX(data.screenRect.left);
	screenRect.right	= IDEAL2REALX(data.screenRect.right);
	screenRect.top		= IDEAL2REALY(data.screenRect.top);
	screenRect.bottom	= IDEAL2REALY(data.screenRect.bottom);

	staticBackground->InitStatic(data.staticBackground, this);

	MUSICMANAGER->PlayMusic("multiplayer_menu.wav");

	pane.x0 = 0;
	pane.x1 = 799;
	pane.y0 = 104;
	pane.y1 = 599;

	// create our font draw agents
	HFONT hFont1, hFont2;
	COLORREF pen1, pen2, background;

	pen1		= RGB(49, 90, 198)	| 0xFF000000;		
	pen2		= RGB(160,200,240)	| 0xFF000000;
	background	= RGB(0,0,0);	   
	hFont1		= CQCreateFont(IDS_MESSAGEBOX_FONT);
	hFont2		= CQCreateFont(IDS_OPENING_FONT);

	CreateFontDrawAgent(hFont1, 1, pen1, background, fontTitleTemplate);
	CreateFontDrawAgent(hFont2, 1, pen2, background, fontNameTemplate);

	yOffset = 540;
	yClimb = 0;
	
	// read the first 32 credit entries into the list
	parseCredits(24);

	if (childFrame)
		childFrame->setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_Credits::alignText (CreditText & credit)
{
	U32 swidth = credit.fontCredit->GetStringWidth(credit.szText);
	credit.pt.x = TEXT_XPOS + S32(TEXT_WIDTH - swidth)/2;
}
//--------------------------------------------------------------------------//
//
void Menu_Credits::parseCredits (U32 nLines)
{
/*	bEndOfFile = true;
	CreditText * pTempArray[4];
	
	for (int i = 0; i < 4; i++)
	{
		pTempArray[i] = new CreditText;

		yOffset += TEXT_HEIGHT;
		pTempArray[i]->DuplicateFontAgent(fontNameTemplate, IDS_OPENING_FONT);
		pTempArray[i]->pt.x = TEXT_XPOS;
		pTempArray[i]->pt.y = yOffset;
	}

	wcscpy(pTempArray[0]->szText, L"Under Construction");
	wcscpy(pTempArray[1]->szText, L"Full Credits to come very soon");
	wcscpy(pTempArray[2]->szText, L"No need to be alarmed");
	wcscpy(pTempArray[3]->szText, L"Sean has everything under control");

	for (i = 0; i < 4; i++)
	{
		alignText(*pTempArray[i]);
		add_credit_line(pCreditsList, pTempArray[i]);
	}
*/
	if (credits_buffer == NULL)
	{
		// read the data from the file
		HANDLE hFile = ::CreateFile("credits.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			// find out how big the buffer is and get it into memory
			fileSize = ::GetFileSize(hFile, NULL);

			if (fileSize)
			{
				// read everything into our buffer
				credits_buffer = new char[fileSize+1];
				bufferPtr = credits_buffer;
				
				U32 nRead = 0;

				::ReadFile(hFile, credits_buffer, fileSize, &nRead, NULL);
				CQASSERT(nRead);
				
				// mark the end of the line...
				credits_buffer[fileSize] = 0;
			}

			// now close the handle
			::CloseHandle(hFile);
		}
		else
		{
			// this will atleast keep us from entering this functions every goddamn update
			bEndOfFile = true;
		}
	}

	// now read nLines of lines
	for (U32 i = 0; (i < nLines) && (bEndOfFile == false);)
	{
		char szLine[64];
		char * oldpos = bufferPtr;
		char * newpos = NULL;
		int nChars = 0;

		// read the next line out of the buffer
		newpos = strchr(oldpos, '\n');
		
		// if new pos failed, than we are about to parse our last entry
		if (newpos)
		{
			nChars = (newpos - oldpos) - 1;
		}
		else
		{
			bEndOfFile = true;
			nChars = strlen(oldpos);
		}
		
		memset(szLine, 0, sizeof(szLine));
		if (nChars)
		{
			// only want the line of text
			strncpy(szLine, oldpos, nChars);

			// advance the bufferPtr to get past any '\n''s
			while (newpos && *newpos == '\n')
			{
				newpos++;
			}
			
			//
			// we've got the whole line now, let's parse the fucker
			//

			// first increase the yOffset
			yOffset += TEXT_HEIGHT;

			// if the string starts with a [d] then we are spacing out "[d] %d" pixels
			if (strncmp(szLine, "[d]", 3) == 0)
			{
				// we are requesting to add blank lines between credits
				char * szVal = szLine+3;
				int val = atoi(szVal);
				yOffset += val;
			}
			else if (strncmp(szLine, "[t]", 3) == 0)
			{
				// for new titles, add a little extra line
				yOffset += 15;

				// we are starting a new title here
				CreditText * titleLine = new CreditText;

				_localAnsiToWide(szLine+4, titleLine->szText, sizeof(titleLine->szText));
				
				titleLine->pt.x = TEXT_XPOS;
				titleLine->pt.y = yOffset;
				titleLine->DuplicateFontAgent(fontTitleTemplate, IDS_MESSAGEBOX_FONT);
				
				alignText(*titleLine);
				add_credit_line(pCreditsList, titleLine);
				++i;
			}
			else
			{
				// we have just names to put in
				CreditText * nameLine = new CreditText;

				_localAnsiToWide(szLine, nameLine->szText, sizeof(nameLine->szText));
				
				nameLine->pt.x = TEXT_XPOS;
				nameLine->pt.y = yOffset;
				nameLine->DuplicateFontAgent(fontNameTemplate, IDS_OPENING_FONT);
				
				alignText(*nameLine);
				add_credit_line(pCreditsList, nameLine);
				++i;
			}

			// assign the new bufferPtr
			bufferPtr = newpos;
		}
		else
		{
			++i;
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_Credits::init (void)
{
	data = 	*((GT_CREDITS *) GENDATA->GetArchetypeData("Menu_Credits"));

	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(data.staticBackground.staticType, pComp);
	pComp->QueryInterface("IStatic", staticBackground);
}
//--------------------------------------------------------------------------//
//
void Menu_Credits::drawCredits (void)
{
	POINT pos;
	wchar_t * pString;

	CreditText * p = pCreditsList;

	while (p)
	{
		pString = p->szText;
		pos.x = p->pt.x;
		pos.y = p->pt.y - yClimb;

		// draw the string
		p->fontBack->StringDraw(&pane, pos.x+2, pos.y+2, pString);
		p->fontCredit->StringDraw(&pane, pos.x, pos.y, pString);

		p = p->pNext;
	}
}
//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_Credits (Frame * parent)
{
	GENDATA->FlushUnusedArchetypes();
	Menu_Credits * menu = new Menu_Credits(parent);

	menu->createViewer("\\GT_CREDITS\\Menu_Credits", "GT_CREDITS", IDS_VIEWCREDITS);
	menu->beginModalFocus();

	CURSOR->EnableCursor(false);

	U32 result = CQDoModal(menu);
	delete menu;

	CURSOR->EnableCursor(true);

	return result;
}