"""
Parse gendata.db (UTF virtual filesystem) and inspect archetype binary blobs.

UTF_HEADER layout (56 bytes):
  DWORD dwIdentifier       'UTF ' = 0x20465455
  DWORD dwVersion
  DWORD dwDirectoryOffset
  DWORD dwDirectorySize
  DWORD dwUnusedEntryOffset
  DWORD dwDirEntrySize
  DWORD dwNamesOffset
  DWORD dwNameSpaceSize
  DWORD dwNameSpaceUsed
  DWORD dwDataStartOffset
  DWORD dwUnusedSpaceOffset
  DWORD dwUnusedSpaceSize
  FILETIME LastWriteTime   (8 bytes)

UTF_DIR_ENTRY layout (dwDirEntrySize bytes, typically 44):
  DWORD dwNext
  DWORD dwName             offset into names buffer
  DWORD dwAttributes       FILE_ATTRIBUTE_DIRECTORY=0x10, FILE_ATTRIBUTE_UNUSED=0xFFFFFFFF
  DWORD Sharing            (4 bytes)
  DWORD dwDataOffset
  DWORD dwSpaceAllocated
  DWORD dwSpaceUsed
  DWORD dwUncompressedSize
  DWORD DOSCreationTime
  DWORD DOSLastAccessTime
  DWORD DOSLastWriteTime
"""

import struct
import sys
import os

GENDATA_PATH = r"E:\GitHub\projects\ConquestFWR\Conquest Frontier Wars\gendata.db"

UTF_IDENT = 0x20465455  # 'UTF '
FILE_ATTRIBUTE_DIRECTORY = 0x10
FILE_ATTRIBUTE_UNUSED    = 0xFFFFFFFF

def read_u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]

def read_cstr(data, offset):
    end = data.index(b'\x00', offset)
    return data[offset:end].decode('ascii', errors='replace')

def parse_header(data):
    ident = read_u32(data, 0)
    assert ident == UTF_IDENT, f"Bad ident: {ident:#010x}"
    version          = read_u32(data, 4)
    dir_offset       = read_u32(data, 8)
    dir_size         = read_u32(data, 12)
    unused_entry_off = read_u32(data, 16)
    entry_size       = read_u32(data, 20)
    names_offset     = read_u32(data, 24)
    name_space_size  = read_u32(data, 28)
    name_space_used  = read_u32(data, 32)
    data_start       = read_u32(data, 36)
    return {
        'version':     version,
        'dir_offset':  dir_offset,
        'dir_size':    dir_size,
        'entry_size':  entry_size,
        'names_offset': names_offset,
        'data_start':  data_start,
    }

def iter_entries(data, hdr, abs_entry_offset, depth=0, _visited=None):
    """
    Yield (name, abs_data_offset, data_size, is_dir, depth).

    dwNext and directory dwDataOffset are offsets from hdr['dir_offset']
    (the start of the directory area), confirmed from BaseUTF.cpp:1263/1292:
      pEntry = (UTF_DIR_ENTRY*)(((char*)pDirectory) + pEntry->dwDataOffset/dwNext)

    File dwDataOffset is from hdr['data_start']:
      GetStartOffset = entry->dwDataOffset + dwDataStartOffset   (UTF.cpp:224)
    """
    if _visited is None:
        _visited = set()
    dir_base = hdr['dir_offset']
    offset   = abs_entry_offset
    while offset != 0:
        if offset in _visited:
            break
        _visited.add(offset)
        attrs      = read_u32(data, offset + 8)
        if attrs == FILE_ATTRIBUTE_UNUSED:
            break
        name_off   = read_u32(data, offset + 4)
        data_off   = read_u32(data, offset + 16)
        space_used = read_u32(data, offset + 24)
        next_rel   = read_u32(data, offset + 0)   # relative to dir_base
        name = read_cstr(data, hdr['names_offset'] + name_off)
        is_dir = bool(attrs & FILE_ATTRIBUTE_DIRECTORY)
        if is_dir:
            child_abs = dir_base + data_off
            if data_off != 0 and child_abs not in _visited:
                yield (name, child_abs, 0, True, depth)
                yield from iter_entries(data, hdr, child_abs, depth + 1, _visited)
        else:
            yield (name, hdr['data_start'] + data_off, space_used, False, depth)
        if next_rel == 0:
            break
        offset = dir_base + next_rel

