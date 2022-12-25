#ifndef GAMETYPES_H
#define GAMETYPES_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               GameTypes.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Sbarton $

    $Header: /Conquest/App/DInclude/GameTypes.h 91    9/08/00 7:14p Sbarton $
*/			    
//--------------------------------------------------------------------------//
// This header includes all of structure definitions needed by ADB.
// All other structures needed at run-time should be included by Data.h. (jy)
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef _ADB
#ifndef NETVECTOR_H
#include "Netvector.h"
#endif
#endif

#ifndef DSPACESHIP_H
#include "DSpaceShip.h"
#endif

#ifndef DPLANET_H
#include "DPlanet.h"
#endif

#ifndef DPLATFORM_H
#include "DPlatform.h"
#endif

#ifndef DSUPPLYSHIP_H
#include "DsupplyShip.h"
#endif

#ifndef DWEAPON_H
#include "DWeapon.h"
#endif

#ifndef DJUMPGATE_H
#include "DJumpGate.h"
#endif

#ifndef DEXPLOSION_H
#include "DExplosion.h"
#endif

#ifndef DEFFECT_H
#include "DEffect.h"
#endif

#ifndef DTURRET_H
#include "DTurret.h"
#endif

#ifndef DVLAUNCH_H
#include "DVLaunch.h"
#endif

#ifndef DWAYPOINT_H
#include "DWaypoint.h"
#endif

#ifndef DMINEFIELD_H
#include "DMinefield.h"
#endif

#ifndef DFIGHTER_H
#include "DFighter.h"
#endif

#ifndef DFIGHTERWING_H
#include "DFighterWing.h"
#endif

#ifndef DAIRDEFENSE_H
#include "DAirDefense.h"
#endif

#ifndef DSHIPLAUNCH_H
#include "DShipLaunch.h"
#endif

#ifndef DFIELD_H
#include "DField.h"
#endif

#ifndef DTRAIL_H
#include "DTrail.h"
#endif

#ifndef DNEBULA_H
#include "DNebula.h"
#endif

#ifndef DSOUNDS_H
#include "DSounds.h"
#endif

#ifndef DFONTS_H
#include "DFonts.h"
#endif

#ifndef DANIMATE_H
#include "DAnimate.h"
#endif

#ifndef DBUTTON_H
#include "DButton.h"
#endif

#ifndef DDIPLOMACYBUTTON_H
#include "DDiplomacyButton.h"
#endif

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

#ifndef DSTATIC_H
#include "DProgressStatic.h"
#endif

#ifndef DTABCONTROL_H
#include "DTabControl.h"
#endif

#ifndef DMENU1_H
#include "DMenu1.h"
#endif

#ifndef DNEWPLAYER_H
#include "DNewPlayer.h"
#endif

#ifndef DEDIT_H
#include "DEdit.h"
#endif

#ifndef DLISTBOX_H
#include "DListbox.h"
#endif

#ifndef DNUGGET_H
#include "DNugget.h"
#endif

#ifndef DTOOLBAR_H
#include "DToolbar.h"
#endif

#ifndef DADMIRALBAR_H
#include "DAdmiralBar.h"
#endif

#ifndef DMENUBRIEFING_H
#include "DMenuBriefing.h"
#endif

#ifndef DMENUOBJECTIVES_H
#include "DMenuObjectives.h"
#endif

#ifndef DCONFIRM_H
#include "DConfirm.h"
#endif

#ifndef DMOVIESCREEN_H
#include "DMovieScreen.h"
#endif

#ifndef DMENUCREDITS_H
#include "DMenu_Credits.h"
#endif

#ifndef DDROPDOWN_H
#include "DDropDown.h"
#endif

#ifndef DPAUSE_H
#include "DPause.h"
#endif

#ifndef DFANCYLAUNCH_H
#include "DFancyLaunch.h"
#endif

#ifndef DCLOAKLAUNCHER_H
#include "DCloakLauncher.h"
#endif

#ifndef DMULTICLOAKLAUNCHER_H
#include "DMultiCloakLauncher.h"
#endif

#ifndef DPINGLAUNCHER_H
#include "DPingLauncher.h"
#endif

#ifndef DWORMHOLELAUNCHER_H
#include "DWormholeLauncher.h"
#endif

#ifndef DJUMPLAUNCHER_H
#include "DJumpLauncher.h"
#endif

#ifndef DARTILERYLAUNCHER_H
#include "DArtileryLauncher.h"
#endif

#ifndef DTRACTORWAVELAUNCHER_H
#include "DTractorWaveLauncher.h"
#endif

#ifndef DBARRAGELAUNCHER_H
#include "DBarrageLauncher.h"
#endif

