//--------------------------------------------------------------------------//
//                                                                          //
//                                Helper.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Scripts/Helper/helper.cpp 78    7/06/01 11:07a Tmauer $
   
*/
//--------------------------------------------------------------------------//
#include "ScriptDef.h"
#include "MPartRef.h"
#include "helper.h"

//--------------------------------------------------------------------------
// Helper.cpp DATA...
//--------------------------------------------------------------------------

S32 GameSpeed;

MPartRef Empty;		//Bogus MPartRef for PlayComm reasons...

//--------------------------------------------------------------------------

char MagpieString[NUM_CHARS][64] = 
{
	"", "Animate!!Blackwell", "Animate!!Halsey", "Animate!!Hawkes", "Animate!!Benson", 
	"Animate!!KerTak", "Animate!!Takei", "Animate!!Steele", "Animate!!Smirnoff", "Animate!!Malkor", 
	"", "Animate!!Vivac", "Animate!!Elan", "Animate!!Natus", "", ""
};

char MagpieString2[NUM_CHARS][64] = 
{
	"", "Animate!!Blackwell2", "Animate!!Halsey2", "Animate!!Hawkes2", "Animate!!Benson2", 
	"Animate!!KerTak2", "Animate!!Takei2", "Animate!!Steele2", "Animate!!Smirnoff2", "Animate!!Malkor2", 
	"", "Animate!!Vivac2", "Animate!!Elan2", "Animate!!Natus2", "", ""
};

//--------------------------------------------------------------------------
// CountObjects: Returns the total number of active objects matching the
// PlayerID and object type, and optionally in the system SystemID
//--------------------------------------------------------------------------
U32 CountObjects(S32 type, U32 PlayerID, U32 SystemID)
{
	U32 count=0;

	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid() )
	{
		if( part->bReady && part->playerID == PlayerID && ( type == 0 || part->mObjClass == type ) )
		{
			if ( !SystemID || part->systemID == SystemID )
				count++;
		}

		part = M_ GetNextPart(part);

	}

	return count;
}

//-----------------------------------------------------------------------------
// CountObjectsInSupply: Returns the total number of active objects matching
// the PlayerID and object type that are fully supplied and optionally in SystemID
//--------------------------------------------------------------------------
U32 CountObjectsInSupply (S32 type, U32 PlayerID, U32 SystemID)
{
	U32 count = 0;
	MPartRef part;
	part = M_ GetFirstPart();

	while (part.isValid())
	{
		if(part->bReady && part->playerID == PlayerID && (type == 0 || part->mObjClass == type) && part->supplies >= part->supplyPointsMax)
		{
			if (!SystemID || part->systemID == SystemID)
				count++;
		}
		part = M_ GetNextPart(part);
	}

	return count;
}

//--------------------------------------------------------------------------
// CountObjectsInRange: Returns the total number of active objects matching the
// PlayerID and object type within the circle define by loc and range
//--------------------------------------------------------------------------
U32 CountObjectsInRange(S32 type, U32 PlayerID, MPartRef & loc, S32 range)
{
	U32 count=0;

	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{

		if(part->bReady && part->playerID == PlayerID && (type == 0 || part->mObjClass == type) && (part->systemID == loc->systemID))
		{
			if (M_ DistanceTo(part, loc) <= range)
				count++;
		}

		part = M_ GetNextPart(part);

	}

	return count;
}

//-----------------------------------------------------------------------------
// CountObjectsInRangeInSupply: Returns the total number of active objects matching
// the PlayerID and object type that are fully supplied within the circle defined
// by loc and range
//--------------------------------------------------------------------------
U32 CountObjectsInRangeInSupply (S32 type, U32 PlayerID, MPartRef & loc, S32 range)
{
	U32 count = 0;
	MPartRef part;
	part = M_ GetFirstPart();

	while (part.isValid())
	{
		if(part->bReady && part->playerID == PlayerID && part->mObjClass == type && part->supplies >= part->supplyPointsMax)
		{
			if (M_ DistanceTo(part, loc) <= range)
				count++;
		}
		part = M_ GetNextPart(part);
	}

	return count;
}

