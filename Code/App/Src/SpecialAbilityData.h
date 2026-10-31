#ifndef SPECIALABILITYDATA_H
#define SPECIALABILITYDATA_H
//--------------------------------------------------------------------------//
// Runtime boundary for the versioned SpecialAbilities GenData table.
//--------------------------------------------------------------------------//

#include "GenData.h"
#include <DSpecialData.h>

// Retail stores 25 records (IDs 0..23 plus DOCK at raw index 24).  The
// surviving CQ2 source stores 32 records and moves DOCK to runtime ID 31.
// Keep that distinction at the raw-data boundary instead of renumbering the
// shared runtime/mission enum.
inline bool ResolveSpecialAbilityDataIndex(UNIT_SPECIAL_ABILITY ability, U32 dataSize, U32 &rawIndex)
{
	const U32 entrySize = sizeof(SPECIALABILITYINFO);
	if ((dataSize % entrySize) != 0)
		return false;

	const U32 recordCount = dataSize / entrySize;
	if (recordCount == 25)
	{
		if ((U32)ability <= (U32)USA_TRACTOR)
		{
			rawIndex = (U32)ability;
			return true;
		}
		if (ability == USA_DOCK)
		{
			rawIndex = 24;
			return true;
		}
		return false;
	}

	if (recordCount == (U32)USA_LAST && (U32)ability < (U32)USA_LAST)
	{
		rawIndex = (U32)ability;
		return true;
	}

	return false;
}

inline const SPECIALABILITYINFO * GetSpecialAbilityData(
	IGeneralData * generalData, UNIT_SPECIAL_ABILITY ability)
{
	if (!generalData)
		return NULL;

	U32 dataSize = 0;
	const SPECIALABILITYINFO * table = (const SPECIALABILITYINFO *)
		generalData->GetArchetypeData("SpecialAbilities", dataSize);
	U32 rawIndex = 0;
	if (!table || !ResolveSpecialAbilityDataIndex(ability, dataSize, rawIndex))
		return NULL;

	return table + rawIndex;
}

#endif
