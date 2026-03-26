"""
xmiff2.py - Python replacement for the 16-bit XMIFF.EXE tool.

Converts sfxdata.xmf + Sfxid.h into the binary sfxdata.dat that gets
embedded as resource IDR_SFX1 in Globals.dll.

Binary format (all little-endian):
  int32   LAST            -- count of SFX::ID enum entries (value of LAST)
  Per SFXCHUNK record:
    int32   id            -- numeric SFX::ID enum value
    float32 volume        -- 0.0 to 1.0
    char[32] filename     -- WAV filename, null-padded to 32 bytes
    float32 cutoff        -- playback cutoff (0.5 default for SFXCHUNK)

Usage:
  python xmiff2.py <sfxdata.xmf> <Sfxid.h> <sfxdata.dat>
  python xmiff2.py   (uses paths relative to script for the Conquest build)
"""

import struct, re, sys, os

# ---------------------------------------------------------------------------
# Paths (defaults relative to Code/App/Src where these files live)
# ---------------------------------------------------------------------------
SCRIPT_DIR   = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT    = os.path.normpath(os.path.join(SCRIPT_DIR, '..', '..'))
XMF_PATH     = os.path.join(REPO_ROOT, 'Code', 'App', 'Src',     'sfxdata.xmf')
SFXID_PATH   = os.path.join(REPO_ROOT, 'Code', 'App', 'DInclude', 'Sfxid.h')
OUT_PATH     = os.path.join(REPO_ROOT, 'Code', 'App', 'Src',     'sfxdata.dat')

if len(sys.argv) == 4:
    XMF_PATH, SFXID_PATH, OUT_PATH = sys.argv[1], sys.argv[2], sys.argv[3]

# ---------------------------------------------------------------------------
# 1. Parse Sfxid.h to build {name: int_value} map
# ---------------------------------------------------------------------------
def parse_sfxid(path):
    id_map = {}
    counter = 0
    in_enum = False
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if 'enum ID' in line:
                in_enum = True
                continue
            if not in_enum:
                continue
            if line.startswith('}'):
                break
            # Strip comments
            line = re.sub(r'//.*', '', line).strip().rstrip(',').strip()
            if not line:
                continue
            # Handle explicit assignment (e.g. INVALID=0)
            m = re.match(r'(\w+)\s*=\s*(\d+)', line)
            if m:
                counter = int(m.group(2))
                id_map[m.group(1)] = counter
                counter += 1
                continue
            m = re.match(r'(\w+)$', line)
            if m:
                id_map[m.group(1)] = counter
                counter += 1
    return id_map

# ---------------------------------------------------------------------------
# 2. Parse sfxdata.xmf to get list of (id_name, volume, filename, cutoff)
# ---------------------------------------------------------------------------
def parse_xmf(path):
    records = []
    with open(path, 'r') as f:
        text = f.read()
    # Strip // comments
    text = re.sub(r'//[^\n]*', '', text)
    # Strip C block comments
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)

    # SFXCHUNK2(id, volume, filename, cutoff)
    for m in re.finditer(
            r'SFXCHUNK2\s*\(\s*(\w+)\s*,\s*([\d.]+)\s*,\s*"([^"]+)"\s*,\s*([\d.]+)\s*\)',
            text):
        records.append((m.group(1), float(m.group(2)), m.group(3), float(m.group(4))))

    # SFXCHUNK(id, volume, filename)  -- default cutoff 0.5
    for m in re.finditer(
            r'(?<!2)SFXCHUNK\s*\(\s*(\w+)\s*,\s*([\d.]+)\s*,\s*"([^"]+)"\s*\)',
            text):
        records.append((m.group(1), float(m.group(2)), m.group(3), 0.5))

    return records

# ---------------------------------------------------------------------------
# 3. Write binary sfxdata.dat
# ---------------------------------------------------------------------------
def write_dat(records, id_map, out_path):
    last_val = id_map.get('LAST', len(id_map))
    with open(out_path, 'wb') as f:
        f.write(struct.pack('<i', last_val))
        for (id_name, volume, filename, cutoff) in records:
            if id_name not in id_map:
                print(f'WARNING: unknown ID {id_name!r}, skipping')
                continue
            fname_bytes = filename.encode('ascii', errors='replace')
            fname_padded = fname_bytes[:32].ljust(32, b'\x00')
            f.write(struct.pack('<i', id_map[id_name]))
            f.write(struct.pack('<f', volume))
            f.write(fname_padded)
            f.write(struct.pack('<f', cutoff))
    print(f'Wrote {out_path}  ({last_val} IDs, {len(records)} records)')

# ---------------------------------------------------------------------------
id_map  = parse_sfxid(SFXID_PATH)
records = parse_xmf(XMF_PATH)
write_dat(records, id_map, OUT_PATH)
