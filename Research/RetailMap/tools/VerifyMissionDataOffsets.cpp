// Compile-only x86 layout probe for retail-corresponding archetype missionData members.
// This file contains independently stated interoperability facts, not retail code.
#include "pch.h"
#include <globals.h>
#include <TSpaceShip.h>
#include <cstddef>
#include <DSpaceship.h>
#include <DPlanet.h>
#include <DPlatform.h>
#include <DJumpGate.h>
#include <DWaypoint.h>
#include <DMinefield.h>
#include <DField.h>
#include <DNebula.h>
#include <DBlackHole.h>
#include <DPlayerBomb.h>
#include <DSpecial.h>
#include <DMovieCamera.h>
#include <DObjectGenerator.h>
#include <DTrigger.h>
#include <DScriptObject.h>

static_assert(sizeof(MISSION_DATA_BIN) == 40, "MISSION_DATA_BIN prefix size");
static_assert(sizeof(MISSION_DATA) == 72, "MISSION_DATA size");

static_assert(offsetof(BASE_SPACESHIP_DATA, missionData) == 44, "BASE_SPACESHIP_DATA::missionData");
static_assert(offsetof(BT_PLANET_DATA, missionData) == 104, "BT_PLANET_DATA::missionData");
static_assert(offsetof(BASE_PLATFORM_DATA, missionData) == 44, "BASE_PLATFORM_DATA::missionData");
static_assert(offsetof(BT_JUMPGATE_DATA, missionData) == 292, "BT_JUMPGATE_DATA::missionData");
static_assert(offsetof(BT_WAYPOINT, missionData) == 40, "BT_WAYPOINT::missionData");
static_assert(offsetof(BT_MINEFIELD_DATA, missionData) == 8, "BT_MINEFIELD_DATA::missionData");
static_assert(offsetof(BT_ASTEROIDFIELD_DATA, missionData) == 304, "BT_ASTEROIDFIELD_DATA::missionData");
static_assert(offsetof(BT_ANTIMATTER_DATA, missionData) == 156, "BT_ANTIMATTER_DATA::missionData");
static_assert(offsetof(BT_NEBULA_DATA, missionData) == 176, "BT_NEBULA_DATA::missionData");
static_assert(offsetof(BT_BLACKHOLE_DATA, missionData) == 336, "BT_BLACKHOLE_DATA::missionData");
static_assert(offsetof(BT_PLAYERBOMB_DATA, missionData) == 2092, "BT_PLAYERBOMB_DATA::missionData");
static_assert(offsetof(BT_STASISBOLT_DATA, missionData) == 92, "BT_STASISBOLT_DATA::missionData");
static_assert(offsetof(BT_REPELLENTCLOUD_DATA, missionData) == 52, "BT_REPELLENTCLOUD_DATA::missionData");
static_assert(offsetof(BT_REPULSORWAVE_DATA, missionData) == 32, "BT_REPULSORWAVE_DATA::missionData");
static_assert(offsetof(BT_DESTABILIZER_DATA, missionData) == 92, "BT_DESTABILIZER_DATA::missionData");
static_assert(offsetof(BT_DUMBRECONPROBE_DATA, missionData) == 56, "BT_DUMBRECONPROBE_DATA::missionData");
static_assert(offsetof(BT_MOVIE_CAMERA_DATA, missionData) == 40, "BT_MOVIE_CAMERA_DATA::missionData");
static_assert(offsetof(BT_OBJECT_GENERATOR, missionData) == 40, "BT_OBJECT_GENERATOR::missionData");
static_assert(offsetof(BT_TRIGGER, missionData) == 40, "BT_TRIGGER::missionData");
static_assert(offsetof(BT_SCRIPTOBJECT, missionData) == 40, "BT_SCRIPTOBJECT::missionData");

static_assert(offsetof(MISSION_DATA, armorData) == sizeof(MISSION_DATA_BIN),
              "MISSION_DATA_BIN must remain the exact prefix of MISSION_DATA");