def find_archetype(data, hdr, target_name):
    """Return (abs_data_offset, size) for an archetype file, or None."""
    for name, data_off, size, is_dir, depth in iter_entries(data, hdr, hdr['dir_offset']):
        if not is_dir and name.lower() == target_name.lower():
            return (data_off, size)
    return None

def hexdump(data, offset, length, label=""):
    if label:
        print(f"\n=== {label} (offset {offset:#010x}, {length} bytes) ===")
    for i in range(0, length, 16):
        chunk = data[offset + i : offset + i + 16]
        hex_part = ' '.join(f'{b:02x}' for b in chunk)
        asc_part = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
        print(f"  {offset+i:08x}  {hex_part:<48}  |{asc_part}|")

def read_u32_at(data, offset):
    return struct.unpack_from("<I", data, offset)[0]

def read_s32_at(data, offset):
    return struct.unpack_from("<i", data, offset)[0]

def read_f32_at(data, offset):
    return struct.unpack_from("<f", data, offset)[0]

def read_str_at(data, offset, length):
    chunk = data[offset:offset+length]
    end = chunk.find(b'\x00')
    if end >= 0:
        chunk = chunk[:end]
    return chunk.decode('ascii', errors='replace')

# ---- GENBASE_DATA ----
# struct GENBASE_DATA {
#   U32 type;   // actually char type[4] or U32 -- the "genbase type" enum stored as U32
#   char name[32];
#   ... rest is struct-specific
# }
# Wait, looking at DToolbar.h/data.i, GENBASE_DATA has:
#   char type[?];  // type string e.g. "Toolbar!!ContextFont"
# Let me check what GENBASE_DATA actually looks like.

def inspect_genbase(data, abs_offset, struct_name, total_size):
    """Print the GENBASE_DATA header and key fields."""
    print(f"\n--- {struct_name} @ abs offset {abs_offset:#010x}, total {total_size} bytes ---")
    # GENBASE_DATA: first field is 'type' - a type string (look for '!!' pattern)
    # Typically starts with a string like "Toolbar!!ContextFont"
    type_str = read_str_at(data, abs_offset, 32)
    print(f"  type[0..31]:  {repr(type_str)}")
    hexdump(data, abs_offset, min(total_size, 128), struct_name)

def dump_dir_entries_flat(data, hdr, max_entries=200):
    """Dump all directory entries flat (no recursion) to understand raw structure."""
    dir_base  = hdr['dir_offset']
    dir_size  = hdr['dir_size'] if 'dir_size' in hdr else 65536
    es        = hdr['entry_size']
    print(f"\n=== Raw directory entries (dir_base={dir_base:#010x}, entry_size={es}) ===")
    print(f"{'idx':>4}  {'abs_off':>10}  {'next':>8}  {'name_off':>8}  {'attrs':>10}  {'data_off':>8}  {'used':>8}  name")
    offset = dir_base
    for i in range(max_entries):
        attrs    = read_u32(data, offset + 8)
        if offset + es > len(data):
            break
        next_rel = read_u32(data, offset + 0)
        name_off = read_u32(data, offset + 4)
        data_off = read_u32(data, offset + 16)
        sp_used  = read_u32(data, offset + 24)
        try:
            name = read_cstr(data, hdr['names_offset'] + name_off) if name_off < 65536 else "?"
        except Exception:
            name = "?"
        is_dir = bool(attrs & FILE_ATTRIBUTE_DIRECTORY)
        unused = (attrs == FILE_ATTRIBUTE_UNUSED)
        kind = "DIR" if is_dir else ("---" if unused else "file")
        print(f"  {i:>4}  {offset:#010x}  {next_rel:>8x}  {name_off:>8x}  {attrs:>10x}  {data_off:>8x}  {sp_used:>8}  {name!r}")
        offset += es
        if i > 0 and offset >= dir_base + hdr.get('dir_size', 65536):
            break

