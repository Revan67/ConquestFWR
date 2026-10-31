// Bounded runtime-ID/raw-table mapping probe.  Contains interoperability
// facts only; it does not embed retail data or implementation text.
#include "pch.h"
#include <globals.h>
#include "SpecialAbilityData.h"

static bool maps(UNIT_SPECIAL_ABILITY ability, U32 bytes, U32 expected)
{
	U32 actual = 0xFFFFFFFF;
	return ResolveSpecialAbilityDataIndex(ability, bytes, actual) && actual == expected;
}

static bool rejects(UNIT_SPECIAL_ABILITY ability, U32 bytes)
{
	U32 actual = 0xFFFFFFFF;
	return !ResolveSpecialAbilityDataIndex(ability, bytes, actual);
}

int main()
{
	static_assert(sizeof(SPECIALABILITYINFO) == 16, "ability record size");
	static_assert(USA_TRACTOR == 23, "last shared retail/current ability ID");
	static_assert(USA_DOCK == 31, "current runtime dock ID");
	static_assert(USA_LAST == 32, "current runtime ability count");

	const U32 retailBytes = 25 * sizeof(SPECIALABILITYINFO);
	const U32 currentBytes = USA_LAST * sizeof(SPECIALABILITYINFO);

	if (!maps(USA_NONE, retailBytes, 0) ||
		!maps(USA_TRACTOR, retailBytes, 23) ||
		!maps(USA_DOCK, retailBytes, 24))
		return 1;

	for (U32 value = 24; value <= 30; ++value)
		if (!rejects((UNIT_SPECIAL_ABILITY)value, retailBytes))
			return 2;

	for (U32 value = 0; value < USA_LAST; ++value)
		if (!maps((UNIT_SPECIAL_ABILITY)value, currentBytes, value))
			return 3;

	if (!rejects(USA_DOCK, 24 * sizeof(SPECIALABILITYINFO)) ||
		!rejects(USA_DOCK, 26 * sizeof(SPECIALABILITYINFO)) ||
		!rejects(USA_DOCK, retailBytes + 1))
		return 4;

	return 0;
}