#ifndef DTALORIANLAUNCHER_H
#include "DTalorianLauncher.h"
#endif

#ifndef DBUFFLAUNCHER_H
#include "DBuffLauncher.h"
#endif

#ifndef DARTIFACTS_H
#include "DArtifacts.h"
#endif

#ifndef DARTIFACTLAUNCHER_H
#include "DArtifactLauncher.h"
#endif

#ifndef DTEMPHQLAUNCHER_H
#include "DTempHQLauncher.h"
#endif

#ifndef DNOVALAUNCHER_H
#include "DNovaLauncher.h"
#endif

#ifndef DMOONRESOURCELAUNCHER_H
#include "DMoonResourceLauncher.h"
#endif

#ifndef DREPAIRLAUNCHER_H
#include "DRepairLauncher.h"
#endif

#ifndef DIGOPTIONS_H
#include "Digoptions.h"
#endif

#ifndef DPLAYERMENU_H
#include "DPlayerMenu.h"
#endif

#ifndef DHOTBUTTON_H
#include "DHotButton.h"
#endif

#ifndef DLOADSAVE_H
#include "DLoadSave.h"
#endif

#ifndef DSYSKITSAVELOAD_H
#include "DSysKitSaveLoad.h"
#endif

#ifndef DGLOBALDATA_H
#include "DGlobalData.h"
#endif

#ifndef DCQGAME_H
#include "DCQGame.h"
#endif

#ifndef DGROUP_H
#include "DGroup.h"
#endif

#ifndef DSCROLLBAR_H
#include "DScrollBar.h"
#endif

#ifndef DLIGHT_H
#include "DLight.h"
#endif

#ifndef DENGINETRAIL_H
#include "DEngineTrail.h"
#endif

#ifndef DHOTSTATIC_H
#include "DHotStatic.h"
#endif

#ifndef DEFFECTOPTS_H
#include "DEffectOpts.h"
#endif

#ifndef DMTECHNODE_H
#include "DMTechNode.h"
#endif

#ifndef DSTRINGPACK_H
#include "DStringPack.h"
#endif

#ifndef DSILHOUETTE_H
#include "DSilhouette.h"
#endif

#ifndef DSHIPSILBUTTON_H
#include "DShipSilButton.h"
#endif

#ifndef DRESEARCH_H
#include "DResearch.h"
#endif

#ifndef DBLACKHOLE_H
#include "DBlackHole.h"
#endif

#ifndef DENDGAME_H
#include "DEndGame.h"
#endif

#ifndef DFLAGSHIP_H
#include "DFlagship.h"
#endif

#ifndef DCOMBOBOX_H
#include "DCombobox.h"
#endif

#ifndef DGUNPLAT_H
#include "DGunplat.h"
#endif

#ifndef DSUPPLYPLAT_H
#include "DSupplyPlat.h"
#endif

#ifndef DBUILDPLAT_H
#include "DBuildPlat.h"
#endif

#ifndef DBUILDSUPPLAT_H
#include "DBuildSupPlat.h"
#endif

#ifndef DREFINEPLAT_H
#include "DRefinePlat.h"
#endif

#ifndef DREPAIRPLAT_H
#include "DRepairPlat.h"
#endif

#ifndef DGENERALPLAT_H
#include "DGeneralPlat.h"
#endif

#ifndef DSELLPLAT_H
#include "DSellPlat.h"
#endif

#ifndef DJUMPPLAT_H
#include "DJumpPlat.h"
#endif

#ifndef DPLAYERBOMB_H
#include "DPlayerBomb.h"
#endif

#ifndef DCHATMENU_H
#include "DChatMenu.h"
#endif

#ifndef DRECONLAUNCH_H
#include "DReconLaunch.h"
#endif

#ifndef DSPECIAL_H
#include "DSpecial.h"
#endif

#ifndef DBUILDOBJ_H
#include "DBuildObj.h"
#endif

#ifndef DMAPGEN_H
#include "DMapGen.h"
#endif

#ifndef DMOVIECAMERA_H
#include "DMovieCamera.h"
#endif

#ifndef DOBJECTGENERATOR_H
#include "DObjectGenerator.h"
#endif

#ifndef DTRIGGER_H
#include "DTrigger.h"
#endif

#ifndef DSCRIPTOBJECT_H
#include "DScriptObject.h"
#endif

#ifndef DQUICKSAVE_H
#include "DQuickSave.h"
#endif

#ifndef DMUSIC_H
#include "DMusic.h"
#endif

#ifndef DSECTOR_H
#include "DSector.h"
#endif

#ifndef DEXTENSION_H
#include "DExtension.h"
#endif

#ifndef DSPECIALDATA_H
#include "DSpecialData.h"
#endif

#endif