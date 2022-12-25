#ifndef UNITCOMM_H
#define UNITCOMM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               UnitComm.h                                //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/UnitComm.h 33    6/25/01 4:16p Tmauer $
*/
//--------------------------------------------------------------------------//
//
/*
	Documentation would go here.			
*/

#ifndef DACOM_H
#include <DACOM.h>
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#define COMMF_ALERT		0x00000001
#define COMMF_DEATH		0x00000002
#define COMMF_SELECT	0x00000004
#define COMMF_NOHILITE  0x00000008		
#define COMMF_ENEMY		0x00000010		// receive message from enemy unit

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
// alert strings
#define NUM_ALERT_STRINGS 19
#define ALERT_UNKNOWN 0
#define ALERT_UNDER_ATTACK 1
#define ALERT_ENEMY_SIGHTED 2
#define ALERT_OUT_SUPPLY 3
#define ALERT_UNIT_BUILT 4
#define ALERT_PLATFORM_BUILD 5
#define ALERT_RESOURCE_EXAHAUSTED 6
#define ALERT_RESEARCH_COMPLETE 7
#define ALERT_MUTATION_COMPLETE 8
#define ALERT_ADMIRAL_LOST 9
#define ALERT_ADMIRAL_MESSAGE 10
#define ALERT_ALLIED_ATTACK 11
#define ALERT_MISSION 12
#define NOVA_WARNING_30 13
#define NOVA_WARNING_25 14
#define NOVA_WARNING_20 15
#define NOVA_WARNING_15 16
#define NOVA_WARNING_10 17
#define NOVA_WARNING_5 18


#define SUB_FIRST						4500

#define SUB_DEATH						IDS_SUB_DEATH
#define SUB_ENEMYSIGHTED				IDS_SUB_ENEMY_SIGHTED
#define SUB_UNDERATTACK					IDS_SUB_UNDERATTACK
#define SUB_NOSUPPLIES					IDS_SUB_NOSUPPLIES
#define SUB_MOVE						IDS_SUB_MOVE
#define SUB_DENIED						IDS_SUB_DENIED
#define SUB_SPECIAL_ATTACK				IDS_SUB_SPECIAL_ATTACK
#define SUB_ATTACK						IDS_SUB_ATTACK
#define SUB_HAR_DEPLETED				IDS_SUB_HAR_DEPLETED
#define SUB_NO_CP						IDS_SUB_NO_CP
#define SUB_NO_GAS						IDS_SUB_NO_GAS
#define SUB_NO_CREW						IDS_SUB_NO_CREW
#define SUB_NO_METAL					IDS_SUB_NO_METAL
#define SUB_NO_BUILD					IDS_SUB_NO_BUILD
#define SUB_ADMIRAL_ON_DECK				IDS_SUB_ADMIRAL_ON_DECK
#define SUB_ADMIRAL_OFF_DECK			IDS_SUB_ADMIRAL_OFF_DECK
#define SUB_ADMIRAL_TRANS_FAIL			IDS_SUB_ADMIRAL_TRANS_FAIL
#define SUB_ADMIRAL_FLAGSHIP_DANGER		IDS_SUB_ADMIRAL_FLAG_DANGER
#define SUB_FLEET_75_DAMAGE				IDS_SUB_FLEET_75_DAMAGE
#define SUB_FLEET_50_DAMAGE				IDS_SUB_FLEET_50_DAMAGE
#define SUB_FLEET_LOW_SUPPLIES			IDS_SUB_FLEET_LOW_SUPPLIES
#define SUB_BATTLE_GOOD					IDS_SUB_BATTLE_GOOD
#define SUB_BATTLE_BLEAK				IDS_SUB_BATTLE_BLEAK
#define SUB_BATTLE_MODERATE				IDS_SUB_BATTLE_MODERATE
#define SUB_SCUTTLE						IDS_SUB_SCUTTLE
#define SUB_RESUPPLY					IDS_SUB_RESUPPLY
#define SUB_SELECT						IDS_SUB_SELECT
#define SUB_CONSTRUCTION_COMP			IDS_SUB_CONSTRUCTION_COMP
#define SUB_ALLIED_ATTACK				IDS_SUB_ALLIED_ATTACK
	
#define SUB_LAST						4600


struct IUnitComm : IDAComponent
{
	// plays comm message if current player is allied with object
	virtual void PlayCommMessage (IBaseObject * obj, U32 dwMissionID, U32 indexPos, U32 subtitle, U32 displayName, U32 flags=0,DWORD param = 0) = 0;

	virtual void PlayCommMessage (IBaseObject * obj, U32 dwMissionID, char * fileName, U32 subtitle, U32 displayName, U32 flags=0,DWORD param = 0) = 0;
};

//------------------------------
// helper macros
//------------------------------