def list_all_archetypes(data, hdr):
    print("=== All archetypes in gendata.db ===")
    for name, data_off, size, is_dir, depth in iter_entries(data, hdr, hdr['dir_offset']):
        indent = "  " * depth
        kind = "DIR " if is_dir else f"{size:6d}B"
        print(f"  {indent}{kind}  {name}")

def inspect_toolbar_struct(data, abs_offset, total_size, struct_name):
    """
    Walk a toolbar struct blob looking for TAB/HOTBUTTON counts.
    GT_TOOLBAR COMMON layout (retail, from data.i):
      char type[32]                   -- "Toolbar!!ContextFont"
      char name[32]
      TABCONTROL_DATA tabCtrl         -- size 24
        char type[32] = "TabControl!!ContextFont"
        S32 numTabs
        S32 tabX, tabY, tabW, tabH
      ... tabs follow
    """
    print(f"\n=== Toolbar struct: {struct_name} @ {abs_offset:#010x}, {total_size} bytes ===")

    off = abs_offset

    # GENBASE_DATA fields (from DGenData.h / DToolbar.h)
    # First 32 bytes: type string
    # Next 32 bytes: name string
    type_str  = read_str_at(data, off,      32)
    name_str  = read_str_at(data, off + 32, 32)
    print(f"  GENBASE type: {repr(type_str)}")
    print(f"  GENBASE name: {repr(name_str)}")

    hexdump(data, abs_offset, min(total_size, 256))

