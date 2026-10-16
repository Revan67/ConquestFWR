// Bounded, read-only-project Trim function/table ownership probe.
// Execute with analyzeHeadless -readOnly -noanalysis; never save the project.
// @category CFW.RetailMap

import ghidra.app.script.GhidraScript;
import ghidra.framework.Application;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressRange;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.symbol.Reference;
import com.google.gson.Gson;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class ProbeTrimPatchWindow extends GhidraScript {
    private static final String CANONICAL =
        "AC13625FEF6DA8D553196C894351E93F842BD8FB5344B97FEBA1FD6835E830F0";
    private final Gson gson = new Gson();

    private static Map<String, Object> row(Object... pairs) {
        Map<String, Object> result = new LinkedHashMap<>();
        for (int i = 0; i < pairs.length; i += 2) result.put((String) pairs[i], pairs[i + 1]);
        return result;
    }

    private List<Map<String, Object>> references(Address address) {
        List<Map<String, Object>> result = new ArrayList<>();
        for (Reference reference : currentProgram.getReferenceManager().getReferencesTo(address)) {
            Address from = reference.getFromAddress();
            Function owner = currentProgram.getFunctionManager().getFunctionContaining(from);
            result.add(row("from", from.toString(), "type", reference.getReferenceType().toString(),
                "source", reference.getSource().toString(), "owner", owner == null ? null : owner.getEntryPoint().toString()));
        }
        return result;
    }

    private Map<String, Object> current(Address address) {
        Function exact = currentProgram.getFunctionManager().getFunctionAt(address);
        Function containing = currentProgram.getFunctionManager().getFunctionContaining(address);
        Instruction instruction = currentProgram.getListing().getInstructionAt(address);
        return row("address", address.toString(), "function_at", exact == null ? null : exact.getEntryPoint().toString(),
            "function_containing", containing == null ? null : containing.getEntryPoint().toString(),
            "instruction_at", instruction == null ? null : instruction.getMnemonicString(),
            "references_to", references(address));
    }

    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("Expected new private JSONL output path");
        if (!CANONICAL.equalsIgnoreCase(currentProgram.getExecutableSHA256()))
            throw new IllegalStateException("Imported program is not canonical retail Trim");
        Path output = Paths.get(args[0]).toAbsolutePath().normalize();
        Address entry = toAddr("1003C7F0");
        Address patch = toAddr("1003C865");
        Address tableSlot = toAddr("100A4A78");
        List<Map<String, Object>> rows = new ArrayList<>();
        rows.add(row("record", "header", "schema", "cfw.trim-patch-ghidra.v1",
            "image_sha256", currentProgram.getExecutableSHA256(), "ghidra", Application.getApplicationVersion(),
            "policy", "ephemeral analysis only; no saved project mutation; no identity promotion"));
        for (String value : new String[]{"1003C7F0", "1003C865", "1003C88B", "100A4A78"})
            rows.add(row("record", "saved_state", "site", current(toAddr(value))));
        boolean disassemblyChanged = false;
        if (currentProgram.getListing().getInstructionAt(entry) == null) {
            disassemble(entry);
            disassemblyChanged = true;
        }
        Function candidate = currentProgram.getFunctionManager().getFunctionAt(entry);
        String error = null;
        if (candidate == null) {
            try { candidate = createFunction(entry, null); }
            catch (Exception e) { error = e.getClass().getSimpleName() + ": " + e.getMessage(); }
        }
        Map<String, Object> recovered = row("record", "ephemeral_recovery", "entry", entry.toString(),
            "disassembled", disassemblyChanged, "created", candidate != null, "error", error,
            "patch_inside", candidate != null && candidate.getBody().contains(patch));
        if (candidate != null) {
            List<Map<String, Object>> ranges = new ArrayList<>();
            for (AddressRange range : candidate.getBody().getAddressRanges(true))
                ranges.add(row("begin", range.getMinAddress().toString(), "end", range.getMaxAddress().toString()));
            recovered.put("ranges", ranges);
            recovered.put("body_size", candidate.getBody().getNumAddresses());
            recovered.put("callers", references(entry));
        }
        rows.add(recovered);
        Memory memory = currentProgram.getMemory();
        List<Map<String, Object>> table = new ArrayList<>();
        for (int delta = -32; delta <= 32; delta += 4) {
            Address slot = tableSlot.add(delta);
            int target = memory.getInt(slot);
            Address targetAddress = toAddr(String.format(Locale.ROOT, "%08X", target));
            Function targetFunction = currentProgram.getFunctionManager().getFunctionAt(targetAddress);
            table.add(row("slot", slot.toString(), "target", targetAddress.toString(),
                "executable_target", memory.getBlock(targetAddress) != null && memory.getBlock(targetAddress).isExecute(),
                "function_entry", targetFunction == null ? null : targetFunction.getEntryPoint().toString(),
                "slot_references", references(slot)));
        }
        rows.add(row("record", "table_neighborhood", "center", tableSlot.toString(), "slots", table));
        rows.add(row("record", "footer", "complete", true));
        try (BufferedWriter writer = Files.newBufferedWriter(output, StandardCharsets.UTF_8,
                StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE)) {
            for (Map<String, Object> value : rows) {
                writer.write(gson.toJson(value));
                writer.newLine();
            }
        }
    }
}
