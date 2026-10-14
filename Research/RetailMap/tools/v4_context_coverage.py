#!/usr/bin/env python3
"""Read-only inventory of frozen pilot context availability; never runs scoring."""
import collections
import hashlib
import json
from pathlib import Path
import sqlite3
import argparse

ROOT = Path(__file__).resolve().parents[3]
LOCAL = ROOT / 'Evidence/RetailMap/local'

def load(name):
    return json.loads((LOCAL / name).read_text(encoding='utf8'))

def jsonl(name):
    return [json.loads(x) for x in (LOCAL / name).read_text(encoding='utf8').splitlines()]

def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest().upper()

def state(category, **facts):
    return {'category':category, **facts}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args=parser.parse_args()
    output=args.output.resolve()
    if LOCAL.resolve() not in output.parents or output.exists():
        raise ValueError('Output must be a new private evidence file')
    inputs=['pilot-selection-ground-truth-20261013.json',
            'retail-functions-v3-controls-r2.sqlite',
            'retail-metadata-20261012-enriched-v1.jsonl',
            'ui-module-20261012/trim-metadata-v1.jsonl',
            'compiled-bridge-final-conquest-v2.jsonl',
            'pilot-independent-ground-truth-20261013.json',
            'pilot-object-static-current-rva-bridge-20261013.json',
            'pilot-trim-current-rva-bridge-20261013.json',
            'pilot-v3-adjudicated-20261013.json',
            'v3-callgraph-final-controls-20261013-r3.json',
            'v3-vtable-context-final-semantic.json',
            'v3-trim-factory-context-20261013-r3.json',
            'frozen-matcher-v3-20261013.json',
            'pilot-trim-independent-crosscheck-20261013.json']
    hashes={name:sha(LOCAL/name) for name in inputs}
    assert hashes[inputs[0]]=='E92CDB2904F8BDA1B5992C0E9A90946D4491915919BCA5EF4E9E1C197E63A857'
    assert hashes[inputs[1]]=='D8142D4D8F6346B3A835201C0CC868643DA379158634C2291D73C0B56E634041'
    db=sqlite3.connect(f'file:{(LOCAL/inputs[1]).as_posix()}?mode=ro',uri=True)
    db.row_factory=sqlite3.Row
    metas={}
    for name in inputs[2:4]:
        rows=jsonl(name)
        h=rows[0]
        metas[h['image_sha256'].upper()]={
            'functions':{int(x['entry'],16):x for x in rows if x['record']=='function'},
            'tables':[x for x in rows if x['record']=='pointer_table_candidate'],
            'complete_footer':rows[-1].get('complete',False), 'path':name}
    bridge=jsonl(inputs[4])
    current_exe=bridge[0]['module_sha256'].upper()
    current_trim=load('frozen-matcher-v3-20261013.json')['inputs_sha256']['current_debug_trim']
    gt={int(row[0],16):row for group in load(inputs[5])['rows'].values() for row in group}
    supplemental={int(x['retail_va'],16):(x['source_id'],int(x['current_final_rva'],16),'SUPPLEMENTAL_RAW_PDB') for x in load(inputs[6])['rows']}
    supplemental.update({int(x[0],16):(x[1],int(x[2],16),'SUPPLEMENTAL_MAP_COFF') for x in load(inputs[7])['rows']})
    adjud=load(inputs[8])['dispute']
    crosscheck={int(x['retail_entry'],16):x for x in load(inputs[-1])['rows']}
    corrected=crosscheck[int(adjud['retail_va'],16)]
    assert corrected['source_id']==adjud['adjudicated_source_id']
    supplemental[int(adjud['retail_va'],16)]=(corrected['source_id'],int(corrected['current_rva'],16),'INDEPENDENT_CROSSCHECK_MAP')
    graph=load(inputs[9])
    def count(sql,values):
        return db.execute(sql,values).fetchone()[0]
    selection=load(inputs[0])['sample']
    inventory=[]
    for subsystem,samples in selection.items():
        rh=next(h for h,m in metas.items() if ('Trim' in subsystem)==('trim-' in m['path']))
        meta=metas[rh]
        base=0x10000000 if subsystem=='Trim/UI' else 0x400000
        ch=current_trim if subsystem=='Trim/UI' else current_exe
        for address,size in samples:
            va=int(address,16); rva=va-base; f=meta['functions'][va]
            assert f['body_size']==size
            indexed=db.execute('SELECT retail_id,size FROM retail_functions WHERE module_sha256=? AND va=?',(rh,va)).fetchone()
            assert indexed is not None and indexed['size']==size
            rid=indexed['retail_id']; truth=gt[va]; sid=truth[2]
            if va==int(adjud['retail_va'],16):
                sid=adjud['adjudicated_source_id']
            links=[] if sid is None else [dict(x) for x in db.execute('SELECT * FROM compiled_source_links WHERE module_sha256=? AND source_id=?',(ch,sid))]
            rvas=sorted({x['rva'] for x in links})
            supplement=supplemental.get(va)
            if supplement and supplement[0]!=sid:
                supplement=None
            if not rvas and supplement:
                rvas=[supplement[1]]
            source_status='LINKED_NOT_SCORED' if links else 'EXTRACTED_NOT_LINKED' if supplement else 'CONTEXT_NOT_EXTRACTED'
            data=f.get('data_references',[])
            strings=[d for d in data if d.get('kind')=='string_reference']
            imports=[e for e in f.get('call_edges',[]) if e.get('external_name')]
            table_hits=[{'table_va':x['address'],'slots':[i for i,t in enumerate(x['targets']) if int(t,16)==va]} for x in meta['tables'] if any(int(t,16)==va for t in x['targets'])]
            feature_counts={x[0]:x[1] for x in db.execute('SELECT feature_kind,count(*) FROM features WHERE owner_kind=? AND owner_id=? GROUP BY feature_kind',('retail',rid))}
            machine_edges=count('SELECT count(*) FROM machine_call_edges_v3 WHERE module_sha256=? AND (owner_rva=? OR target_rva=?)',(rh,rva,rva))
            vslots=count('SELECT count(*) FROM machine_vtable_slots_v3 WHERE module_sha256=? AND target_rva=?',(rh,rva))
            edges=count('SELECT count(*) FROM retail_call_edges WHERE retail_id=?',(rid,))
            graph_count=len(f.get('call_edges',[]))+len(f.get('callers',[]))+f.get('unresolved_call_sites',0)
            channels={
                'source_compiled_bridge':state(source_status, source_id=sid, current_rvas=[hex(x) for x in rvas], imported_links=len(links), supplemental_provenance=None if supplement is None else supplement[2], identity_review=truth[1], stale_primary_bridge_discarded=va==int(adjud['retail_va'],16), secondary_crosscheck_proposal=crosscheck.get(va) if truth[1]=='U' else None),
                'retail_callgraph':state('LINKED_NOT_SCORED' if edges or feature_counts.get('caller') else 'EXTRACTED_NOT_LINKED' if graph_count else 'CONTEXT_NOT_EXTRACTED', saved_call_edges=len(f.get('call_edges',[])), saved_callers=len(f.get('callers',[])), unresolved_call_sites=f.get('unresolved_call_sites',0), database_retail_edges=edges, bounded_machine_edges=machine_edges, frozen_v3_graph_scoring=False),
                'paired_machine_callgraph':state('CONTEXT_NOT_EXTRACTED', reason='Full graphs computed transiently by v3 helper, but durable capture retains controls only. Pilot per-function current contexts are not persisted. Saved Ghidra edges remain separately available.'),
                'imports':state('LINKED_NOT_SCORED' if feature_counts.get('import') else 'EXTRACTED_NOT_LINKED' if imports else 'CONTEXT_NOT_EXTRACTED', saved_named_edges=len(imports), database_import_features=feature_counts.get('import',0)),
                'pointer_table_candidates':state('EXTRACTED_NOT_LINKED' if table_hits else 'CONTEXT_NOT_EXTRACTED', memberships=table_hits, ownership_verified=False, reason='Candidate pointer membership is not vtable ownership; zero extracted membership is not physical absence.'),
                'owned_vtable_pair':state('LINKED_NOT_SCORED' if vslots else 'CONTEXT_NOT_EXTRACTED', bounded_verified_slot_rows=vslots, reason='Existing semantic table relationship is bounded to IAdmiral development controls.'),
                'retail_strings':state('LINKED_NOT_SCORED' if feature_counts.get('string_hash') else 'EXTRACTED_NOT_LINKED' if strings else 'CONTEXT_NOT_EXTRACTED', saved_hashed_references=len(strings), database_hash_features=feature_counts.get('string_hash',0), hashes=[x['string_sha256_utf8'] for x in strings]),
                'paired_rare_strings':state('EXTRACTED_NOT_LINKED' if strings else 'CONTEXT_NOT_EXTRACTED', reason='Durable pilot retail ASCII-immediate/current-function comparison and rarity linkage not present; v3 string evidence/scoring is gated to retail 0x10061240.'),
                'retail_data_references':state('LINKED_NOT_SCORED' if feature_counts.get('global') else 'EXTRACTED_NOT_LINKED' if data else 'CONTEXT_NOT_EXTRACTED', saved_references=len(data), database_global_features=feature_counts.get('global',0), kinds=dict(collections.Counter(x.get('kind') for x in data))),
                'semantic_field_factory_context':state('CONTEXT_NOT_EXTRACTED', reason='Existing field/subtype probes select development functions only; numeric constants do not establish semantic fields or raw-record ownership.'),
                'current_source_class':state('LINKED_NOT_SCORED' if any(x.get('qualified_name') and '::' in x['qualified_name'] and x['relationship']=='ONE_TO_ONE' for x in links) else 'EXTRACTED_NOT_LINKED' if sid and '::' in sid.split(':',2)[-1] else 'CONTEXT_NOT_EXTRACTED', source_class=None if not sid or '::' not in sid.split(':',2)[-1] else sid.split(':',2)[-1].split('::',1)[0], retail_class_automatically_linked=False),
            }
            assert all(x['category'] in {'CONTEXT_NOT_EXTRACTED','EXTRACTED_NOT_LINKED','LINKED_NOT_SCORED','TRULY_ABSENT'} for x in channels.values())
            inventory.append({'subsystem':subsystem,'retail_va':hex(va),'retail_rva':hex(rva),'retail_module_sha256':rh,'current_module_sha256':ch,'saved_function_size':size,'metadata_function_available':True,'category':'EXTRACTED_NOT_LINKED','channels':channels,'frozen_v3_context_eligible':False,'context_absence_proven':False,'evidence_ref':meta['path']})
    assert len(inventory)==45
    summaries={}
    for subsystem in [*selection,'aggregate']:
        rows=[x for x in inventory if subsystem=='aggregate' or x['subsystem']==subsystem]
        n=len(rows)
        channel_summary={}
        for name in rows[0]['channels']:
            c=collections.Counter(x['channels'][name]['category'] for x in rows)
            channel_summary[name]={'counts':dict(c),'percentages':{k:round(v*100/n,3) for k,v in c.items()}}
        summaries[subsystem]={
            'denominator':n,
            'overall_categories':{'EXTRACTED_NOT_LINKED':{'count':n,'percent':100.0}},
            'channels':channel_summary,
            'functions_with_saved_call_context':sum(
                x['channels']['retail_callgraph']['saved_call_edges']
                + x['channels']['retail_callgraph']['saved_callers']
                + x['channels']['retail_callgraph']['unresolved_call_sites'] > 0 for x in rows),
            'functions_with_saved_direct_edge_or_caller':sum(
                x['channels']['retail_callgraph']['saved_call_edges']
                + x['channels']['retail_callgraph']['saved_callers'] > 0 for x in rows),
            'functions_with_only_unresolved_call_sites':sum(
                x['channels']['retail_callgraph']['saved_call_edges']
                + x['channels']['retail_callgraph']['saved_callers'] == 0
                and x['channels']['retail_callgraph']['unresolved_call_sites'] > 0 for x in rows),
            'functions_with_saved_strings':sum(x['channels']['retail_strings']['saved_hashed_references']>0 for x in rows),
            'functions_with_saved_data_references':sum(x['channels']['retail_data_references']['saved_references']>0 for x in rows),
            'functions_with_pointer_candidates':sum(bool(x['channels']['pointer_table_candidates']['memberships']) for x in rows),
            'functions_with_current_rva_evidence':sum(bool(x['channels']['source_compiled_bridge']['current_rvas']) for x in rows),
        }
        summaries[subsystem]['availability_percentages']={key:round(value*100/n,3) for key,value in summaries[subsystem].items() if key.startswith('functions_with_')}
    result={'schema':'cfw.v4-context-coverage-inventory.v1','status':'PRE_SCORING_AVAILABILITY_AUDIT','selection_use':'All 45 old V3 pilot functions are V4 development material; no new scoring or identity promotion.','category_definitions':{'CONTEXT_NOT_EXTRACTED':'Required channel or pairing is absent from durable extracted artifacts. Empty incomplete analysis is included here, never treated as negative evidence.','EXTRACTED_NOT_LINKED':'Raw evidence exists but required module/source/pair/ownership link is missing from frozen scoring input.','LINKED_NOT_SCORED':'Evidence is indexed to a function/source in frozen DB, but pilot function is outside frozen context gates. May be one-sided; pairing availability is stated separately.','TRULY_ABSENT':'Requires exhaustive observation establishing physical/semantic absence. No row satisfies that standard in this inventory.'},'inputs_sha256':hashes,'tool_sha256':sha(Path(__file__)),'database_channel_population':{table:count('SELECT count(*) FROM '+table,()) for table in ['retail_call_edges','machine_call_edges_v3','machine_vtable_slots_v3','compiled_source_links']},'full_graph_capture_coverage':{'retail':graph['retail_coverage'],'current':graph['current_coverage'],'durable_rows':'controls only; global coverage statistics do not provide pilot adjacency rows'},'summary':summaries,'functions':inventory,'limitations':['Audit measures durable artifact availability, not semantic equivalence, correctness or matcher performance.','No disassembly/decompilation text is copied into this inventory.','No new binary context extraction or scoring was run.','TRULY_ABSENT remains unused because saved-analysis completeness is insufficient.','40 primary identities include one posthoc correction; 0x10062520 stale onSetFocus compiled bridge is discarded in favor of independently reviewed sendMouseUpdate MAP RVA 0x949b0.','Five unresolved primary Trim identities remain unresolved; independent crosscheck suggestions are recorded as secondary proposals without forcing them into source links.','Current per-function ASCII strings and full graph were transient computations; global statistics are not function-level durable extraction.','Vtable candidates without constructor/class ownership cannot establish transferable slot identities.']}
    with output.open('x',encoding='utf8',newline='\n') as stream:
        json.dump(result,stream,indent=2,sort_keys=True);stream.write('\n')
    assert all(sha(LOCAL/name)==value for name,value in hashes.items())
    print(json.dumps({'output':str(output),'sha256':sha(output),'function_count':len(inventory),'summary':summaries},indent=2))

if __name__ == '__main__':
    main()
