#!/usr/bin/env python3
"""Bounded, private flagship table/constructor evidence; no retail code exported.

Reports pointer membership separately from proven constructor table ownership.
No controls, databases, game files or Ghidra projects are changed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest().upper()


class Image:
    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        pe = struct.unpack_from('<I', self.data, 0x3c)[0]
        n, optlen = struct.unpack_from('<H12xH', self.data, pe + 6)
        opt = pe + 24
        if struct.unpack_from('<H', self.data, opt)[0] != 0x10b:
            raise ValueError('Expected PE32')
        self.base = struct.unpack_from('<I', self.data, opt + 28)[0]
        self.sections = []
        for i in range(n):
            p = opt + optlen + i * 40
            name = self.data[p:p+8].rstrip(b'\0').decode()
            size, rva, rawlen, raw = struct.unpack_from('<IIII', self.data, p + 8)
            flags = struct.unpack_from('<I', self.data, p + 36)[0]
            self.sections.append((name, rva, rawlen, raw, flags))

    def offset(self, va):
        rva = va - self.base
        for _, start, length, raw, _ in self.sections:
            if start <= rva < start + length:
                return raw + rva - start
        raise ValueError(f'Address outside file: {va:x}')

    def u32(self, va):
        return struct.unpack_from('<I', self.data, self.offset(va))[0]

    def table(self, va, count):
        return [self.u32(va + i * 4) for i in range(count)]

    def executable(self, va):
        return any(self.base + rva <= va < self.base + rva + length and flags & 0x20000000
                   for _, rva, length, _, flags in self.sections)

    def pointers(self, target):
        result = []
        for name, rva, length, raw, flags in self.sections:
            if flags & 0x20000000:
                continue
            for pos in range(raw, raw + length - 3, 4):
                if struct.unpack_from('<I', self.data, pos)[0] == target:
                    result.append({'address': self.base + rva + pos - raw, 'section': name})
        return result


def disasm(image, start, stop):
    result = subprocess.run(['llvm-objdump', '-d', '--no-show-raw-insn',
                             f'--start-address={start}', f'--stop-address={stop}',
                             str(image.path)], check=True, text=True, capture_output=True)
    return [(int(m[1], 16), m[2].strip()) for line in result.stdout.splitlines()
            if (m := re.match(r'\s*([0-9a-fA-F]+):\s+(.*)', line))]


def stores(instructions):
    result = []
    for va, text in instructions:
        m = re.fullmatch(r'movl\s+\$(0x[0-9a-f]+),\s*(0x[0-9a-f]+)?\(%(\w+)\).*', text)
        if m:
            result.append({'instruction_va': va, 'table_va': int(m[1], 16),
                           'object_offset': int(m[2] or '0', 16), 'object_register': m[3]})
    return result


def publics(path, image):
    raw = subprocess.run(['llvm-pdbutil', 'dump', '-publics', str(path)],
                         check=True, text=True, capture_output=True).stdout
    result = []
    pat = re.compile(r'S_PUB32[^\n]*`([^`]+)`\s*\n\s*flags = [^\n]*addr = (\d+):(\d+)')
    for m in pat.finditer(raw):
        segment, offset = int(m[2]), int(m[3])
        if 1 <= segment <= len(image.sections):
            result.append({'name': m[1], 'va': image.base + image.sections[segment-1][1] + offset})
    return result


def bridge(path):
    rows = [json.loads(line) for line in Path(path).read_text().splitlines()]
    return rows[0], [row for row in rows[1:] if row.get('record') == 'function']


def raw_field_return(image, va, limit):
    """Extract only a first-return accessor's observed displacement, not its name."""
    inst = disasm(image, va, va + limit)
    stop = next((i for i, (_, text) in enumerate(inst) if re.match(r'retl?\b', text)), None)
    if stop is None:
        return None
    inst = inst[:stop+1]
    calls = [(address, text) for address, text in inst if text.startswith('call')]
    if len(calls) != 1 or not re.fullmatch(r'calll\s+\*0x34\(%\w+\)', calls[0][1]):
        return None
    if len(inst) < 2:
        return None
    address, text = inst[-2]
    m = re.fullmatch(r'movl\s+(0x[0-9a-f]+)\(%eax\),\s*%eax', text)
    if not m or address <= calls[0][0]:
        return None
    return {'raw_field_offset':int(m[1],16),'read_instruction_va':address,
            'raw_accessor_call_va':calls[0][0],'raw_accessor_vtable_byte_offset':0x34,
            'return_instruction_va':inst[-1][0]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--retail', type=Path, required=True)
    parser.add_argument('--current', type=Path, required=True)
    parser.add_argument('--pdb', type=Path, required=True)
    parser.add_argument('--bridge', type=Path, required=True)
    parser.add_argument('--parser-resource', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    retail, current = Image(args.retail), Image(args.current)
    if sha(args.retail) != 'ED8741C0CB727A43B7DE7BE0E73566B04FBC1CBD58B54A62D538E3F39DCB3F52':
        raise ValueError('Retail identity mismatch')
    head, rows = bridge(args.bridge)
    if sha(args.current) != head['module_sha256'] or sha(args.pdb) != head['pdb_sha256']:
        raise ValueError('Current image/PDB identity mismatch')
    symbols = publics(args.pdb, current)
    names = {row['va']: row['name'] for row in symbols}
    tables = [row for row in symbols if row['name'].startswith('??_7') and
              ('UFlagship@@' in row['name'] or 'UFlagshipFactory@@' in row['name'])]
    getters = [row for row in symbols if row['name'].startswith('?GetToolbar') and '@Flagship@@' in row['name']]
    all_tables = sorted(row['va'] for row in symbols if row['name'].startswith('??_7'))
    current_tables = []
    for row in tables:
        # Current MSVC vftables have a preceding RTTI locator. A following table's
        # preceding locator is not a slot in this table. Long tables remain bounded.
        successors = [va for va in all_tables if va > row['va']]
        count = min(64, (min(successors) - row['va'] - 4)//4) if successors else 64
        entries = current.table(row['va'], max(0, count))
        entries = entries[:next((i for i, va in enumerate(entries) if not current.executable(va)), len(entries))]
        current_tables.append({**row, 'status': 'COMPILER_NAMED_VFTABLE',
            'bounded_entry_count': len(entries),
            'entries':[{'zero_based_slot':i,'target_va':va,'pdb_symbol':names.get(va)} for i,va in enumerate(entries)],
            'getter_slots': [
                {'zero_based_slot': i, 'slot_offset': i*4, 'target_va': va, 'name': names[va]}
                for i, va in enumerate(entries) if va in {g['va'] for g in getters}]})
    retail_stores = stores(disasm(retail, 0x483cb0, 0x483d81))
    retail_table = 0x593b5c
    entries = retail.table(retail_table, (0x593bd0-retail_table)//4)
    retail_getters = [0x480b30,0x480b50,0x480b70,0x480b90]
    current_funcs = []
    for row in rows:
        syms = row.get('pdb_symbols', [])
        relevant = [s for s in syms if ('Flagship' in s.get('pdb_name','')) and
                    any(token in s.get('pdb_name','') for token in ['GetToolbar','CreateArchetype','CreateInstance','Flagship::Flagship','ObjectImpl','_Coi'])]
        if relevant:
            current_funcs.append({k: row.get(k) for k in ['va','rva','identity_status','relationship','source_candidates','source_frames','pdb_symbols']})
    factory_table = 0x593e58
    factory_entries = retail.table(factory_table, 8)
    factory_registration = [s for s in stores(disasm(retail,0x484110,0x484150)) if s['table_va'] == factory_table]
    resource_hash = sha(args.parser_resource)
    if resource_hash != 'C8A1215C3422C75B2BD29D793C9E28FFA1BAF2DE5E0C244C20D26F7EE737860D':
        raise ValueError('Parser resource identity mismatch')
    parser_text = args.parser_resource.read_text(encoding='latin1')
    block = parser_text.split('struct BASE_FLAGSHIP_SAVELOAD\n{',1)[1].split('\n};',1)[0]
    for token in ['U32 dockAgentID;', 'U32 dockshipID;', 'U32 targetID;', 'S32 idleTimer;',
                  '} mode;', 'bool bAttached;', 'U8   numDeadShips;', '} dmessages;',
                  '} smessages;', 'S32 forecastTimer;', '} lastForecast;', 'ADMIRAL_TACTIC admiralTactic;']:
        if token not in block:
            raise ValueError(f'Retail parser declaration changed: {token}')
    admiral = next(t for t in current_tables if t['name'].endswith('6BIAdmiral@@@'))
    current_entries = current.table(admiral['va'],admiral['bounded_entry_count'])
    anchors = []
    for method, retail_va in [('GetAdmiralHotkey',0x42d960),('IsDocked',0x483e70),
                              ('GetDockship',0x483e80),('SetAdmiralTactic',0x480bb0),
                              ('GetAdmiralTactic',0x480bc0)]:
        symbol = next(s for s in symbols if s['name'].startswith('?'+method+'@Flagship@@'))
        anchors.append({'method':method,'retail_va':retail_va,'current_va':symbol['va'],
                        'retail_slot':entries.index(retail_va),'current_slot':current_entries.index(symbol['va'])})
    # Validate the non-toolbar observations before making the alignment usable.
    checks = [
        ('raw_hotkey_to_instance', disasm(retail,0x482b10,0x482b42), ['0x288(%eax)', '0x1b8c(%ecx)', '0x1a28(%ecx)']),
        ('save_block_copy', disasm(retail,0x482b50,0x482b76), ['0x1a08(%ecx)', '0x1a8(%eax)', '$0x9, %ecx', 'movsl']),
        ('retail_hotkey_member', disasm(retail,0x42d960,0x42d967), ['0x188(%ecx)']),
        ('retail_docked_member', disasm(retail,0x483e70,0x483e76), ['0x18(%ecx)']),
        ('retail_tactic_setter', disasm(retail,0x480bb0,0x480bbA), ['0x24(%ecx)']),
        ('retail_tactic_getter', disasm(retail,0x480bc0,0x480bc4), ['0x24(%ecx)'])]
    validated_checks = []
    for label, inst, facts in checks:
        joined = '\n'.join(text for _,text in inst)
        if not all(fact in joined for fact in facts):
            raise ValueError(f'Non-toolbar anchor probe failed: {label}')
        validated_checks.append({'probe':label,'instruction_addresses':[va for va,_ in inst], 'status':'VERIFIED'})
    selected = {a['method']:a for a in anchors}
    if any(selected[k]['current_slot'] - selected[k]['retail_slot'] != 1
           for k in ['IsDocked','GetDockship','SetAdmiralTactic','GetAdmiralTactic']):
        raise ValueError('Independent bracketing anchors disagree on +1 alignment')
    if selected['GetAdmiralHotkey']['retail_slot'] != selected['GetAdmiralHotkey']['current_slot']:
        raise ValueError('Pre-insertion anchor drifted')
    result = {'schema':'cfw.v3-bounded-vtable-context.v1', 'tool_sha256':sha(__file__),
        'inputs':{str(p):sha(p) for p in [args.retail,args.current,args.pdb,args.bridge,args.parser_resource]},
        'warning':'Functional pointer facts only; no retail code or bulk mappings. Pointer membership alone is not table ownership. No claim of global vtable ABI parity.',
        'retail':{'factory_create_instance':0x483cb0,
            'construction_table_stores':retail_stores,
            'admiral_table':{'table_va':retail_table,'owner_evidence':'CreateInstance stores table at allocated Flagship object +0x1a04',
                'bounded_entry_count':len(entries),'end_boundary':'next constructor-installed table 0x593bd0; exact declared retail interface end not independently typed',
                'entries':[{'zero_based_slot':i,'target_va':va} for i,va in enumerate(entries)],
                'getter_slots':[{'target_va':va,'zero_based_slot':entries.index(va),'slot_offset':entries.index(va)*4,
                                'pointer_occurrences':retail.pointers(va)} for va in retail_getters]},
            'factory_table':{'table_va':factory_table,'bounded_targets':factory_entries,
                'create_archetype_slot':factory_entries.index(0x483bf0),
                'create_instance_slot':factory_entries.index(0x483cb0),
                'construction_store':factory_registration,
                'ownership_status':'VERIFIED_FACTORY_ALLOCATION_TABLE_STORE'}},
        'current':{'tables':current_tables,'getters':getters,'function_bridge':current_funcs}}
    result['class_table_relationship'] = {
        'status':'VERIFIED_BOUNDED_CLASS_INTERFACE_RELATIONSHIP',
        'retail_table_va':retail_table, 'current_table_va':admiral['va'],
        'retail_object_offset':0x1a04, 'current_object_offset':0x1ab4,
        'basis':['retail class=1 subtype=9 factory and CreateInstance allocation/table stores',
                 'current PDB/source verified FlagshipFactory and compiler named IAdmiral table',
                 'independent hotkey raw-field and parser-save docking/tactic member contracts'],
        'scope':'Only this constructor-owned interface table; no global layout or full-method equivalence claim'}
    result['slot_alignment'] = {
        'status':'VERIFIED', 'current_minus_retail_slot':1,
        'retail_interval_inclusive':[11,28], 'current_interval_inclusive':[12,29],
        'selection_basis':'Non-toolbar anchors precede and follow toolbar interval. Toolbar identities/order were not used to select alignment.',
        'anchors':anchors, 'validated_non_toolbar_probes':validated_checks,
        'retail_save_layout':{'resource_sha256':resource_hash,'record':'BASE_FLAGSHIP_SAVELOAD',
            'size':36,'bAttached_offset':20,'admiralTactic_offset':32,
            'derivation':'four initial U32/S32 plus 4-byte mode; bool, U8 and two 1-byte message bitfields; S32 forecastTimer, 4-byte forecast enum, 4-byte tactic enum',
            'instance_base':0x1a08,'saved_destination_offset':0x1a8,'copy_dwords':9,
            'tactic_instance_offset':0x1a28,'tactic_interface_offset':0x24,
            'docked_instance_offset':0x1a1c,'docked_interface_offset':0x18},
        'insertion':{'method':'SetAdmiralHotkey','current_slot':11,'retail_slot':None,
                     'basis':'retail hotkey slot10 is followed directly by schema-anchored docked member slot11; current PDB table inserts setter between them'},
        'limitations':['Only the bracketing interval is approved for this bounded alignment; not every intervening method has independent semantic review.',
                      'This does not establish equality of function behavior or permit shared-database writes.']}
    # Individual identity requires the accessed semantic field, not the table
    # order or a shift selected using expected control correspondences.
    toolbar_block = parser_text.split('struct _toolbarInfo\n',1)[1].split('}toolbarInfo;',1)[0]
    members = re.findall(r'^\s*(U32|HBTNTXT::\w+)\s+(\w+)\s*;',toolbar_block,re.M)
    if len(members) != 4:
        raise ValueError('Retail toolbar tail is not four 32-bit declarations')
    # Retail FSHIP length 744 and final four-U32 tail are existing independent
    # parser/database layout facts; derive each field offset from that tail.
    field_schema = {744-len(members)*4+i*4:{'field':name,'type':typ}
                    for i,(typ,name) in enumerate(members)}
    retail_semantics = []
    for slot, va in enumerate(entries):
        observed = raw_field_return(retail,va,64)
        if observed and observed['raw_field_offset'] in field_schema:
            retail_semantics.append({'retail_va':va,'retail_slot':slot,
                                     **observed,**field_schema[observed['raw_field_offset']]})
    current_semantics = []
    root = Path(__file__).resolve().parents[3]
    for row in current_funcs:
        candidates = row.get('source_candidates') or []
        if len(candidates) != 1 or row['va'] not in current_entries:
            continue
        candidate = candidates[0]
        source_path = root / candidate['source_file']
        if sha(source_path) != candidate['source_blob_sha256']:
            raise ValueError('Current getter source bytes disagree with pinned compiler bridge')
        line = int(candidate['source_id'].split(':',2)[1])
        source = '\n'.join(source_path.read_text(encoding='latin1').splitlines()[line-1:line+11])
        match = re.search(r'return\s+\w+->toolbarInfo\.(\w+)\s*;',source)
        if not match:
            continue
        # First closing brace ends these short, non-nested named source bodies.
        first_body = source.split('{',1)[1].split('}',1)[0]
        match = re.search(r'return\s+\w+->toolbarInfo\.(\w+)\s*;',first_body)
        if not match:
            continue
        observed = raw_field_return(current,row['va'],max(s['size'] for s in row['pdb_symbols']))
        if not observed:
            raise ValueError('Named current field return lacks compiled raw-access return proof')
        current_semantics.append({'current_va':row['va'],'current_slot':current_entries.index(row['va']),
            'source_id':candidate['source_id'],'source_file_sha256':candidate['source_blob_sha256'],
            'pdb_names':[s['pdb_name'] for s in row['pdb_symbols']],
            'field':match[1],**observed})
    mappings = []
    for item in retail_semantics:
        partners = [c for c in current_semantics if c['field'] == item['field']]
        if len(partners) != 1:
            raise ValueError('Individual getter semantic identity is ambiguous')
        mappings.append({'status':'VERIFIED_FIELD_IDENTITY','field':item['field'],
                         'retail':item,'current':partners[0]})
    if len(mappings) != len(members) or len({m['field'] for m in mappings}) != len(members):
        raise ValueError('Individual semantic coverage is incomplete')
    result['per_getter_mapping'] = {'status':'VERIFIED_FIELD_IDENTITY',
        'selection_basis':'Unique join of observed retail raw-field displacement through shipped parser tail member names to current compiler/source-byte-proven return-field expression. Neither expected controls nor slot/address order participates in partner selection.',
        'retail_schema':{'parser_resource_sha256':resource_hash,'record_size':744,
                         'tail_fields_by_offset':field_schema},
        'mappings':mappings,
        'limitations':'Functional field identity only. Retail and current raw offsets differ; this does not prove compatible record loading or runtime parity.'}
    # Recover constructor/factory table writes only within already proven current
    # bridge functions. This is a bounded evidence probe, not all-function ranking.
    table_set = {row['va'] for row in tables}
    proofs = []
    for row in current_funcs:
        sizes = [s['size'] for s in row['pdb_symbols'] if s.get('size')]
        if not sizes:
            continue
        found = [s for s in stores(disasm(current,row['va'],row['va']+max(sizes))) if s['table_va'] in table_set]
        if found:
            proofs.append({'function_va':row['va'], 'pdb_names':[s['pdb_name'] for s in row['pdb_symbols']], 'stores':found})
    result['current']['construction_table_stores'] = proofs
    owned = [store for proof in proofs for store in proof['stores'] if store['table_va'] == admiral['va']]
    if len(owned) != 1:
        raise ValueError('Current class interface has no unique bounded construction proof')
    result['class_table_relationship']['current_object_offset'] = owned[0]['object_offset']
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n',encoding='utf8')
    print(json.dumps({'out':str(args.out),'sha256':sha(args.out),
                      'retail_getter_slots':result['retail']['admiral_table']['getter_slots'],
                      'current_getter_tables':[{k:t[k] for k in ['name','va','bounded_entry_count','getter_slots']}
                                               for t in current_tables if t['getter_slots']],
                      'slot_alignment_status':result['slot_alignment']['status'],
                      'current_construction_proof_count':len(proofs)},indent=2))


if __name__ == '__main__':
    main()