#define SHIPCOMM(message,subtitle) \
	UNITCOMM->PlayCommMessage(this, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::SPACESHIP *)0)->message))+0)) / sizeof(M_STRING)), subtitle,pInitData->displayName )

#define SHIPCOMMDEATH(value) \
	UNITCOMM->PlayCommMessage(this, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::SPACESHIP *)0)->death))+0)) / sizeof(M_STRING)), SUB_DEATH,pInitData->displayName, value ? COMMF_DEATH|COMMF_ALERT : COMMF_DEATH,value ? 0:ALERT_ADMIRAL_LOST)

#define SHIPCOMM2(obj, dwMissionID, message, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::SPACESHIP *)0)->message))+0)) / sizeof(M_STRING)), subtitle,displayName )

#define GUNBOATCOMM(message,subtitle) \
	UNITCOMM->PlayCommMessage(this, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::GUNBOAT *)0)->message))+0)) / sizeof(M_STRING)), subtitle,pInitData->displayName )

#define GUNBOATCOMM2(obj, dwMissionID, message, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::GUNBOAT *)0)->message))+0)) / sizeof(M_STRING)), subtitle,displayName )

#define GUNBOATNOHILITECOMM(obj, dwMissionID, message, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::GUNBOAT *)0)->message))+0)) / sizeof(M_STRING)), subtitle,displayName, COMMF_NOHILITE)

#define HARVESTERCOMM(obj, dwMissionID, message, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(this, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::HARVESTER *)0)->message))+0)) / sizeof(M_STRING)) , subtitle,displayName)

#define FABRICATORCOMM(message,subtitle) \
	UNITCOMM->PlayCommMessage(this, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::FABRICATOR *)0)->message))+0)) / sizeof(M_STRING)), subtitle,pInitData->displayName )

#define FABRICATORCOMM2(obj, dwMissionID, message, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::FABRICATOR *)0)->message))+0)) / sizeof(M_STRING)), subtitle,displayName )

#define MINELAYERCOMM(message,subtitle) \
	UNITCOMM->PlayCommMessage(this, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::MINELAYER *)0)->message))+0)) / sizeof(M_STRING)), subtitle,pInitData->displayName )

#define SUPPLYSHIPCOMM(message,subtitle) \
	UNITCOMM->PlayCommMessage(this, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::SUPPLYSHIP *)0)->message))+0)) / sizeof(M_STRING)), subtitle,pInitData->displayName )

#define TROOPSHIPCOMM(message,subtitle) \
	UNITCOMM->PlayCommMessage(this, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::TROOPSHIP *)0)->message))+0)) / sizeof(M_STRING)), subtitle,pInitData->displayName )

#define TROOPSHIPCOMM2(obj, dwMissionID, message, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::TROOPSHIP *)0)->message))+0)) / sizeof(M_STRING)), subtitle,displayName )

#define PLATFORMCOMM(message,subtitle) \
	UNITCOMM->PlayCommMessage(this, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::PLATFORM *)0)->message))+0)) / sizeof(M_STRING)), subtitle,pInitData->displayName )

#define PLATFORMCOMM2(obj, dwMissionID, message, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::PLATFORM *)0)->message))+0)) / sizeof(M_STRING)), subtitle,displayName  )

#define GENPLATFORMCOMM(message,subtitle) \
	UNITCOMM->PlayCommMessage(this, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::GENPLATFORM *)0)->message))+0)) / sizeof(M_STRING)), subtitle,pInitData->displayName)

#define FLAGSHIPCOMM(message,subtitle) \
	UNITCOMM->PlayCommMessage(this, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::FLAGSHIP *)0)->message))+0)) / sizeof(M_STRING)), subtitle,pInitData->displayName )

#define FLAGSHIPCOMM2(obj, dwMissionID, message, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::FLAGSHIP *)0)->message))+0)) / sizeof(M_STRING)), subtitle,displayName  )

#define FLAGSHIPALERT(message,subtitle) \
	UNITCOMM->PlayCommMessage(this, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::FLAGSHIP *)0)->message))+0)) / sizeof(M_STRING)), subtitle,pInitData->displayName, COMMF_ALERT,ALERT_ADMIRAL_MESSAGE )

#define FLAGSHIPSCUTTLE(obj, dwMissionID, message, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::FLAGSHIP *)0)->message))+0)) / sizeof(M_STRING)), subtitle,displayName, COMMF_ENEMY)

#define FLEETSHIPCOMM(x)   \
		if (admiralID)	   \
		{					\
			IBaseObject * obj = OBJLIST->FindObject(admiralID);	\
			if (obj)	\
				SHIPCOMM2(obj, dwMissionID, x,subtitle,pInitData->displayName);	\
			else	\
				SHIPCOMM(x,subtitle);	\
		}  \
		else				\
			SHIPCOMM(x,subtitle)

