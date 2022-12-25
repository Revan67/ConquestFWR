//////////////////////////////////////////////////////////////////////////////
//  File:    HeDbgMsg.cpp
//
//  Purpose: Implementation of a message to communicate between the python
//  debugger and the debug engine
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001 by Humongous Entertainment
//  
//  This library is free software; you can redistribute it and/or modify it 
//  under the terms of the GNU Lesser General Public License as published by 
//  the Free Software Foundation; either version 2.1 of the License, or  any 
//  later version.
//  
//  This library is distributed in the hope that it will be useful, but 
//  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
//  License for more details.
//  
//  You should have received a copy of the GNU Lesser General Public License 
//  along with this library (see the file license.txt); if not, write to the 
//  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//////////////////////////////////////////////////////////////////////////////	
#include "stdafx.h"
#include "HeDbgMsg.h"


//-----------------------------------------------------------------------------
// Message identifiers 
//
//  Each message type is identified by a four byte id.  This id is specified in 
//  ascii format however the four byte size allows comparison as a long.
//
//  The contents of this array must match the EMessageType enumeration.
const char* aszMsgIds[] = 
{
	"INIT",
	"EVAL",
	"EVER",
	"SOPT",
	"PING",
	"PONG",
	"BRK ",
	"BLST",
	"TRAC",
	"OUTP",
	"DONE",
	"EXEP",
	"GLOB",
	"LOCS",
	"MODS",
	"EXEC",
	"BRPT",
	"BRNW",
	"STEP",
	"LOAD",
	"STOP",
	"EXIT",
};

//check to see that there are the right number of elements in asxMsgIds
CompileTimeAssert(NUMELEMENTS(aszMsgIds) == eMSG_NUMMSGS);




const char* GetMsgString(EMessageType eType)
{
	//check the eType parameter
	assert((eType > eMSG_INVALID) && (eType < eMSG_NUMMSGS));
	//verify the type string is the correct length
	assert(strlen(aszMsgIds[eType]) == sizeof(uint32_t));
	
	return aszMsgIds[eType];
}

uint32_t GetMsgVal(EMessageType eType)
{
	//check the eType parameter
	assert((eType > eMSG_INVALID) && (eType < eMSG_NUMMSGS));
	//verify the type string is the correct length
	assert(strlen(aszMsgIds[eType]) == sizeof(uint32_t));

	return *((uint32_t*)aszMsgIds[eType]);
}

void SMsgHeader::SetMsgSize(size_t size)
{
	int count = _snprintf(MsgSize, NUMELEMENTS(MsgSize), "%d", size);
	assert(count >= 0 && count < NUMELEMENTS(MsgSize));
}

size_t SMsgHeader::GetMsgSize() const
{
	size_t result = strtol(MsgSize, NULL, 10);
	return result;
}
