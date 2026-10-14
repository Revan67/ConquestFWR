#!/usr/bin/env python3
"""Private V4 development feasibility only; conditioned candidate pool, no identities promoted."""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import sqlite3
import validate_machine_matcher_v2 as body


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest().upper()


def capture(path):
    rows = [json.loads(line) for line in Path(path).read_text().splitlines()]
    if rows[-1].get('complete') is not True:
        raise ValueError('Incomplete capture')
    return rows[0], {int(row['va'], 16): row for row in rows if row['record'] == 'function'}


def cosine(left, right):
    if not left or not right:
        return 0.0
    norm = math.sqrt(sum(x*x for x in left.values()) * sum(x*x for x in right.values()))
    return sum(value * right.get(key, 0) for key, value in left.items()) / norm if norm else 0.0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    local = Path('Evidence/RetailMap/local')
    if local.resolve() not in args.output.resolve().parents or args.output.exists():
        raise ValueError('Output must be fresh private evidence')
    paths = {
        'retail_exe': local/'v4-ir-retail-exe-full-20261013-r2.jsonl',
        'retail_trim': local/'v4-ir-retail-trim-20261013-r2.jsonl',
        'current_exe': local/'v4-ir-current-exe-20261013-r2.jsonl',
        'current_trim': local/'v4-ir-current-trim-20261013-r2.jsonl',
        'evaluation': local/'pilot-v3-evaluation-reviewed-20261013.json',
        'adjudication': local/'pilot-v3-adjudicated-20261013.json',
        'database': local/'pilot-v3-frozen-20261013.sqlite',
        'probe': Path(__file__).with_name('ProbeHighFunctionIR.java'),
        'evaluator': Path(__file__),
        'installed_vt_entrypoint': Path('C:/Utilities/ghidra_12.1.3_PUBLIC/Ghidra/Features/VersionTracking/ghidra_scripts/AutoVersionTrackingScript.java'),
        'installed_bsim_entrypoint': Path('C:/Utilities/ghidra_12.1.3_PUBLIC/Ghidra/Features/BSim/ghidra_scripts/GenerateSignatures.java'),
        'installed_fid_entrypoint': Path('C:/Utilities/ghidra_12.1.3_PUBLIC/Ghidra/Features/FunctionID/ghidra_scripts/FIDHashCurrentFunction.java'),
        'installed_decompiler_jar': Path('C:/Utilities/ghidra_12.1.3_PUBLIC/Ghidra/Features/Decompiler/lib/Decompiler.jar'),
        'installed_bsim_jar': Path('C:/Utilities/ghidra_12.1.3_PUBLIC/Ghidra/Features/BSim/lib/BSim.jar'),
    }
    captures = {key: capture(path) for key,path in paths.items() if key.startswith(('retail_', 'current_'))}
    for key,expected in [('retail_exe',body.RETAIL_EXE),('retail_trim',body.RETAIL_TRIM),('current_exe',body.CURRENT_EXE),('current_trim',body.CURRENT_TRIM)]:
        if captures[key][0]['image_sha256'].upper() != expected:
            raise ValueError('Noncanonical image')
    connection = sqlite3.connect(f'file:{paths["database"].resolve().as_posix()}?mode=ro', uri=True)
    features = {module:body.load_features(connection,module) for module in [body.RETAIL_EXE,body.RETAIL_TRIM,body.CURRENT_EXE,body.CURRENT_TRIM]}
    idf = {module:body.weights(features[module]) for module in [body.CURRENT_EXE,body.CURRENT_TRIM]}
    rows = []
    for gt in json.loads(paths['evaluation'].read_text())['ranked']:
        if not gt.get('true_rva'):
            continue
        trim = gt['subsystem']=='Trim/UI'
        base=0x10000000 if trim else 0x400000
        retail_module,current_module=(body.RETAIL_TRIM,body.CURRENT_TRIM) if trim else (body.RETAIL_EXE,body.CURRENT_EXE)
        retail_va=int(gt['retail_va'],16)
        current_rva=0x949b0 if retail_va==0x10062520 else int(gt['true_rva'],16)
        retail=captures['retail_trim' if trim else 'retail_exe'][1][retail_va]
        current=captures['current_trim' if trim else 'current_exe'][1][base+current_rva]
        expected=features[current_module][current_rva]['data']['byte_extent']
        extent_ok=(current['body_bytes']==expected and int(current['body_min'],16)==base+current_rva and int(current['body_max'],16)==base+current_rva+expected-1)
        valid=extent_ok and retail['decompile_completed'] and current['decompile_completed'] and not retail['bsim_bad_data'] and not current['bsim_bad_data'] and not retail['bsim_unimplemented'] and not current['bsim_unimplemented']
        rows.append({'retail_va':retail_va,'current_va':base+current_rva,'subsystem':gt['subsystem'],'extent_reference':'MAP next-symbol span includes possible padding; mismatch remains uncertain' if trim else 'PDB compiler function extent','current_expected_bytes':expected,'current_recovered_bytes':current['body_bytes'],'extent_complete':extent_ok,'ir_usable':valid,'retail':retail,'current':current,'modules':(retail_module,current_module),'base':base})
    metrics={}
    for subsystem in ['Object/archetype','Flagship','Trim/UI']:
        pool=[row for row in rows if row['subsystem']==subsystem and row['ir_usable']]
        results=[]
        for row in pool:
            ranks={}
            for method in ['body','raw_pcode_shapes','high_pcode_shapes','bsim_unweighted_cosine']:
                scores=[]
                for candidate in pool:
                    if method=='body':
                        score=body.score(features[row['modules'][0]][row['retail_va']-row['base']],features[row['modules'][1]][candidate['current_va']-row['base']],idf[row['modules'][1]])[0]
                    else:
                        key='bsim_features' if method=='bsim_unweighted_cosine' else method
                        left,right=row['retail'][key],candidate['current'][key]
                        score=cosine(Counter(left),Counter(right)) if method=='bsim_unweighted_cosine' else cosine(left,right)
                    scores.append((candidate['current_va'],score))
                true=next(score for va,score in scores if va==row['current_va'])
                ranks[method]=sum(score>=true-0.005 for _,score in scores)
            results.append({'retail_va':row['retail_va'],'ranks_worst_neartie':ranks})
        metrics[subsystem]={'valid_pairs':len(pool),'methods':{method:{'top1':sum(item['ranks_worst_neartie'][method]<=1 for item in results),'mrr':round(sum(1/item['ranks_worst_neartie'][method] for item in results)/len(results),6) if results else None} for method in ['body','raw_pcode_shapes','high_pcode_shapes','bsim_unweighted_cosine']},'rows':results}
    clean=[{key:value for key,value in row.items() if key not in ['retail','current','modules','base']} for row in rows]
    report={'schema':'cfw.v4.ir-feasibility.v1','status':'DEVELOPMENT_ONLY_CONDITIONED_POOL_NOT_BROAD_RANKING','limitation':'Candidate universe is only the complete adjudicated pairs in each subsystem, not the full emitted population. Native BSim features use unweighted cosine here, not calibrated BSim LSH similarity. Matching compiler extent does not independently validate decompiler semantics, calling convention, or switch recovery. No confidence or identity promotion.','inputs':{key:{'path':str(path),'sha256':sha(path)} for key,path in paths.items()},'captures':{key:{'requested':len(value[1]),'decompiled':sum(row.get('decompile_completed',False) for row in value[1].values()),'bsim_nonempty':sum(bool(row.get('bsim_features')) for row in value[1].values()),'fid_nonempty':sum(bool(row.get('fid')) for row in value[1].values())} for key,value in captures.items()},'pairs':clean,'metrics':metrics}
    with args.output.open('x') as stream:
        json.dump(report,stream,indent=2,sort_keys=True)
        stream.write('\n')
    print(json.dumps({'capture_totals':report['captures'],'complete_extent_pairs':sum(row['extent_complete'] for row in rows),'usable_pairs':sum(row['ir_usable'] for row in rows),'metrics':{key:{k:v for k,v in value.items() if k!='rows'} for key,value in metrics.items()}},indent=2))


if __name__=='__main__':
    main()
