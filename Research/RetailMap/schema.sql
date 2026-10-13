-- ConquestFWR retail/source correspondence metadata schema.
-- Contains independently recorded identifiers and fingerprints, not retail code.
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS index_runs (
    run_id TEXT PRIMARY KEY,
    kind TEXT NOT NULL CHECK (kind IN ('retail', 'seed', 'source', 'match')),
    created_utc TEXT NOT NULL,
    input_identity TEXT NOT NULL,
    metadata_sha256 TEXT NOT NULL,
    item_count INTEGER NOT NULL DEFAULT 0,
    notes TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS retail_functions (
    retail_id TEXT PRIMARY KEY,
    run_id TEXT NOT NULL REFERENCES index_runs(run_id),
    module_sha256 TEXT NOT NULL,
    module_name TEXT NOT NULL,
    image_base INTEGER NOT NULL,
    va INTEGER NOT NULL,
    rva INTEGER NOT NULL,
    size INTEGER,
    ghidra_name TEXT,
    recovered_name TEXT,
    signature TEXT,
    calling_convention TEXT,
    parameter_count INTEGER,
    block_count INTEGER,
    instruction_count INTEGER,
    cyclomatic_complexity INTEGER,
    byte_sha256 TEXT,
    normalized_instruction_sha256 TEXT,
    pseudocode_fingerprint TEXT,
    evidence_status TEXT NOT NULL DEFAULT 'UNREVIEWED',
    UNIQUE (module_sha256, rva)
);

CREATE TABLE IF NOT EXISTS source_functions (
    source_id TEXT PRIMARY KEY,
    run_id TEXT NOT NULL REFERENCES index_runs(run_id),
    source_commit TEXT NOT NULL,
    path TEXT NOT NULL,
    compile_target TEXT NOT NULL,
    qualified_name TEXT NOT NULL,
    signature TEXT,
    start_line INTEGER,
    end_line INTEGER,
    source_fingerprint TEXT,
    source_blob_sha256 TEXT NOT NULL,
    worktree_dirty INTEGER NOT NULL CHECK (worktree_dirty IN (0,1)),
    worktree_identity TEXT NOT NULL,
    compiled_state TEXT NOT NULL CHECK (compiled_state IN ('COMPILED', 'EXCLUDED', 'UNKNOWN')),
    UNIQUE (source_commit, path, qualified_name, start_line, compile_target)
);

CREATE TABLE IF NOT EXISTS features (
    owner_kind TEXT NOT NULL CHECK (owner_kind IN ('retail', 'source')),
    owner_id TEXT NOT NULL,
    feature_kind TEXT NOT NULL CHECK (feature_kind IN
        ('string_hash', 'numeric_constant', 'call', 'caller', 'callee',
         'global', 'import', 'vtable', 'switch', 'field_offset', 'symbol')),
    feature_value TEXT NOT NULL,
    provenance TEXT NOT NULL DEFAULT '',
    PRIMARY KEY (owner_kind, owner_id, feature_kind, feature_value)
);
CREATE INDEX IF NOT EXISTS features_lookup ON features(feature_kind, feature_value);

CREATE TABLE IF NOT EXISTS retail_call_edges (
    retail_id TEXT NOT NULL REFERENCES retail_functions(retail_id),
    site_va INTEGER NOT NULL,
    target TEXT NOT NULL,
    reference_type TEXT NOT NULL,
    external_name TEXT,
    PRIMARY KEY (retail_id, site_va, target)
);

-- These are address tables referencing saved-analysis Functions. They are NOT
-- automatically vtables; a separate reviewed vtables table exists below.
CREATE TABLE IF NOT EXISTS retail_pointer_table_candidates (
    module_sha256 TEXT NOT NULL,
    table_va INTEGER NOT NULL,
    targets_json TEXT NOT NULL,
    provenance TEXT NOT NULL,
    PRIMARY KEY (module_sha256, table_va)
);

-- A bounded code block at an address-taken target is not a function. Keep
-- these leads separate from retail_functions and reviewed correspondences.
CREATE TABLE IF NOT EXISTS retail_code_leads (
    module_sha256 TEXT NOT NULL,
    va INTEGER NOT NULL,
    defined_function_at INTEGER NOT NULL CHECK (defined_function_at IN (0,1)),
    probe_instruction_count INTEGER NOT NULL,
    probe_byte_count INTEGER NOT NULL,
    probe_byte_sha256 TEXT NOT NULL,
    probe_shape_sha256 TEXT NOT NULL,
    stop_reason TEXT NOT NULL,
    incoming_references_json TEXT NOT NULL,
    evidence_status TEXT NOT NULL DEFAULT 'UNREVIEWED',
    PRIMARY KEY (module_sha256, va)
);

CREATE TABLE IF NOT EXISTS correspondences (
    correspondence_id INTEGER PRIMARY KEY,
    retail_id TEXT NOT NULL REFERENCES retail_functions(retail_id),
    source_id TEXT NOT NULL REFERENCES source_functions(source_id),
    confidence INTEGER NOT NULL CHECK (confidence BETWEEN 0 AND 100),
    relationship TEXT NOT NULL CHECK (relationship IN
        ('EXACT_FUNCTION', 'NEAR_FUNCTION', 'THUNK', 'ADJUSTOR', 'INLINED_FRAGMENT',
         'SPLIT', 'MERGED', 'SEMANTIC_ONLY', 'UNKNOWN')),
    status TEXT NOT NULL CHECK (status IN
        ('EXACT/NEAR MATCH', 'MODIFIED', 'RETAIL-ONLY', 'CQ2-ONLY',
         'SPLIT', 'MERGED', 'UNKNOWN', 'CONTRADICTED')),
    semantic_difference TEXT NOT NULL DEFAULT 'UNREVIEWED',
    match_method TEXT NOT NULL,
    evidence_ref TEXT NOT NULL DEFAULT '',
    reviewer TEXT NOT NULL DEFAULT '',
    reviewed_utc TEXT,
    notes TEXT NOT NULL DEFAULT '',
    UNIQUE (retail_id, source_id, relationship)
);
CREATE INDEX IF NOT EXISTS correspondences_retail ON correspondences(retail_id, confidence DESC);
CREATE INDEX IF NOT EXISTS correspondences_source ON correspondences(source_id, confidence DESC);

-- Automated leads are deliberately separate from reviewed correspondences.
CREATE TABLE IF NOT EXISTS match_candidates (
    retail_id TEXT NOT NULL REFERENCES retail_functions(retail_id),
    source_id TEXT NOT NULL REFERENCES source_functions(source_id),
    matcher_run_id TEXT NOT NULL REFERENCES index_runs(run_id),
    rank INTEGER NOT NULL,
    score REAL NOT NULL,
    signal_groups INTEGER NOT NULL,
    shared_features_json TEXT NOT NULL,
    warning TEXT NOT NULL DEFAULT '',
    PRIMARY KEY (retail_id, source_id, matcher_run_id)
);
CREATE INDEX IF NOT EXISTS match_candidates_rank ON match_candidates(matcher_run_id, retail_id, rank);

-- Independent function-identity reviews may precede bulk extractor coverage
-- (notably address-taken code and other retail modules). This does not assert
-- implementation equivalence or gameplay parity.
CREATE TABLE IF NOT EXISTS reviewed_identities (
    module_sha256 TEXT NOT NULL,
    module_name TEXT NOT NULL,
    va INTEGER NOT NULL,
    source_id TEXT NOT NULL REFERENCES source_functions(source_id),
    source_blob_sha256 TEXT NOT NULL,
    relationship TEXT NOT NULL,
    confidence INTEGER NOT NULL CHECK (confidence BETWEEN 0 AND 100),
    evidence_type TEXT NOT NULL,
    evidence_ref TEXT NOT NULL,
    reviewer TEXT NOT NULL,
    review_date TEXT NOT NULL,
    semantic_parity TEXT NOT NULL DEFAULT 'UNTESTED',
    review_manifest_sha256 TEXT NOT NULL,
    PRIMARY KEY (module_sha256, va, source_id)
);

CREATE TABLE IF NOT EXISTS structures (
    structure_id TEXT PRIMARY KEY,
    retail_module_sha256 TEXT NOT NULL,
    retail_name TEXT,
    retail_size INTEGER,
    source_type TEXT,
    source_size INTEGER,
    status TEXT NOT NULL DEFAULT 'OPEN',
    provenance TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS structure_fields (
    structure_id TEXT NOT NULL REFERENCES structures(structure_id),
    retail_offset INTEGER NOT NULL,
    retail_width INTEGER,
    retail_field TEXT,
    source_field TEXT,
    transformation TEXT NOT NULL DEFAULT 'UNMAPPED',
    confidence INTEGER CHECK (confidence BETWEEN 0 AND 100),
    provenance TEXT NOT NULL,
    PRIMARY KEY (structure_id, retail_offset)
);

CREATE TABLE IF NOT EXISTS globals (
    global_id TEXT PRIMARY KEY,
    retail_module_sha256 TEXT NOT NULL,
    retail_va INTEGER NOT NULL,
    source_symbol TEXT,
    confidence INTEGER CHECK (confidence BETWEEN 0 AND 100),
    provenance TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS vtables (
    vtable_id TEXT NOT NULL,
    slot INTEGER NOT NULL,
    retail_module_sha256 TEXT NOT NULL,
    retail_vtable_va INTEGER NOT NULL,
    retail_id TEXT REFERENCES retail_functions(retail_id),
    source_id TEXT REFERENCES source_functions(source_id),
    confidence INTEGER CHECK (confidence BETWEEN 0 AND 100),
    provenance TEXT NOT NULL,
    PRIMARY KEY (vtable_id, slot)
);

CREATE TABLE IF NOT EXISTS unresolved (
    question_id TEXT PRIMARY KEY,
    subsystem TEXT NOT NULL,
    question TEXT NOT NULL,
    last_proven_point TEXT NOT NULL DEFAULT '',
    next_observation TEXT NOT NULL,
    evidence_ref TEXT NOT NULL DEFAULT '',
    status TEXT NOT NULL DEFAULT 'OPEN'
);

-- M2.5 compiled-machine bridge. This is additive: existing v1 runs, ranked
-- leads and reviewed identities are preserved byte-for-byte in the frozen
-- local v1 database before this schema is applied to a separate copy.
CREATE TABLE IF NOT EXISTS binary_modules (
    module_sha256 TEXT PRIMARY KEY,
    module_name TEXT NOT NULL,
    origin TEXT NOT NULL CHECK (origin IN ('RETAIL', 'RECONSTRUCTED')),
    configuration TEXT,
    module_bytes INTEGER NOT NULL,
    pe_timestamp INTEGER NOT NULL,
    image_base INTEGER NOT NULL,
    architecture TEXT NOT NULL,
    linker_version TEXT,
    manifest_sha256 TEXT,
    build_input_fingerprint TEXT,
    pdb_sha256 TEXT,
    pdb_guid TEXT,
    pdb_age INTEGER,
    provenance TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS compiled_functions (
    module_sha256 TEXT NOT NULL REFERENCES binary_modules(module_sha256),
    rva INTEGER NOT NULL,
    va INTEGER NOT NULL,
    byte_extent INTEGER,
    pdb_symbols_json TEXT NOT NULL,
    source_frames_json TEXT NOT NULL,
    relationship TEXT NOT NULL CHECK (relationship IN
        ('ONE_TO_ONE', 'SOURCE_INLINED', 'BINARY_FOLDED', 'SOURCE_SPLIT',
         'BINARY_THUNK', 'UNKNOWN')),
    source_identity_status TEXT NOT NULL CHECK (source_identity_status IN
        ('PDB_SOURCE_BYTES_VERIFIED', 'MAP_OBJECT_SOURCE_BYTES_VERIFIED', 'UNKNOWN')),
    provenance TEXT NOT NULL,
    PRIMARY KEY (module_sha256, rva)
);

CREATE TABLE IF NOT EXISTS compiled_source_links (
    module_sha256 TEXT NOT NULL,
    rva INTEGER NOT NULL,
    source_id TEXT REFERENCES source_functions(source_id),
    source_path TEXT NOT NULL,
    source_line INTEGER,
    qualified_name TEXT,
    compiler_source_sha256 TEXT,
    current_source_sha256 TEXT,
    relationship TEXT NOT NULL CHECK (relationship IN
        ('ONE_TO_ONE', 'SOURCE_INLINED', 'BINARY_FOLDED', 'SOURCE_SPLIT',
         'BINARY_THUNK', 'UNKNOWN')),
    provenance TEXT NOT NULL,
    FOREIGN KEY (module_sha256, rva) REFERENCES compiled_functions(module_sha256, rva),
    PRIMARY KEY (module_sha256, rva, source_path, source_line, qualified_name)
);

CREATE TABLE IF NOT EXISTS machine_features (
    owner_kind TEXT NOT NULL CHECK (owner_kind IN ('RETAIL', 'RECONSTRUCTED')),
    module_sha256 TEXT NOT NULL,
    rva INTEGER NOT NULL,
    feature_family TEXT NOT NULL,
    feature_value TEXT NOT NULL,
    occurrence INTEGER NOT NULL DEFAULT 1,
    extractor_sha256 TEXT NOT NULL,
    normalization_version TEXT NOT NULL,
    provenance TEXT NOT NULL,
    PRIMARY KEY (owner_kind, module_sha256, rva, feature_family,
                 feature_value, normalization_version)
);

CREATE TABLE IF NOT EXISTS matcher_versions (
    matcher_version TEXT PRIMARY KEY,
    validation_status TEXT NOT NULL CHECK (validation_status IN
        ('MATCHER_V1_UNVALIDATED', 'MATCHER_V2_DEVELOPMENT',
         'MATCHER_V2_VALIDATED')),
    implementation_sha256 TEXT NOT NULL,
    feature_schema TEXT NOT NULL,
    scoring_configuration_json TEXT NOT NULL,
    evidence_ref TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS matcher_v2_runs (
    run_id TEXT PRIMARY KEY,
    matcher_version TEXT NOT NULL REFERENCES matcher_versions(matcher_version),
    retail_input_sha256 TEXT NOT NULL,
    compiled_input_sha256 TEXT NOT NULL,
    source_bridge_sha256 TEXT NOT NULL,
    candidate_generation_json TEXT NOT NULL,
    scoring_json TEXT NOT NULL,
    validation_state TEXT NOT NULL DEFAULT 'UNVALIDATED',
    notes TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS ranked_machine_links (
    run_id TEXT NOT NULL REFERENCES matcher_v2_runs(run_id),
    retail_module_sha256 TEXT NOT NULL,
    retail_rva INTEGER NOT NULL,
    current_module_sha256 TEXT NOT NULL,
    current_rva INTEGER NOT NULL,
    rank INTEGER NOT NULL,
    identity_score REAL NOT NULL,
    contribution_json TEXT NOT NULL,
    disposition TEXT NOT NULL CHECK (disposition IN
        ('AUTO-CANDIDATE HIGH', 'AUTO-CANDIDATE', 'AMBIGUOUS',
         'UNMATCHED', 'UNKNOWN')),
    PRIMARY KEY (run_id, retail_module_sha256, retail_rva,
                 current_module_sha256, current_rva)
);

CREATE TABLE IF NOT EXISTS matcher_control_results (
    run_id TEXT NOT NULL REFERENCES matcher_v2_runs(run_id),
    control_id TEXT NOT NULL,
    control_kind TEXT NOT NULL CHECK (control_kind IN
        ('DEVELOPMENT_POSITIVE', 'HELD_OUT_POSITIVE', 'HARD_NEGATIVE')),
    true_counterpart_rank INTEGER,
    top_score REAL,
    true_score REAL,
    hard_negative_margin REAL,
    candidates_considered INTEGER NOT NULL,
    outcome TEXT NOT NULL,
    evidence_ref TEXT NOT NULL,
    PRIMARY KEY (run_id, control_id)
);

CREATE TABLE IF NOT EXISTS reviewed_machine_correspondences (
    retail_module_sha256 TEXT NOT NULL,
    retail_rva INTEGER NOT NULL,
    current_module_sha256 TEXT NOT NULL,
    current_rva INTEGER NOT NULL,
    source_id TEXT REFERENCES source_functions(source_id),
    identity_confidence INTEGER NOT NULL CHECK (identity_confidence BETWEEN 0 AND 100),
    semantic_similarity TEXT NOT NULL DEFAULT 'UNREVIEWED',
    restoration_status TEXT NOT NULL DEFAULT 'UNREVIEWED',
    module_relationship TEXT NOT NULL CHECK (module_relationship IN
        ('SAME_MODULE', 'MOVED_MODULE', 'INLINED', 'SPLIT', 'MERGED',
         'RETAIL_ONLY', 'CQ2_ONLY', 'UNKNOWN')),
    reviewer TEXT NOT NULL,
    evidence_ref TEXT NOT NULL,
    PRIMARY KEY (retail_module_sha256, retail_rva,
                 current_module_sha256, current_rva)
);

-- Matcher v3 research is deliberately additive. The independently reviewed
-- anchors below are not matcher output, and holdout identities are never
-- graph-propagation seeds while those controls are being evaluated.
CREATE TABLE IF NOT EXISTS correspondence_anchors (
    anchor_id TEXT PRIMARY KEY,
    retail_module_sha256 TEXT NOT NULL,
    retail_rva INTEGER NOT NULL,
    current_module_sha256 TEXT NOT NULL,
    current_rva INTEGER NOT NULL,
    source_id TEXT,
    evidence_ref TEXT NOT NULL,
    confidence INTEGER NOT NULL CHECK (confidence BETWEEN 0 AND 100),
    independent_status TEXT NOT NULL CHECK (independent_status IN
        ('VERIFIED', 'REVIEWED_HIGH_CONFIDENCE')),
    graph_propagation INTEGER NOT NULL CHECK (graph_propagation IN (0,1)),
    validation_role TEXT NOT NULL CHECK (validation_role IN
        ('CALIBRATION', 'HOLDOUT', 'NONCONTROL')),
    provenance TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS machine_call_edges_v3 (
    module_sha256 TEXT NOT NULL,
    owner_rva INTEGER NOT NULL,
    site_rva INTEGER NOT NULL,
    edge_kind TEXT NOT NULL,
    target_rva INTEGER,
    import_name TEXT,
    evidence_ref TEXT NOT NULL,
    PRIMARY KEY (module_sha256, owner_rva, site_rva, edge_kind)
);
CREATE INDEX IF NOT EXISTS machine_call_edges_v3_target
    ON machine_call_edges_v3(module_sha256, target_rva);

-- Pointer-table candidates do not become vtables by inference. These rows
-- require explicit constructor/class evidence and retain each slot's basis.
CREATE TABLE IF NOT EXISTS machine_vtable_slots_v3 (
    module_sha256 TEXT NOT NULL,
    table_rva INTEGER NOT NULL,
    slot INTEGER NOT NULL,
    target_rva INTEGER NOT NULL,
    ownership_evidence TEXT NOT NULL,
    slot_evidence TEXT NOT NULL,
    PRIMARY KEY (module_sha256, table_rva, slot)
);
CREATE TABLE IF NOT EXISTS vtable_relationships_v3 (
    retail_module_sha256 TEXT NOT NULL,
    retail_table_rva INTEGER NOT NULL,
    current_module_sha256 TEXT NOT NULL,
    current_table_rva INTEGER NOT NULL,
    class_relationship TEXT NOT NULL,
    slot_alignment TEXT NOT NULL,
    alignment_evidence TEXT NOT NULL,
    confidence INTEGER NOT NULL CHECK (confidence BETWEEN 0 AND 100),
    PRIMARY KEY (retail_module_sha256, retail_table_rva,
                 current_module_sha256, current_table_rva)
);

CREATE TABLE IF NOT EXISTS matcher_v3_runs (
    run_id TEXT PRIMARY KEY,
    parent_v2_run_id TEXT NOT NULL REFERENCES matcher_v2_runs(run_id),
    implementation_sha256 TEXT NOT NULL,
    controls_sha256 TEXT NOT NULL,
    contextual_inputs_json TEXT NOT NULL,
    scoring_json TEXT NOT NULL,
    validation_state TEXT NOT NULL,
    promotion_count INTEGER NOT NULL DEFAULT 0 CHECK (promotion_count=0),
    notes TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS matcher_v3_control_results (
    run_id TEXT NOT NULL REFERENCES matcher_v3_runs(run_id),
    control_id TEXT NOT NULL,
    ablation TEXT NOT NULL,
    true_counterpart_rank INTEGER,
    equivalence_set_json TEXT NOT NULL,
    best_score REAL,
    second_score REAL,
    margin REAL,
    contextual_evidence_count INTEGER NOT NULL,
    outcome TEXT NOT NULL,
    evidence_json TEXT NOT NULL,
    PRIMARY KEY (run_id, control_id, ablation)
);
CREATE TABLE IF NOT EXISTS matcher_v3_candidate_leads (
    run_id TEXT NOT NULL REFERENCES matcher_v3_runs(run_id),
    control_id TEXT NOT NULL,
    ablation TEXT NOT NULL,
    current_module_sha256 TEXT NOT NULL,
    current_rva INTEGER NOT NULL,
    rank INTEGER,
    score REAL NOT NULL,
    body_score REAL NOT NULL,
    context_json TEXT NOT NULL,
    disposition TEXT NOT NULL,
    PRIMARY KEY (run_id, control_id, ablation, current_module_sha256,
                 current_rva)
);