void MassStance(S32 type, U32 PlayerID, UNIT_STANCE stance, U32 SystemID)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{
	
		if(part->playerID == PlayerID && (!type || part->mObjClass == type))
		{
			if (!SystemID || part->systemID == SystemID)
			{
				UNIT_STANCE curStance;
				curStance = M_ GetStance(part);
				if (curStance != stance)
					M_ SetStance(part, stance);
			}
		}

		part = M_ GetNextPart(part);

	}

}

void MassStanceInRange(S32 type, U32 PlayerID, UNIT_STANCE stance, MPartRef & loc, S32 range)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{

		if(part->playerID == PlayerID && part->mObjClass == type)
		{
			if (M_ DistanceTo(part, loc) <= range)
			{
				UNIT_STANCE curStance;
				curStance = M_ GetStance(part);
				if (curStance != stance)
					M_ SetStance(part, stance);
			}
		}

		part = M_ GetNextPart(part);

	}

}

void MassAttack(S32 type, U32 PlayerID, MPartRef & target, U32 SystemID)
{
	MPartRef part;
    MGroupRef tempGroup;
    bool groupEmpty = true;

	part = M_ GetFirstPart();

	while (part.isValid())
	{
	
		if(part->caps.attackOk && part->playerID == PlayerID && part->systemID && (!type || part->mObjClass == type))
		{
			if (!M_ IsPlatform(part) && (!SystemID || part->systemID == SystemID))
            {
                groupEmpty = false;

                tempGroup += part;
            }
		}

		part = M_ GetNextPart(part);

	}

    if ( !groupEmpty )
    {
        MScript::OrderAttack( tempGroup, target );
    }

}

void MassAttackInRange(S32 type, U32 PlayerID, MPartRef & target, MPartRef & loc, S32 range)
{
	MPartRef part;
    MGroupRef tempGroup;
    bool groupEmpty = true;

	part = M_ GetFirstPart();

	while (part.isValid())
	{

		if(part->playerID == PlayerID && part->mObjClass == type)
		{
			if (M_ DistanceTo(part, loc) <= range)
            {
                groupEmpty = false;

                tempGroup += part;
            }
		}

		part = M_ GetNextPart(part);

	}

    if ( !groupEmpty )
    {
        MScript::OrderAttack( tempGroup, target );
    }
}

void MassMove(S32 type, U32 PlayerID, MPartRef & target, U32 SystemID)
{
	MPartRef part;
    MGroupRef tempGroup;
    bool groupEmpty = true;

	part = M_ GetFirstPart();

	while (part.isValid())
	{
	
		if(part->playerID == PlayerID && part->mObjClass == type)
		{
			if (!SystemID || part->systemID == SystemID)
            {
                groupEmpty = false;

                tempGroup += part;
            }
		}

		part = M_ GetNextPart(part);

	}

    if ( !groupEmpty )
    {
        MScript::OrderMoveTo( tempGroup, target );
    }
}

void MassMoveInRange(S32 type, U32 PlayerID, MPartRef &target, MPartRef &loc, S32 range)
{
	MPartRef part;
    MGroupRef tempGroup;
    bool groupEmpty = true;

	part = M_ GetFirstPart();

	while (part.isValid())
	{

		if(part->playerID == PlayerID && part->mObjClass == type)
		{
			if (M_ DistanceTo(part, loc) <= range)
            {
                groupEmpty = false;

                tempGroup += part;
            }
		}

		part = M_ GetNextPart(part);

	}

    if ( !groupEmpty )
    {
        MScript::OrderMoveTo( tempGroup, target );
    }
}



void MassTeleport(S32 type, U32 PlayerID, MPartRef & target, U32 SystemID)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{
	
		if(part->playerID == PlayerID && part->mObjClass == type)
		{
			if (!SystemID || part->systemID == SystemID)
				M_ OrderTeleportTo(part, target);
		}

		part = M_ GetNextPart(part);

	}

}

