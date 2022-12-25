#ifndef EVENTPRIORITY_H
#define EVENTPRIORITY_H
//--------------------------------------------------------------------------//
//                                                                          //
//                            EventPriority.h                               //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

   $Header: /Conquest/App/Src/EventPriority.h 19    9/11/00 3:32p Jasony $

   Event priorities
*/
//--------------------------------------------------------------------------//

#define EVENT_PRIORITY_CURSOR	 		0xF0001000
#define EVENT_PRIORITY_STATUS			0xF0000C00
#define EVENT_PRIORITY_LINEDISPLAY		0xF0000D00
#define EVENT_PRIORITY_CONFIRM_EXIT		0xF0000080
#define EVENT_PRIORITY_DIPLOMACYMENU	0xF0000075
#define EVENT_PRIORITY_CHATMENU			0xF0000070
#define EVENT_PRIORITY_TEXTCHAT			0xF0000065
#define EVENT_PRIORITY_MENU				0xF0000060
#define EVENT_PRIORITY_MUSIC			0xF0000050
#define EVENT_PRIORITY_SFX				0xF0000040

#define EVENT_PRIORITY_IG_OPTIONS	0x90000080			// in-game options
#define EVENT_PRIORITY_MODALRECT	0x90000000
#define EVENT_PRIORITY_PAUSEMENU	0x80010060
#define EVENT_PRIORITY_OBJECTIVES   0x80010040
#define EVENT_PRIORITY_TALKINGHEAD	0x80001000
#define EVENT_PRIORITY_TOOLBAR		0x80000100
#define EVENT_PRIORITY_STATIC_HIGH	0x80000002
#define EVENT_PRIORITY_HOTRECT		0x80000001
#define EVENT_PRIORITY_SLIDER		0x80000000
#define EVENT_PRIORITY_BUTTON		0x80000000
#define EVENT_PRIORITY_CAMERA		0x80000000
#define EVENT_PRIORITY_MISSION      0x78000000
#define EVENT_PRIORITY_OBJLIST      0x70000000
#define EVENT_PRIORITY_STATIC		0x40000000

#define EVENT_PRIORITY_ENVIRONMENT  0x00000100


//--------------------------------------------------------------------------//
//--------------------------End EventPriority.h-----------------------------//
//--------------------------------------------------------------------------//
#endif