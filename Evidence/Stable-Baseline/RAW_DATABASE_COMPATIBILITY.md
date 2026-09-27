# Raw database compatibility follow-up — 2026-09-14

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Research only. No runtime source, installed data, or deployed binaries changed in this pass.
The 2026-09-13 Debug candidate remains the test candidate.

## Two different loading contracts

`Mission::CorrelateSymbol` (Mission.cpp:684) resolves old/new symbols and invokes DocuView.
This is the route exercised by the mission-save conversion tests. Destination record size
differences can be legitimate when fields are mapped by name and missing fields get defaults.

`ObjectList::loadTypesData` (ObjList.cpp:4040) instead opens GameTypes.db, totals file sizes,
allocates one contiguous block, and calls `load_bytes`. Each `objData` begins immediately
after the preceding record's on-disk size (ObjList.cpp:4014). ReadFile copies the record
directly; there is no schema conversion or padding to the current C++ type. GetArchetypeData
(ObjList.cpp:2377) returns that same pointer. FlagshipFactory::CreateArchetype casts it to
BT_FLAGSHIP_DATA (Flagship.cpp:6231).

GenData.cpp's `load_bytes` uses the same raw, consecutive-record arrangement for GenData.db
(around lines 517–524). Do not describe either database as automatically repaired by the
mission parser. Previous discussion of name-based flagship bonus conversion was hypothetical,
not an accurate description of this actual archetype load path.

## Confirmed flagship record-boundary mismatch

The installed GameTypes.db and RetailRef/GameTypes.db have identical SHA-256:
`20A454B8269140D824B3A42CEAC0412FBD31085550275AB4AB8A2C64BF173741`.
Direct UTF-directory inspection found 20 FSHIP records, each exactly 744 bytes:

Hawkes, Halsey, Takei, M_Warlord, S_Counsel, Benson, Steele, Smirnoff,
Mordella, Azkar, VerLak, KerTak, Thripid, Malkor, Blanus, Elan, Vivac,
Joule, Procyo and Natus (full names use the FSHIP!! faction/name prefixes).

The retail parser also describes BT_FLAGSHIP_DATA as 744 bytes. A compile-only x86 probe
using the normal runtime headers (not _ADB) verified these current compiler values:

| Current runtime item | Bytes / offset |
|---|---:|
| sizeof(BT_FLAGSHIP_DATA) | 1384 |
| sizeof(AdmiralBonuses) | 204 |
| attackRadius | 644 |
| admiralBonueses | 648 |
| commandKits | 852 |
| startingFormations | 1300 |
| toolbarInfo | 1364 |
| maxQueueSize | 1380 |

`Flagship::initFlagship` (5427 onward) directly reads startingFormations. That access is
outside a 744-byte record. `GetAvailibleCommandKit` and its ID variant (3987 onward) read
commandKits, also outside the record. The raw allocator places other records after it, so
this can consume neighboring archetype bytes without triggering a heap-boundary fault.
This is a proven record-boundary defect; a current runtime trace of a specific instance
has not been captured. Actual neighboring record identity depends on enumeration order.

Even within the first 744 bytes, interpretations differ: current admiralBonueses starts
where retail stores admiralHotkey; retail bonuses then have different shape and meaning.
Damage, defence, speed, supply, sensor and range consumers are detailed in the parser findings.
Padding to 1384 alone would stop some boundary reads but would not restore correct semantics.

The inspected Blackwell character records are 816 bytes and are not these FSHIP records.
This finding must not be presented as proof of the reported Blackwell inversion or mining
failure. A flagship-specific compatibility issue can coexist with those other defects.

## Confirmed special-ability table mismatch

Installed GenData.db contains SpecialAbilities with on-disk size 400 bytes. Retail schema:
25 SPECIALABILITYINFO entries, 16 bytes each. Retail UNIT_SPECIAL_ABILITY ends with
USA_DOCK=24 and USA_LAST=25. Current code inserts seven sequel abilities before USA_DOCK.

The compile-only probe verifies current SPECIALABILITYINFO=16, GT_SPECIALABILITIES=512,
_GT_SPECIALABILITIES=512, USA_DOCK=31, USA_LAST=32, and transfer offset=496. Retail transfer
is offset 384. The first 24 entries retain their order; this is not evidence that every
special ability is broken.

CMenu_Ind.cpp:509, CMenu_Fleet.cpp:408 and CMenu_MulMix.cpp:561 obtain the raw pointer and
index it by an ability ID. Their assertion compares two current C++ sizes (both 512), not
the 400-byte database record; it cannot detect this mismatch. Index 31 would read bytes
496–511 outside the record. Whether each affected ID reaches a menu in a particular run
still needs runtime/call-site coverage. Menu_AdmiralBar assigns USA_DOCK for flagship
transfer, while ArtifactLauncher also assigns it; JumpLauncher assigns the inserted ID.
These consumers must be audited before changing the enum or simply truncating the table.

## Verification and next implementation boundaries

Run `Evidence\Stable-Baseline\Build-FlagshipLayoutProbe.cmd`. It compiles
FlagshipLayoutProbe.cpp to TEMP with VS2022 x86 and the runtime headers. All layout assertions
passed, exit 0. Initial missing include/context errors were harness setup errors; the final
probe includes the project's globals header and DirectX SDK. Existing header warnings remain.

1. Separate raw-database layout checks from save-schema conversion checks in the baseline gate.
2. Inventory concrete on-disk records against runtime consumer types, not just textual schema
   differences. Require size and relevant field offsets; equal total sizes can conceal shifts.
3. For flagships, choose a retail binary view plus explicit runtime adaptation, or restore the
   retail runtime model. Define bonus/hotkey/toolbar mappings and sequel-only defaults first.
4. For special abilities, preserve retail IDs and bound raw-table accesses; isolate unsupported
   sequel entries. Trace launcher, menu, order/network and save consumers before implementation.
5. Verify with actual retail record bytes and sentinel/guarded adjacent storage, then gameplay.

No speculative fix or deployment was made. These are additional baseline defects, not claims
that the outstanding visual, harvesting or orientation reports are now explained or fixed.