void MassCancel(S32 type, U32 PlayerID, U32 SystemID)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{
	
		if(part->playerID == PlayerID && part->mObjClass == type)
		{
			if (!SystemID || part->systemID == SystemID)
				M_ OrderCancel(part);
		}

		part = M_ GetNextPart(part);

	}

}

//Cancel all units in a certain system...its best to use a SystemID with this one though
//or else ALL you units will go to sleep...

void MassCancelAll(U32 PlayerID, U32 SystemID)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{
	
		if(part->playerID == PlayerID)
		{
			if (!SystemID || part->systemID == SystemID)
				M_ OrderCancel(part);
		}

		part = M_ GetNextPart(part);

	}

}

void MassAttackCap(U32 PlayerID, bool bAttackOk, U32 SystemID)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{
	
		if(part->playerID == PlayerID && !M_ IsPlatform(part))
		{
			if (!SystemID || part->systemID == SystemID)
				M_ EnableAttackCap(part, bAttackOk);
		}

		part = M_ GetNextPart(part);

	}

}

void MassSwitchSystem(U32 PlayerID, U32 NewID, U32 SystemID)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{
	
		if(part->playerID == PlayerID && part->systemID == SystemID)
		{
				M_ SwitchPlayerID(part, NewID);
		}

		part = M_ GetNextPart(part);

	}

}
//---------------------------------------------------------------
// Calls EnableAIForPart(bOn) for each object owned by PlayerID
//---------------------------------------------------------------
void MassEnableAI(U32 PlayerID, bool bOn, U32 SystemID)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{

		if(part->playerID == PlayerID)
		{
			if (!SystemID || part->systemID == SystemID)
				M_ EnableAIForPart(part, bOn);
		}

		part = M_ GetNextPart(part);
	}

}

//---------------------------------------------------------------
// Calls EnableAIForPart(bOn) for each gunboat owned by PlayerID
//---------------------------------------------------------------
void MassEnableGunboatAI(U32 PlayerID, bool bOn, U32 SystemID)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{

		if(part->playerID == PlayerID && M_ IsGunboat(part))
		{
			if (!SystemID || part->systemID == SystemID)
				M_ EnableAIForPart(part, bOn);
		}

		part = M_ GetNextPart(part);
	}

}

//---------------------------------------------------------------
// Calls EnableAIForPart(bOn) for each platform owned by PlayerID
//---------------------------------------------------------------
void MassEnablePlatformAI(U32 PlayerID, bool bOn, U32 SystemID)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{

		if(M_ IsPlatform(part) && part->playerID == PlayerID)
		{
			if (!SystemID || part->systemID == SystemID)
				M_ EnableAIForPart(part, bOn);
		}

		part = M_ GetNextPart(part);
	}

}

//---------------------------------------------------------------

void MassSwitchPlayerIDInRange(S32 type, U32 PlayerID, U32 NewPlayerID, MPartRef & loc, S32 range)
{
	MPartRef part, part2;

	part = M_ GetFirstPart();

	while (part.isValid())
	{

		if(part->playerID == PlayerID && (!type || part->mObjClass == type))
		{
			if (M_ DistanceTo(part, loc) <= range)
			{
				//  DOES NOT WORK FOR JUMPPLAT OBJECTS NOW

				//if (part->mObjClass == M_JUMPPLAT)
				//	part2 = M_ GetJumpgateSibling(part);
				M_ SwitchPlayerID(part, NewPlayerID);
				//if (part2.isValid())
				//	M_ SwitchPlayerID(part2, NewPlayerID);
			}
		}

		part = M_ GetNextPart(part);

	}

}

//---------------------------------------------------------------
// FindFirstObjectOfType: Returns the first object in the
// active object list of specified type, player id and system...
//---------------------------------------------------------------
MPartRef FindFirstObjectOfType (S32 type, U32 PlayerID, U32 SystemID)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{
		if(part->playerID == PlayerID && (!type || part->mObjClass == type))
		{
			if (part->systemID == SystemID || !SystemID)
			return part;
		}
		part = M_ GetNextPart(part);
	}
	return part;
}

