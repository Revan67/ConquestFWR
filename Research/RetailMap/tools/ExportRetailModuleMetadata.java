// Independently written compatibility metadata extractor. No retail code is exported.
// Separate, hash-pinned Trim.dll export. Use a private independent project.
// Run with analyzeHeadless -process trim.dll -readOnly -noanalysis
//   -scriptPath <this directory> -postScript ExportRetailModuleMetadata.java <new-output.jsonl>
// @category CFW.RetailMap

import ghidra.app.script.GhidraScript;
import ghidra.app.util.PseudoDisassembler;
import ghidra.app.util.PseudoInstruction;
import ghidra.framework.Application;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressRange;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.data.StringDataInstance;
import ghidra.program.model.scalar.Scalar;
import com.google.gson.Gson;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.security.MessageDigest;
import java.util.*;

public class ExportRetailModuleMetadata extends GhidraScript {
    private static final String CANONICAL =
        "AC13625FEF6DA8D553196C894351E93F842BD8FB5344B97FEBA1FD6835E830F0";
    private final Gson gson = new Gson();

    private String rva(Address address) {
        return String.format(Locale.ROOT, "0x%08x", address.subtract(currentProgram.getImageBase()));
    }

    private String moduleIdentity(Address address) {
        return CANONICAL.toLowerCase(Locale.ROOT) + ":" + rva(address);
    }

    private static Map<String, Object> record(Object... pairs) {
        Map<String, Object> result = new LinkedHashMap<>();
        for (int i = 0; i < pairs.length; i += 2) result.put((String) pairs[i], pairs[i + 1]);
        return result;
    }

    private static String hex(byte[] bytes) {
        StringBuilder result = new StringBuilder();
        for (byte b : bytes) result.append(String.format(Locale.ROOT, "%02x", b & 255));
        return result.toString();
    }

    private static String hash(String value) throws Exception {
        return hex(MessageDigest.getInstance("SHA-256").digest(value.getBytes(StandardCharsets.UTF_8)));
    }

    private void line(BufferedWriter out, Map<String, Object> value) throws Exception {
        out.write(gson.toJson(value));
        out.newLine();
    }

    private Map<String, Object> callMetadata(Address site, Address target, String type) {
        Map<String, Object> result = record("site", site.toString(), "target", target.toString(), "type", type);
        Symbol symbol = currentProgram.getSymbolTable().getPrimarySymbol(target);
        if (symbol != null) result.put("target_symbol", symbol.getName(true));
        Function function = currentProgram.getFunctionManager().getFunctionAt(target);
        if (function != null && function.isThunk()) function = function.getThunkedFunction(true);
        if (function != null && function.isExternal()) result.put("external_name", function.getName(true));
        // Some indirect calls target an IAT slot rather than an external function.
        for (Reference reference : currentProgram.getReferenceManager().getReferencesFrom(target)) {
            if (reference.getToAddress().isExternalAddress()) {
                Symbol external = currentProgram.getSymbolTable().getPrimarySymbol(reference.getToAddress());
                if (external != null) result.put("external_name", external.getName(true));
            }
        }
        return result;
    }