def verify_toolbar_layout(data, blob_start, blob_size):
    """
    Verify GT_TOOLBAR binary layout for the retail Toolbar!!Default blob.

    Struct sizes (from static_asserts):
      HOTBUTTON_DATA    = 32
      BUILDBUTTON_DATA  = 104
      RESEARCHBUTTON_DATA = 64
      EDIT_DATA         = 44
      ICON_DATA         = 16
      STATIC_DATA       = 64
      TABCONTROL_DATA   = 108
      QUEUECONTROL_DATA = 16 (4 x S32)
      M_OBJCLASS        = 4 (U32 enum)

    GT_TOOLBAR layout:
      [0]   char vfxShapeType[32]    = 32
      [32]  char vfxToolBar[3][32]   = 96
      [128] RECT contextRect         = 16
      [144] RECT sysmapRect[3]       = 48
      [192] RECT sectorMapRect[3]    = 48
      [240] U32 topBarX, topBarY     = 8
      --- header total = 248 ---
      [248] COMMON[3]  (3 x 864)     = 2592
             COMMON = 16*HOTBUTTON_DATA + 5*STATIC_DATA + 2*ICON_DATA
                    = 512 + 320 + 32 = 864
      [2840] NONE (1 x STATIC_DATA)  = 64
      [2904] FABRICATOR x 4          = 4 x 6972 = 27888
             FABRICATOR = M_OBJCLASS(4) + 3*HOTBUTTON_DATA(96) + EDIT_DATA(44)
                        + STATIC_DATA(64) + TABCONTROL_DATA(108)
                        + 4 tabs x 16*BUILDBUTTON_DATA(1664) = 6972
      [30792] LINDUSTRIAL x 12       = 12 x 1524 (WITHOUT QC) or 12 x 1540 (WITH QC)
              LINDUSTRIAL = M_OBJCLASS(4) + 2*ICON_DATA(32) + 2*HOTBUTTON_DATA(64)
                          + 6*STATIC_DATA(384) + 10*BUILDBUTTON_DATA(1040) = 1524
    """
    SZ_HOTBUTTON     = 32
    SZ_BUILDBUTTON   = 104
    SZ_RESEARCHBUTTON= 64
    SZ_EDIT          = 44
    SZ_ICON          = 16
    SZ_STATIC        = 64
    SZ_TABCONTROL    = 108
    SZ_QUEUECONTROL  = 16
    SZ_OBJCLASS      = 4

    # Header
    HDR_SIZE = 32 + 96 + 16 + 48 + 48 + 8  # = 248

    # COMMON
    COMMON_HOTBTNS  = 16
    COMMON_STATICS  = 5
    COMMON_ICONS    = 2
    SZ_COMMON = COMMON_HOTBTNS*SZ_HOTBUTTON + COMMON_STATICS*SZ_STATIC + COMMON_ICONS*SZ_ICON
    # = 512 + 320 + 32 = 864
    SZ_COMMON3 = 3 * SZ_COMMON  # = 2592

    # NONE (1 x STATIC_DATA)
    SZ_NONE = SZ_STATIC  # = 64

    # FABRICATOR
    SZ_FAB_HEADER = SZ_OBJCLASS + 3*SZ_HOTBUTTON + SZ_EDIT + SZ_STATIC + SZ_TABCONTROL
    # = 4 + 96 + 44 + 64 + 108 = 316
    SZ_FAB_TABS = 4 * 16 * SZ_BUILDBUTTON  # 4 tabs * 16 buttons * 104 = 6656
    SZ_FABRICATOR = SZ_FAB_HEADER + SZ_FAB_TABS  # = 6972

    # LINDUSTRIAL (without QUEUECONTROL_DATA)
    SZ_LIND_NO_QC = SZ_OBJCLASS + 2*SZ_ICON + 2*SZ_HOTBUTTON + 6*SZ_STATIC + 10*SZ_BUILDBUTTON
    # = 4 + 32 + 64 + 384 + 1040 = 1524
    SZ_LINDUSTRIAL_WITH_QC = SZ_LIND_NO_QC + SZ_QUEUECONTROL  # = 1540

    # Key offsets within the blob
    off_fabricator_0 = HDR_SIZE + SZ_COMMON3 + SZ_NONE
    off_fab_tab_ctrl = off_fabricator_0 + SZ_FAB_HEADER - SZ_TABCONTROL  # tabCtrl is last in header
    off_fab_numTabs  = off_fab_tab_ctrl + 64  # numTabs at offset 64 within TABCONTROL_DATA
                                               # (tabControlType[32] + hotButtonType[32] + iBaseImage[4] = 68, so numTabs is at 68)
    # Wait: tabControlType[32] + hotButtonType[32] + iBaseImage[4] = 68, numTabs at 68
    off_fab_numTabs  = off_fab_tab_ctrl + 68  # corrected

    off_lindustrial_0 = off_fabricator_0 + 4 * SZ_FABRICATOR

    print(f"\n=== GT_TOOLBAR layout verification ===")
    print(f"Blob: [{blob_start:#010x}, {blob_start+blob_size:#010x}), {blob_size} bytes")
    print(f"  header_end        = {HDR_SIZE}")
    print(f"  common[3] end     = {HDR_SIZE + SZ_COMMON3}")
    print(f"  none end          = {HDR_SIZE + SZ_COMMON3 + SZ_NONE}")
    print(f"  fabricator[0]     = {off_fabricator_0} (= 0x{off_fabricator_0:x})")
    print(f"  SZ_FABRICATOR     = {SZ_FABRICATOR}")
    print(f"  fabTab offset     = {off_fab_tab_ctrl} within blob")
    print(f"  fabTab numTabs    = blob[{off_fab_numTabs}]")
    print(f"  lindustrial[0]    = {off_lindustrial_0} (= 0x{off_lindustrial_0:x})")
    print(f"  SZ_LINDUSTRIAL    = {SZ_LIND_NO_QC} (no QC) / {SZ_LINDUSTRIAL_WITH_QC} (with QC)")

    # --- Check 1: GT_TOOLBAR header strings ---
    vfx_type = read_str_at(data, blob_start, 32)
    print(f"\n[1] vfxShapeType[32]  = {vfx_type!r}")
    for i in range(3):
        s = read_str_at(data, blob_start + 32 + i*32, 32)
        print(f"    vfxToolBar[{i}][32] = {s!r}")

    # --- Check 2: FABRICATOR's fabTab ---
    fab_tab_abs = blob_start + off_fab_tab_ctrl
    tab_type_str = read_str_at(data, fab_tab_abs, 32)
    tab_hbtype   = read_str_at(data, fab_tab_abs + 32, 32)
    tab_base_img = read_s32_at(data, fab_tab_abs + 64)
    tab_numTabs  = read_s32_at(data, fab_tab_abs + 68)
    print(f"\n[2] FABRICATOR fabTab @ blob+{off_fab_tab_ctrl} (file {fab_tab_abs:#010x}):")
    print(f"    tabControlType = {tab_type_str!r}")
    print(f"    hotButtonType  = {tab_hbtype!r}")
    print(f"    iBaseImage     = {tab_base_img}")
    print(f"    numTabs        = {tab_numTabs}  (expect 4)")
    hexdump(data, fab_tab_abs, 108, "TABCONTROL_DATA")

    # --- Check 3: LINDUSTRIAL[0] at both possible positions ---
    lind_abs = blob_start + off_lindustrial_0
    lind_type = read_u32_at(data, lind_abs)
    print(f"\n[3] LINDUSTRIAL[0] @ blob+{off_lindustrial_0} (file {lind_abs:#010x}):")
    print(f"    M_OBJCLASS type = {lind_type} ({lind_type:#010x})")

    # At blob offset +484 within lindustrial: either QC xOrigin or build0 baseImage
    off_qc_or_build = 4 + 2*SZ_ICON + 2*SZ_HOTBUTTON + 6*SZ_STATIC  # = 484
    qc_abs = lind_abs + off_qc_or_build
    print(f"\n    At lindustrial+{off_qc_or_build} (file {qc_abs:#010x}):")
    print(f"    If QUEUECONTROL_DATA present: xOrigin={read_s32_at(data,qc_abs)}, "
          f"yOrigin={read_s32_at(data,qc_abs+4)}, "
          f"w={read_s32_at(data,qc_abs+8)}, h={read_s32_at(data,qc_abs+12)}")
    print(f"    If BUILDBUTTON build0:        baseImage={read_u32_at(data,qc_abs)}, "
          f"noMoneyImg={read_u32_at(data,qc_abs+4)}, "
          f"xOrigin={read_s32_at(data,qc_abs+8)}, yOrigin={read_s32_at(data,qc_abs+12)}")
    build0_rtArch_at484 = read_str_at(data, qc_abs + 8, 32)   # rtArchetype if NO QC
    build0_rtArch_at500 = read_str_at(data, qc_abs + 24, 32)  # rtArchetype if WITH QC
    print(f"    rtArchetype if NO QC  (+8):   {build0_rtArch_at484!r}")
    print(f"    rtArchetype if WITH QC (+24): {build0_rtArch_at500!r}")
    hexdump(data, qc_abs, 32, "lindustrial+484 (QC vs build0)")

    # --- Check 4: Look at the LINDUSTRIAL+1524 boundary (next instance if no QC) ---
    next_no_qc  = lind_abs + SZ_LIND_NO_QC
    next_with_qc = lind_abs + SZ_LINDUSTRIAL_WITH_QC
    next_no_qc_type  = read_u32_at(data, next_no_qc)
    next_with_qc_type = read_u32_at(data, next_with_qc)
    print(f"\n[4] Next instance check:")
    print(f"    lindustrial+1524 (no QC)  M_OBJCLASS = {next_no_qc_type} ({next_no_qc_type:#010x})  @ {next_no_qc:#010x}")
    print(f"    lindustrial+1540 (with QC) M_OBJCLASS = {next_with_qc_type} ({next_with_qc_type:#010x})  @ {next_with_qc:#010x}")
    print(f"    (both should be same M_OBJCLASS if our schema matches the binary)")
    hexdump(data, next_no_qc - 16, 48, "boundary region (lindustrial+1508..1555)")