MPartRef FindFirstPlat (U32 PlayerID, U32 SystemID)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{
		if(part->playerID == PlayerID && M_ IsPlatform(part))
		{
			if (part->systemID == SystemID || !SystemID)
			return part;
		}
		part = M_ GetNextPart(part);
	}
	return part;
}
//---------------------------------------------------------------
// FindNextObjectOfType: Returns the next object in the
// active object list of specified type, player id and system...
//---------------------------------------------------------------
MPartRef FindNextObjectOfType (S32 type, U32 PlayerID, U32 SystemID, MPartRef & prevPart)
{
	MPartRef part;

	part = M_ GetNextPart(prevPart);

	while (part.isValid())
	{
		if(part->playerID == PlayerID && part->mObjClass == type)
		{
			if (part->systemID == SystemID || !SystemID)
			return part;
		}
		part = M_ GetNextPart(part);
	}
	return part;
}

//---------------------------------------------------------------
// PlayerInSystem: Checks to see if the player has any units in the
// specified system...
//---------------------------------------------------------------
MPartRef PlayerInSystem (U32 PlayerID, U32 SystemID)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{
		if(part->playerID == PlayerID && part->systemID == SystemID)
		{
			if (part->mObjClass != M_JUMPPLAT)
				return part;
		}
		part = M_ GetNextPart(part);
	}

	return part;
}
//---------------------------------------------------------------
MPartRef GenAndMove(const char * name, MPartRef & Generator, MPartRef & WayPoint)
{
	MPartRef part;

	M_ SetGenType(Generator, name);
	part = M_ GenForceGeneration(Generator);
	M_ OrderMoveTo(part, WayPoint);

	return part;
}

//---------------------------------------------------------------
MPartRef GenAndAttack(const char * name, MPartRef & Generator)
{
	MPartRef part;
	MPartRef Enemy;

	M_ SetGenType(Generator, name);
	part = M_ GenForceGeneration(Generator);

	Enemy = M_ FindNearestEnemy(part, true, false);

	M_ ClearPath(part->systemID, Enemy->systemID, part->playerID);

	M_ OrderAttack(part, Enemy);

	return part;

}

//---------------------------------------------------------------
void AttackNearestEnemy( MPartRef & Attacker)
{
	MPartRef Enemy;

	Enemy = M_ FindNearestEnemy(Attacker, true, false);

	M_ ClearPath(Attacker->systemID, Enemy->systemID, Attacker->playerID);

	M_ OrderAttack(Attacker, Enemy);
}

//---------------------------------------------------------------
bool VisibleOrDead( MPartRef & part, U32 PlayerID)
{
	if (!part.isValid())
		return true;
	
	if (M_ IsVisibleToPlayer(part, PlayerID))
		return true;

	return false;
}

//---------------------------------------------------------------

void UnderMissionControl(void)
{
	M_ EnableMouse(false);

	//M_ EnableMovieMode(true);

	GameSpeed = M_ GetGameSpeed();

	M_ SetGameSpeed(0);
}

//---------------------------------------------------------------
void UnderPlayerControl(void)
{
	M_ SetGameSpeed(GameSpeed);

	//M_ EnableMovieMode(false);

	M_ EnableMouse(true);

}
//---------------------------------------------------------------

//-----------------------------------------------------------------------------


