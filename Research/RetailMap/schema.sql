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