def main():
    print(f"Reading {GENDATA_PATH}")
    with open(GENDATA_PATH, 'rb') as f:
        data = f.read()
    print(f"File size: {len(data):,} bytes")

    hdr = parse_header(data)
    print(f"UTF version: {hdr['version']:#06x}, entry_size: {hdr['entry_size']}")
    print(f"dir_offset: {hdr['dir_offset']:#010x}, names_offset: {hdr['names_offset']:#010x}, data_start: {hdr['data_start']:#010x}")

    # Check gametypes.db -- also a UTF file
    GT_PATH = r"D:\SteamLibrary\steamapps\common\Conquest Frontier Wars\gametypes.db"
    with open(GT_PATH, 'rb') as f:
        gtdata = f.read()
    print(f"\ngametypes.db size: {len(gtdata):,} bytes")

    # Is it a UTF file?
    gt_ident = read_u32(gtdata, 0)
    if gt_ident == UTF_IDENT:
        print("gametypes.db is a UTF file")
        gthdr = parse_header(gtdata)
        print(f"  dir_offset={gthdr['dir_offset']:#010x}, names_offset={gthdr['names_offset']:#010x}, "
              f"data_start={gthdr['data_start']:#010x}, entry_size={gthdr['entry_size']}")
        gt_names = gtdata[gthdr['names_offset'] : gthdr['dir_offset']]
        print(f"  Names buffer: {len(gt_names)} bytes")

        # Search names buffer for toolbar archetypes
        for t in ['LINDUSTRIAL', 'FABRICATOR', 'RESEARCH', 'BUILD_RES', 'FLEET',
                  'Toolbar!!']:
            idx = gt_names.find(t.encode('ascii'))
            if idx >= 0:
                print(f"  Name '{t}' at names_buf[{idx:#x}]")
                # find directory entry
                n_entries = gthdr['dir_size'] // gthdr['entry_size']
                for i in range(n_entries):
                    eoff     = gthdr['dir_offset'] + i * gthdr['entry_size']
                    name_off = read_u32(gtdata, eoff + 4)
                    attrs    = read_u32(gtdata, eoff + 8)
                    if name_off == idx:
                        is_dir   = bool(attrs & FILE_ATTRIBUTE_DIRECTORY)
                        data_off = read_u32(gtdata, eoff + 16)
                        sp_used  = read_u32(gtdata, eoff + 24)
                        abs_d    = (gthdr['dir_offset'] + data_off) if is_dir else (gthdr['data_start'] + data_off)
                        print(f"    entry[{i}] {'DIR' if is_dir else 'file'} data_off={data_off:#x} "
                              f"abs={abs_d:#010x} size={sp_used}")
            else:
                print(f"  Name '{t}' NOT in gametypes.db names buffer")
    else:
        print(f"gametypes.db ident={gt_ident:#010x} (not UTF)")

    # Broad search: look for partial strings in the ENTIRE file
    print("\n--- Broad search in entire file ---")
    for pat in [b'INDUSTR', b'FABRI', b'BUILD_R', b'RESEARCH', b'FLEET\x00',
                b'Toolbar!!C']:
        pos = data.find(pat)
        if pos >= 0:
            ctx = data[pos:pos+48]
            print(f"  {pat!r} @ {pos:#010x}: {ctx!r}")
        else:
            print(f"  {pat!r} NOT FOUND in gendata.db")

    # Find archetypes by searching the names buffer directly
    names_buf = data[hdr['names_offset'] : hdr['dir_offset']]
    print(f"\nNames buffer: {len(names_buf)} bytes")

    targets = ["LINDUSTRIAL", "FABRICATOR", "RESEARCH", "BUILD_RES", "FLEET",
               "Toolbar!!Default"]
    for t in targets:
        idx = names_buf.find(t.encode('ascii'))
        if idx >= 0:
            # verify null terminator
            print(f"  Name '{t}' at names_buf[{idx:#x}]")
            # now scan all dir entries for one whose name_off == idx
            dir_base = hdr['dir_offset']
            es       = hdr['entry_size']
            n_entries = hdr['dir_size'] // es
            found = False
            for i in range(n_entries):
                eoff      = dir_base + i * es
                attrs     = read_u32(data, eoff + 8)
                name_off  = read_u32(data, eoff + 4)
                data_off  = read_u32(data, eoff + 16)
                sp_used   = read_u32(data, eoff + 24)
                if name_off == idx:
                    is_dir = bool(attrs & FILE_ATTRIBUTE_DIRECTORY)
                    kind   = "DIR" if is_dir else "file"
                    abs_data = (dir_base + data_off) if is_dir else (hdr['data_start'] + data_off)
                    print(f"    entry[{i}] @{eoff:#010x}: {kind}, data_off={data_off:#x}, "
                          f"abs_data={abs_data:#010x}, size={sp_used}")
                    found = True
        else:
            print(f"  Name '{t}' NOT in names buffer")

    # ---- Verify toolbar binary layout ----
    idx = names_buf.find(b'Toolbar!!Default')
    if idx >= 0:
        dir_base  = hdr['dir_offset']
        es        = hdr['entry_size']
        n_entries = hdr['dir_size'] // es
        for i in range(n_entries):
            eoff     = dir_base + i * es
            name_off = read_u32(data, eoff + 4)
            attrs    = read_u32(data, eoff + 8)
            if name_off == idx and not (attrs & FILE_ATTRIBUTE_DIRECTORY):
                data_off   = read_u32(data, eoff + 16)
                sp_used    = read_u32(data, eoff + 24)
                blob_start = hdr['data_start'] + data_off
                verify_toolbar_layout(data, blob_start, sp_used)
                break

    # Find Toolbar!!Default blob, then search inside it for named toolbar instances
    # LINDUSTRIAL/FABRICATOR etc are not separate files -- they're named instances
    # inside the packed toolbar blob.
    print("\n--- Search inside Toolbar!!Default blob ---")
    idx = names_buf.find(b'Toolbar!!Default')
    if idx >= 0:
        dir_base  = hdr['dir_offset']
        es        = hdr['entry_size']
        n_entries = hdr['dir_size'] // es
        for i in range(n_entries):
            eoff     = dir_base + i * es
            name_off = read_u32(data, eoff + 4)
            attrs    = read_u32(data, eoff + 8)
            if name_off == idx and not (attrs & FILE_ATTRIBUTE_DIRECTORY):
                data_off  = read_u32(data, eoff + 16)
                sp_used   = read_u32(data, eoff + 24)
                blob_start = hdr['data_start'] + data_off
                blob_end   = blob_start + sp_used
                print(f"Toolbar!!Default blob: [{blob_start:#010x}, {blob_end:#010x}), {sp_used} bytes")

                # Search the blob for known toolbar instance names
                search_names = [b'LINDUSTRIAL', b'FABRICATOR', b'RESEARCH',
                                b'BUILD_RES', b'FLEET', b'Toolbar!!', b'Mantis']
                for sn in search_names:
                    blob_slice = data[blob_start:blob_end]
                    pos = 0
                    hits = []
                    while True:
                        p = blob_slice.find(sn, pos)
                        if p < 0:
                            break
                        hits.append(blob_start + p)
                        pos = p + 1
                    if hits:
                        print(f"  '{sn.decode()}' found at: {[hex(h) for h in hits[:8]]}")
                    else:
                        print(f"  '{sn.decode()}' NOT found in blob")
                break

if __name__ == "__main__":
    main()