U32 PlayBlackwellMagpie (const char * file, U32 text, MPartRef & speaker, U32 mhandle, U32 subtitle)
{
	#define CONVERSION_TO_MILLSECONDS 1000

	U32 new_mhandle = 0;

	M_ EndStream(mhandle);
	new_mhandle = M_ PlayAnimatedMessage(file, "Animate!!Blackwell", BlackwellVidLeft, BlackwellVidTop, speaker, subtitle);

//	text_hold += ttl;

//	U32 duration = text_hold;

//	ttl *= CONVERSION_TO_MILLSECONDS;
//	text_hold *= CONVERSION_TO_MILLSECONDS;
//	M_ FlushTeletype();
//	M_ PlayTeletype(text, BlackwellTextLeft, BlackwellTextTop, BlackwellTextRight, BlackwellTextBottom,	TalkyTextColor, text_hold, ttl, true);

	return new_mhandle;

}

//-----------------------------------------------------------------------------
//  

MPartRef GetFirstJumpgate(U32 fromSystemID, U32 toSystemID, U32 playerID)
{
	MPartRef part = M_ GetFirstPart();

	while (part.isValid())
	{
		if(part->mObjClass == M_JUMPPLAT)	//found jumpgate
		{
			if (part->systemID == fromSystemID && M_ GetJumpgateDestination(part) == toSystemID)
			{
				if (playerID == 0)	//requesting any player, so we got it
					return part;
				else if (part->playerID == playerID)	//correct player
					return part;
			}
		}
		part = M_ GetNextPart(part);
	}

	return part;	//returns invalid part if not found
}

//-----------------------------------------------------------------------------
//  

MPartRef GetNextJumpgate(U32 fromSystemID, U32 toSystemID, MPartRef & prevgate, U32 playerID)
{
	MPartRef part = M_ GetNextPart(prevgate);

	while (part.isValid())
	{
		if(part->mObjClass == M_JUMPPLAT)	//found jumpgate
		{
			if (part->systemID == fromSystemID && M_ GetJumpgateDestination(part) == toSystemID)
			{
				if (playerID == 0)	//requesting any player, so we got it
					return part;
				else if (part->playerID == playerID)	//correct player
					return part;
			}
		}
		part = M_ GetNextPart(part);
	}

	return part;	//returns invalid part if not found
}
//-----------------------------------------------------------------------------

void TextOutput(char * buffer)
{
	FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), buffer);
}

//-----------------------------------------------------------------------------