    private void seedProbes(Path output, String[] args) throws Exception {
        PseudoDisassembler decoder = new PseudoDisassembler(currentProgram);
        decoder.setRespectExecuteFlag(true);
        try (BufferedWriter out = Files.newBufferedWriter(output, StandardCharsets.UTF_8,
                StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE)) {
            line(out, record("record", "header", "schema", "cfw.retail-seed-probe.v1",
                "image_sha256", currentProgram.getExecutableSHA256().toLowerCase(Locale.ROOT),
                "image_name", currentProgram.getName(), "image_base", currentProgram.getImageBase().toString(),
                "ghidra_version", Application.getApplicationVersion(),
                "policy", "decode one straight-line block per supplied seed; stop at first non-call control transfer or 256 instructions; no analysis/database edits; no inferred function extent"));
            for (int arg = 2; arg < args.length; arg++) {
                monitor.checkCancelled();
                Address seed = toAddr(args[arg]);
                Function containing = currentProgram.getFunctionManager().getFunctionContaining(seed);
                Function at = currentProgram.getFunctionManager().getFunctionAt(seed);
                CodeUnit unit = currentProgram.getListing().getCodeUnitContaining(seed);
                Map<String, Object> result = record("record", "seed_probe", "entry", seed.toString(),
                    "retail_id", moduleIdentity(seed),
                    "defined_function_at", at != null, "defined_instruction_at", currentProgram.getListing().getInstructionAt(seed) != null,
                    "code_unit_kind", unit == null ? "absent" : unit instanceof Instruction ? "instruction" : "data",
                    "code_unit_start", unit == null ? "" : unit.getAddress().toString(),
                    "containing_function", containing == null ? "" : containing.getEntryPoint().toString(),
                    "provenance", at == null ? "address-taken defined instruction block, not saved Function" : "saved Function; bounded instruction block probe",
                    "evidence_status", "UNREVIEWED");
                Map<String, Integer> incoming = new TreeMap<>(), mnemonics = new TreeMap<>(), constants = new TreeMap<>();
                List<Map<String, Object>> incomingEdges = new ArrayList<>();
                for (Reference reference : currentProgram.getReferenceManager().getReferencesTo(seed)) {
                    incoming.merge(reference.getReferenceType().toString(), 1, Integer::sum);
                    incomingEdges.add(record("from", reference.getFromAddress().toString(), "type", reference.getReferenceType().toString()));
                }
                MessageDigest bodyHash = MessageDigest.getInstance("SHA-256"), shapeHash = MessageDigest.getInstance("SHA-256");
                List<Map<String, Object>> calls = new ArrayList<>();
                Set<String> operands = new TreeSet<>();
                Address cursor = seed;
                int count = 0, bytes = 0, unresolvedCalls = 0;
                String stop = "instruction_limit";
                while (count < 256) {
                    monitor.checkCancelled();
                    PseudoInstruction instruction;
                    try { instruction = decoder.disassemble(cursor); }
                    catch (Exception error) { stop = "decode_failure:" + error.getClass().getSimpleName(); break; }
                    if (instruction == null) { stop = "null_decode"; break; }
                    count++;
                    bytes += instruction.getLength();
                    bodyHash.update(instruction.getBytes());
                    String mnemonic = instruction.getMnemonicString().toUpperCase(Locale.ROOT);
                    mnemonics.merge(mnemonic, 1, Integer::sum);
                    StringBuilder shape = new StringBuilder(mnemonic);
                    for (int op = 0; op < instruction.getNumOperands(); op++) {
                        shape.append(':').append(instruction.getOperandType(op));
                        for (Object object : instruction.getOpObjects(op)) {
                            if (object instanceof Scalar) {
                                Scalar scalar = (Scalar) object;
                                constants.merge(scalar.bitLength() + ":0x" + Long.toUnsignedString(scalar.getUnsignedValue(), 16), 1, Integer::sum);
                            } else if (object instanceof Address && ((Address) object).isMemoryAddress()) {
                                operands.add(object.toString());
                            }
                        }
                    }
                    shapeHash.update((shape + "\n").getBytes(StandardCharsets.UTF_8));
                    if (instruction.getFlowType().isCall()) {
                        Set<String> targets = new TreeSet<>();
                        for (Address target : instruction.getFlows()) targets.add(target.toString());
                        for (Reference reference : currentProgram.getReferenceManager().getReferencesFrom(cursor))
                            if (reference.getReferenceType().isCall()) targets.add(reference.getToAddress().toString());
                        for (String target : targets) calls.add(callMetadata(cursor, toAddr(target), "pseudo_or_saved_call_reference"));
                        if (targets.isEmpty()) unresolvedCalls++;
                    }
                    if (!instruction.getFlowType().isCall() && !instruction.getFlowType().isFallthrough()) {
                        stop = "control_transfer:" + instruction.getFlowType();
                        break;
                    }
                    cursor = instruction.getFallThrough();
                    if (cursor == null) { stop = "no_fallthrough"; break; }
                }
                List<Map<String, Object>> data = new ArrayList<>();
                for (String target : operands) data.add(dataReference(toAddr(target)));
                result.putAll(record("incoming_reference_counts", incoming, "incoming_references", incomingEdges,
                    "probe_instruction_count", count,
                    "probe_byte_count", bytes, "probe_byte_sha256", hex(bodyHash.digest()),
                    "probe_instruction_shape_sha256", hex(shapeHash.digest()), "mnemonic_counts", mnemonics,
                    "constants", constants, "call_edges", calls, "unresolved_call_sites", unresolvedCalls,
                    "operand_address_candidates", data, "stop_reason", stop));
                line(out, result);
            }
            long pointerData = 0, missingFunctionPointerData = 0;
            Set<String> missingTargets = new TreeSet<>();
            for (Data datum : currentProgram.getListing().getDefinedData(true)) {
                monitor.checkCancelled();
                Object value = datum.isPointer() ? datum.getValue() : null;
                if (!(value instanceof Address)) continue;
                pointerData++;
                Address target = (Address) value;
                if (currentProgram.getListing().getInstructionAt(target) != null &&
                        currentProgram.getFunctionManager().getFunctionAt(target) == null) {
                    missingFunctionPointerData++;
                    missingTargets.add(target.toString());
                }
            }
            line(out, record("record", "footer", "complete", true, "seeds", args.length - 2,
                "defined_pointer_data_count", pointerData,
                "defined_pointer_data_to_instruction_without_function", missingFunctionPointerData,
                "distinct_defined_instruction_targets_without_function", missingTargets.size(),
                "coverage_policy", "defined top-level pointer Data only; not exhaustive, no target promotion; original pointer-table records required Function targets"));
        }
    }