#define SUPPLYSHIPCOMM2(obj, dwMissionID, message, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::SUPPLYSHIP *)0)->message))+0)) / sizeof(M_STRING)),subtitle,displayName )

#define SHIPSELECT(obj, dwMissionID, message, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::SPACESHIP *)0)->message))+0)) / sizeof(M_STRING)),subtitle,displayName, COMMF_SELECT|COMMF_NOHILITE)

#define SHIPNOHILITECOMM(obj, dwMissionID, message, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::SPACESHIP *)0)->message))+0)) / sizeof(M_STRING)),subtitle,displayName, COMMF_NOHILITE)

#define PLATSELECT(obj, dwMissionID, message, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::PLATFORM *)0)->message))+0)) / sizeof(M_STRING)),subtitle,displayName, COMMF_SELECT|COMMF_NOHILITE)

#define ENEMYSIGHTED(obj, dwMissionID, displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::PLATFORM *)0)->enemySighted))+0)) / sizeof(M_STRING)), SUB_ENEMYSIGHTED,displayName , COMMF_ALERT|COMMF_NOHILITE, ALERT_ENEMY_SIGHTED)

#define UNDERATTACK(obj, dwMissionID, displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::PLATFORM *)0)->underAttack))+0)) / sizeof(M_STRING)), SUB_UNDERATTACK,displayName , COMMF_ALERT|COMMF_NOHILITE, ALERT_UNDER_ATTACK)

#define FLEETSHIP_UNDERATTACK \
		if (admiralID)	   \
		{					\
			IBaseObject * obj = OBJLIST->FindObject(admiralID);	\
			if (obj)	\
				UNDERATTACK(obj, dwMissionID, pInitData->displayName);	\
			else	\
				UNDERATTACK(this,dwMissionID, pInitData->displayName);	\
		}  \
		else				\
			UNDERATTACK(this, dwMissionID, pInitData->displayName)

#define FLAGSHIP_NOSUPPLIES \
	UNITCOMM->PlayCommMessage(this, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::FLAGSHIP *)0)->suppliesout))+0)) / sizeof(M_STRING)),SUB_NOSUPPLIES, pInitData->displayName, COMMF_ALERT, ALERT_OUT_SUPPLY)

#define GUNBOAT_ALERT(obj, dwMissionID, message, subtitle,displayName, alertType) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::GUNBOAT *)0)->message))+0)) / sizeof(M_STRING)), subtitle,displayName, COMMF_ALERT, alertType)

#define GUNPLAT_ALERT(obj, dwMissionID, message, subtitle,displayName, alertType) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::GUNPLAT *)0)->message))+0)) / sizeof(M_STRING)), subtitle,displayName, COMMF_ALERT, alertType)

#define PLATFORM_ALERT(obj, dwMissionID, message, subtitle,displayName, alertType) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::PLATFORM *)0)->message))+0)) / sizeof(M_STRING)), subtitle,displayName, COMMF_ALERT, alertType)

#define SPACESHIP_ALERT(obj, dwMissionID, message, subtitle,displayName, alertType) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::SPACESHIP *)0)->message))+0)) / sizeof(M_STRING)), subtitle,displayName, COMMF_ALERT, alertType)

#define COMM_RESEARCH_COMPLETED(fileName, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(this, dwMissionID,fileName, subtitle,displayName,COMMF_ALERT, ALERT_RESEARCH_COMPLETE);

#define COMM_MUTATION_COMPLETED(fileName, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(this, dwMissionID,fileName, subtitle,displayName,COMMF_ALERT, ALERT_MUTATION_COMPLETE);

#define COMM_ALLIED_ATTACK(obj, dwMissionID, subtitle,displayName) \
	UNITCOMM->PlayCommMessage(obj, dwMissionID, (((size_t)((&(((UNITSOUNDS::SPEECH::SPACESHIP *)0)->alliedAttack))+0)) / sizeof(M_STRING)), subtitle,displayName, COMMF_ALERT, ALERT_ALLIED_ATTACK)

#define COMM_MISSION_ALLERT(target,dwMissionID, fileName) \
	UNITCOMM->PlayCommMessage(target, dwMissionID,fileName,0,0,COMMF_ALERT|COMMF_ENEMY, ALERT_MISSION);

#define COMM_NOVA_ALLERT(target,dwMissionID, fileName, warning) \
	UNITCOMM->PlayCommMessage(target, dwMissionID,fileName,0,0,COMMF_ALERT|COMMF_ENEMY, warning);

//----------------------------------------------------------------------------//
//-----------------------------END UnitComm.h--------------------------------//
//----------------------------------------------------------------------------//
#endif