bool IsSomethingPlaying(U32 handle)
{
	if (M_ IsTeletypePlaying(handle))
		return true;
	else if (M_ IsStreamPlaying(handle))
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------

#define MOLeft				150
#define MORight				500
#define MOTop				175
#define MOBottom			300

// Handles various methods of doing mission dialogue:  
//   must have either TELETYPE_SPEECH, AUDIO_SPEECH, OR MAGPIE_SPEECH defined

U32 DoSpeech(U32 stringID, char *file, U32 holdTime, U32 printTime, const MPartRef &part, CHAR_TYPE charNum)
{

#ifdef _TELETYPE_SPEECH

	M_ FlushTeletype();
	return M_ PlayTeletype(stringID, MOLeft, MOTop, MORight, MOBottom, MOTextColor, holdTime, printTime, false);

#endif

#ifdef _AUDIO_SPEECH

	if (M_ IsAudioThere(file))
	{
		M_ FlushStreams();
		return M_ PlayAudio(file, part); 
	}
	else
	{
		M_ FlushTeletype();
		return M_ PlayTeletype(stringID, MOLeft, MOTop, MORight, MOBottom, MOTextColor, holdTime, printTime, false);
	}

#endif

#ifdef _MAGPIE_SPEECH

	if (M_ IsAudioThere(file))
	{
		char txtfile[128];
		U32 len = strlen(file);
		strcpy(txtfile, file);
		txtfile[len-3] = 't'; txtfile[len-2] = 'x'; txtfile[len-1] = 't';

		M_ FlushStreams();

		if (strlen(MagpieString2[charNum]) > 0 && M_ IsAudioThere(txtfile))
			return M_ PlayAnimatedMessage(file, MagpieString2[charNum], MagpieLeft, MagpieTop, part, stringID);
		else
		{
			return M_ PlayAudio(file, part,stringID); 
		}
	}
	else
	{
		M_ FlushTeletype();
		return M_ PlayTeletype(stringID, MOLeft, MOTop, MORight, MOBottom, MOTextColor, holdTime, printTime, false);
	}

#endif

	return 0;
}

//-----------------------------------------------------------------------------

U32 DoBriefingSpeech(U32 stringID, char *file, U32 holdTime, U32 printTime, U32 slotNum, CHAR_TYPE charNum, bool bStatic)
{

#ifdef _TELETYPE_SPEECH

	M_ FlushTeletype();
	return M_ PlayBriefingTeletype(stringID, MOTextColor, holdTime, printTime, false);

#endif

#ifdef _AUDIO_SPEECH

	if (M_ IsAudioThere(file))
	{
		M_ FlushStreams();
		return M_ PlayAudio(file); 
	}
	else
	{
		M_ FlushTeletype();
		return M_ PlayBriefingTeletype(stringID, MOTextColor, holdTime, printTime, false);
	}

#endif

#ifdef _MAGPIE_SPEECH

	if (M_ IsAudioThere(file))
	{
		char txtfile[128];
		U32 len = strlen(file);
		strcpy(txtfile, file);
		txtfile[len-3] = 't'; txtfile[len-2] = 'x'; txtfile[len-1] = 't';
		if (strlen(MagpieString[charNum]) > 0 && M_ IsAudioThere(txtfile))	
		{
			CQBRIEFINGITEM slotAnim;	
			strcpy(slotAnim.szFileName, file);
			slotAnim.slotID = slotNum;
			slotAnim.bHighlite = false;	
			slotAnim.bContinueAnimating = bStatic;
			strcpy(slotAnim.szTypeName, MagpieString[charNum]);
			M_ FreeBriefingSlot(slotNum);
			U32 handle = M_ PlayBriefingTalkingHead(slotAnim);
			MScript::BriefingSubtitle(handle,stringID);
			return handle;
		}
		else
		{
			U32 handle = M_ PlayAudio(file);
			MScript::BriefingSubtitle(handle,stringID);
			return handle;
		}

	}
	else
	{
		M_ FlushTeletype();
		return M_ PlayBriefingTeletype(stringID, MOTextColor, holdTime, printTime, false);
	}

#endif

	return 0;
}

//-----------------------------------------------------------------------------

U32 DoBriefingSpeechMagpie(U32 stringID, char *file, U32 holdTime, U32 printTime, U32 slotNum, CHAR_TYPE charNum, bool bStatic)
{

	if (strlen(MagpieString[charNum]) > 0)
	{
		CQBRIEFINGITEM slotAnim;
		strcpy(slotAnim.szFileName, file);
		slotAnim.slotID = slotNum;
		slotAnim.bHighlite = false;
		slotAnim.bContinueAnimating = bStatic;
		strcpy(slotAnim.szTypeName, MagpieString[charNum]);
		M_ FreeBriefingSlot(slotNum);
		U32 handle = M_ PlayBriefingTalkingHead(slotAnim);
		MScript::BriefingSubtitle(handle,stringID);
		return handle;
	}
	else
	{
		U32 handle = M_ PlayAudio(file);
		MScript::BriefingSubtitle(handle,stringID);
		return handle;
	}

}

//-----------------------------------------------------------------------------

U32 DoBriefingSpeechMagpieDirect
( 
	U32 stringID,
    char *file, 
    U32 slotNum,
    CHAR_TYPE charNum,
    bool bStatic
)
{
	CQBRIEFINGITEM slotAnim;

	strcpy( slotAnim.szFileName, file );
	strcpy( slotAnim.szTypeName, MagpieString[charNum] );

	slotAnim.slotID = slotNum;
	slotAnim.bHighlite = false;
	slotAnim.bContinueAnimating = bStatic;

    MScript::FreeBriefingSlot( slotNum );

	U32 handle = M_ PlayBriefingTalkingHead(slotAnim);
	MScript::BriefingSubtitle(handle,stringID);
	return handle;
}

//-----------------------------------------------------------------------------

// SLOTNUM OF -1 MEANS ALL SLOTS
U32 ShowBriefingAnimation(S32 slotNum, char *archetype, U32 timer, bool bLoop, bool bRepeat)
{
	CQBRIEFINGITEM slotAnim;

	strcpy(slotAnim.szTypeName, "Animate!!");
	strcat(slotAnim.szTypeName, archetype);

	slotAnim.dwTimer = timer;
	slotAnim.bHighlite = false;
	slotAnim.bContinueAnimating = bRepeat;
	slotAnim.bLoopAnimation = bLoop;

	if (slotNum >= 0)
	{
		slotAnim.slotID = slotNum;
		return M_ PlayBriefingAnimation(slotAnim);
	}
	else
	{
		U32 handle;
		slotAnim.slotID = 1;
		handle = M_ PlayBriefingAnimation(slotAnim);
		slotAnim.slotID = 2;
		handle = M_ PlayBriefingAnimation(slotAnim);
		slotAnim.slotID = 3;
		handle = M_ PlayBriefingAnimation(slotAnim);
		slotAnim.slotID = 0;
		handle = M_ PlayBriefingAnimation(slotAnim);
		return handle;
	}
				
}

//-----------------------------------------------------------------------------

U32 ShowBriefingHead(U32 slotNum, CHAR_TYPE charNum)
{
	return DoBriefingSpeechMagpie(0, "empty.wav", 0, 0, slotNum, charNum, true);
}

//--------------------------------------------------------------------------//
//	STANDARD TELETYPE METHODS

U32 TeletypeObjective( U32 stringID )
{
    U32 displayTime;

    displayTime = MScript::GetScriptStringLength( stringID ) * NEW_OBJECTIVES_MILS_PER_CHAR;

	return M_ PlayTeletype( stringID, 
				NEW_OBJECTIVES_LEFT, NEW_OBJECTIVES_TOP, NEW_OBJECTIVES_RIGHT, NEW_OBJECTIVES_BOTTOM, 
				NEW_OBJECTIVES_COLOR, NEW_OBJECTIVES_HOLD_TIME + displayTime, displayTime,
                false );
}

U32 TeletypeMissionOver( U32 stringID ,U32 failureID)
{
	U32 displayTime;

    displayTime = MScript::GetScriptStringLength( stringID ) * STANDARD_TELETYPE_MILS_PER_CHAR;

	if(failureID)
	{
		M_ PlayTeletype( failureID, 
		        MISSION_OVER_TELETYPE_MIN_X_POS, MISSION_OVER_TELETYPE_MIN_Y_POS-20, 
			    MISSION_OVER_TELETYPE_MAX_X_POS, MISSION_OVER_TELETYPE_MAX_Y_POS-20, 
				STANDARD_TELETYPE_COLOR, STANDARD_TELETYPE_HOLD_TIME, displayTime,
				false, true );
	}

	return M_ PlayTeletype( stringID, 
            MISSION_OVER_TELETYPE_MIN_X_POS, MISSION_OVER_TELETYPE_MIN_Y_POS, 
            MISSION_OVER_TELETYPE_MAX_X_POS, MISSION_OVER_TELETYPE_MAX_Y_POS, 
            STANDARD_TELETYPE_COLOR, STANDARD_TELETYPE_HOLD_TIME, displayTime,
            false, true );
}

U32 TeletypeBriefing( U32 stringID, bool bPerm )
{
	U32 holdtime, displayTime;

    displayTime = MScript::GetScriptStringLength( stringID ) * STANDARD_TELETYPE_MILS_PER_CHAR;

	if (bPerm) 
    {
        holdtime=0; 
    }
    else 
    {
        holdtime = STANDARD_BRIEFING_TELETYPE_HOLD_TIME + displayTime;
    }

	return M_ PlayBriefingTeletype( stringID, 
        STANDARD_TELETYPE_COLOR, holdtime, displayTime, 
        false );
}

//-----------------------------------------------------------------------------
//helper.cpp