    private Map<String, Object> dataReference(Address address) throws Exception {
        Data data = currentProgram.getListing().getDataContaining(address);
        Map<String, Object> result = record("address", address.toString());
        if (data == null) {
            result.put("kind", "untyped_data_reference");
            return result;
        }
        result.put("data_start", data.getAddress().toString());
        result.put("data_length", data.getLength());
        // Hash decoded strings; do not export retail expression or raw string bytes.
        StringDataInstance string = StringDataInstance.getStringDataInstance(data);
        if (string != null && string.getStringValue() != null) {
            String value = string.getStringValue();
            result.put("kind", "string_reference");
            result.put("string_sha256_utf8", hash(value));
            result.put("string_characters", value.length());
        } else {
            result.put("kind", "global_reference");
            result.put("type", data.getDataType().getName());
        }
        return result;
    }

    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        boolean seedMode = args.length >= 3 && args[1].equals("--seeds");
        boolean recoveredMode = args.length >= 3 && args[1].equals("--ephemeral-recovered");
        if (args.length != 1 && !seedMode && !recoveredMode)
            throw new IllegalArgumentException("Expected new JSONL path [--seeds|--ephemeral-recovered <VA> ...]");
        // Verify the imported program itself, not merely the similarly named disk file.
        String importedHash = currentProgram.getExecutableSHA256();
        if (importedHash == null || !CANONICAL.equalsIgnoreCase(importedHash))
            throw new IllegalStateException("Imported program SHA-256 is not the canonical retail image: " + importedHash);
        if (!currentProgram.getName().equalsIgnoreCase("trim.dll"))
            throw new IllegalStateException("Unexpected module filename: " + currentProgram.getName());
        Path output = Paths.get(args[0]).toAbsolutePath().normalize();
        if (seedMode) { seedProbes(output, args); return; }
        Set<Address> recoveredEntries = new HashSet<>();
        if (recoveredMode) {
            for (int i = 2; i < args.length; i++) {
                Address entry = toAddr(args[i]);
                if (currentProgram.getFunctionManager().getFunctionAt(entry) == null)
                    throw new IllegalStateException("Declared ephemeral function was not created: " + entry);
                recoveredEntries.add(entry);
            }
        }
        try (BufferedWriter out = Files.newBufferedWriter(output, StandardCharsets.UTF_8,
                StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE)) {
            line(out, record("record", "header", "schema", "cfw.retail-metadata.v1",
                "image_sha256", importedHash.toLowerCase(Locale.ROOT),
                "image_name", currentProgram.getName(), "image_base", currentProgram.getImageBase().toString(),
                "language", currentProgram.getLanguageID().toString(),
                "compiler_spec", currentProgram.getCompilerSpec().getCompilerSpecID().toString(),
                "ghidra_version", Application.getApplicationVersion(),
                "analysis_policy", recoveredMode ?
                    "saved analysis plus explicitly listed ephemeral functions; headless read-only discards changes" :
                    "current independent module analysis; exporter performs no database edits",
                "ephemeral_function_entries", recoveredEntries.size(),
                "identity_policy", "lowercase module SHA-256 plus image-relative RVA; filenames are descriptive only",
                "byte_hash_policy", "SHA-256 of body address ranges in increasing address order, without gaps",
                "instruction_hash_policy", "mnemonic plus ordered Ghidra operand-type integers; no operand values",
                "call_symbol_policy", "saved target symbols; external names resolved from external/thunk function or IAT external reference",
                "classification_policy", "analysis-derived candidates; data references and pointer tables do not establish semantics"));
            long functions = 0;
            for (Function function : currentProgram.getFunctionManager().getFunctions(true)) {
                monitor.checkCancelled();
                MessageDigest bodyHash = MessageDigest.getInstance("SHA-256");
                List<Map<String, Object>> ranges = new ArrayList<>();
                byte[] buffer = new byte[4096];
                for (AddressRange range : function.getBody().getAddressRanges(true)) {
                    ranges.add(record("start", range.getMinAddress().toString(), "end", range.getMaxAddress().toString()));
                    Address position = range.getMinAddress();
                    long remaining = range.getLength();
                    while (remaining > 0) {
                        monitor.checkCancelled();
                        int count = (int) Math.min(remaining, buffer.length);
                        int got = currentProgram.getMemory().getBytes(position, buffer, 0, count);
                        if (got != count) throw new IllegalStateException("Incomplete body read at " + position);
                        bodyHash.update(buffer, 0, got);
                        remaining -= got;
                        if (remaining > 0) position = position.add(got);
                    }
                }
                MessageDigest shapeHash = MessageDigest.getInstance("SHA-256");
                MessageDigest mnemonicHash = MessageDigest.getInstance("SHA-256");
                Map<String, Integer> mnemonics = new TreeMap<>();
                Map<String, Integer> constants = new TreeMap<>();
                Set<String> calls = new TreeSet<>();
                Set<String> callers = new TreeSet<>();
                Set<String> dataTargets = new TreeSet<>();
                List<Map<String, Object>> edges = new ArrayList<>();
                int instructionCount = 0, unresolvedCalls = 0;
                for (Instruction instruction : currentProgram.getListing().getInstructions(function.getBody(), true)) {
                    instructionCount++;
                    String mnemonic = instruction.getMnemonicString().toUpperCase(Locale.ROOT);
                    mnemonics.merge(mnemonic, 1, Integer::sum);
                    mnemonicHash.update((mnemonic + "\n").getBytes(StandardCharsets.UTF_8));
                    StringBuilder shape = new StringBuilder(mnemonic);
                    for (int i = 0; i < instruction.getNumOperands(); i++) {
                        shape.append(':').append(instruction.getOperandType(i));
                        for (Object operand : instruction.getOpObjects(i)) {
                            if (operand instanceof Scalar) {
                                Scalar scalar = (Scalar) operand;
                                constants.merge(scalar.bitLength() + ":0x" + Long.toUnsignedString(scalar.getUnsignedValue(), 16), 1, Integer::sum);
                            }
                        }
                    }
                    shape.append('\n');
                    shapeHash.update(shape.toString().getBytes(StandardCharsets.UTF_8));
                    boolean resolvedCall = false;
                    for (Reference reference : instruction.getReferencesFrom()) {
                        Address target = reference.getToAddress();
                        if (reference.getReferenceType().isCall()) {
                            calls.add(target.toString());
                            edges.add(callMetadata(instruction.getAddress(), target, reference.getReferenceType().toString()));
                            resolvedCall = true;
                        } else if (reference.getReferenceType().isData() && target.isMemoryAddress()) {
                            dataTargets.add(target.toString());
                        }
                    }
                    if (instruction.getFlowType().isCall() && !resolvedCall) unresolvedCalls++;
                }
                List<Map<String, Object>> data = new ArrayList<>();
                for (String target : dataTargets) data.add(dataReference(toAddr(target)));
                for (Reference reference : currentProgram.getReferenceManager().getReferencesTo(function.getEntryPoint())) {
                    if (reference.getReferenceType().isCall()) {
                        Function caller = currentProgram.getFunctionManager().getFunctionContaining(reference.getFromAddress());
                        if (caller != null) callers.add(caller.getEntryPoint().toString());
                    }
                }
                line(out, record("record", "function", "retail_id", moduleIdentity(function.getEntryPoint()),
                    "entry", function.getEntryPoint().toString(),
                    "rva", rva(function.getEntryPoint()), "module_sha256", importedHash.toLowerCase(Locale.ROOT),
                    "function_provenance", recoveredEntries.contains(function.getEntryPoint()) ?
                        "EPHEMERAL_RECOVERED_CANDIDATE" : "SAVED_ANALYSIS_FUNCTION",
                    "name", function.getName(), "name_source", function.getSymbol().getSource().toString(),
                    "signature_sha256", hash(function.getPrototypeString(false, false)),
                    "body_ranges", ranges, "body_size", function.getBody().getNumAddresses(),
                    "body_sha256", hex(bodyHash.digest()), "instruction_count", instructionCount,
                    "instruction_shape_sha256", hex(shapeHash.digest()), "mnemonic_sha256", hex(mnemonicHash.digest()),
                    "mnemonic_counts", mnemonics, "constants", constants,
                    "thunk", function.isThunk(), "external", function.isExternal(),
                    "calls", calls, "callers", callers, "call_edges", edges, "unresolved_call_sites", unresolvedCalls,
                    "data_references", data));
                functions++;
            }
            // Consecutive defined pointers into known functions are only vtable candidates.
            // No claim about RTTI, ownership, class identity, or missing undefined tables.
            long pointerTables = 0;
            List<String> tableTargets = new ArrayList<>();
            Address tableStart = null, previous = null;
            for (Data datum : currentProgram.getListing().getDefinedData(true)) {
                monitor.checkCancelled();
                Object value = datum.isPointer() ? datum.getValue() : null;
                Address target = value instanceof Address ? (Address) value : null;
                boolean functionPointer = target != null && currentProgram.getFunctionManager().getFunctionAt(target) != null;
                boolean contiguous = previous != null && previous.add(currentProgram.getDefaultPointerSize()).equals(datum.getAddress());
                if (!functionPointer || (!tableTargets.isEmpty() && !contiguous)) {
                    if (tableTargets.size() >= 2) {
                        line(out, record("record", "pointer_table_candidate", "address", tableStart.toString(), "targets", tableTargets));
                        pointerTables++;
                    }
                    tableTargets = new ArrayList<>();
                    tableStart = null;
                }
                if (functionPointer) {
                    if (tableStart == null) tableStart = datum.getAddress();
                    tableTargets.add(target.toString());
                    previous = datum.getAddress();
                } else previous = null;
            }
            if (tableTargets.size() >= 2) {
                line(out, record("record", "pointer_table_candidate", "address", tableStart.toString(), "targets", tableTargets));
                pointerTables++;
            }
            line(out, record("record", "footer", "complete", true, "functions", functions, "pointer_table_candidates", pointerTables));
            println("Metadata complete: " + functions + " functions; " + output);
        }
        // A failed/cancelled run has no complete footer and must not be used as evidence.
    }
}
