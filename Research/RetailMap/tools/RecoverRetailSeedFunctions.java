// Experimental, ephemeral function-boundary probe for explicitly supplied VAs.
// Run headless with -readOnly -noanalysis; never save the Ghidra project.
// Exports metadata/hashes only, not retail bytes or pseudocode.
// @category CFW.RetailMap

import ghidra.app.script.GhidraScript;
import ghidra.framework.Application;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressRange;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import com.google.gson.Gson;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.security.MessageDigest;
import java.util.*;

public class RecoverRetailSeedFunctions extends GhidraScript {
    private static final String CANONICAL =
        "ED8741C0CB727A43B7DE7BE0E73566B04FBC1CBD58B54A62D538E3F39DCB3F52";
    private final Gson gson = new Gson();

    private static Map<String, Object> row(Object... pairs) {
        Map<String, Object> value = new LinkedHashMap<>();
        for (int i = 0; i < pairs.length; i += 2) value.put((String) pairs[i], pairs[i + 1]);
        return value;
    }

    private static String hex(byte[] bytes) {
        StringBuilder result = new StringBuilder();
        for (byte value : bytes) result.append(String.format(Locale.ROOT, "%02x", value & 255));
        return result.toString();
    }

    private void write(BufferedWriter output, Map<String, Object> value) throws Exception {
        output.write(gson.toJson(value));
        output.newLine();
    }

    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) throw new IllegalArgumentException("Expected fresh JSONL path and one or more VAs");
        String imageHash = currentProgram.getExecutableSHA256();
        if (imageHash == null || !CANONICAL.equalsIgnoreCase(imageHash))
            throw new IllegalStateException("Imported program is not the canonical retail EXE");
        Path output = Paths.get(args[0]).toAbsolutePath().normalize();
        try (BufferedWriter stream = Files.newBufferedWriter(output, StandardCharsets.UTF_8,
                StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE)) {
            write(stream, row("record", "header", "schema", "cfw.retail-ephemeral-functions.v1",
                "image_sha256", imageHash.toLowerCase(Locale.ROOT),
                "image_name", currentProgram.getName(), "image_base", currentProgram.getImageBase().toString(),
                "ghidra_version", Application.getApplicationVersion(),
                "policy", "explicit VAs only; create Function in read-only ephemeral session; no project save; identity and semantics unreviewed"));
            int recovered = 0;
            for (int i = 1; i < args.length; i++) {
                monitor.checkCancelled();
                Address address = toAddr(args[i]);
                Function before = currentProgram.getFunctionManager().getFunctionAt(address);
                Function function = before;
                String error = "";
                if (function == null) {
                    try { function = createFunction(address, null); }
                    catch (Exception exception) { error = exception.getClass().getSimpleName() + ": " + exception.getMessage(); }
                }
                Map<String, Object> result = row("record", "seed", "entry", address.toString(),
                    "saved_function_before", before != null,
                    "ephemeral_function_created", before == null && function != null,
                    "error", error,
                    "evidence_status", "UNREVIEWED");
                if (function != null) {
                    recovered++;
                    List<Map<String, String>> ranges = new ArrayList<>();
                    MessageDigest bytes = MessageDigest.getInstance("SHA-256");
                    long instructionCount = 0;
                    for (AddressRange range : function.getBody().getAddressRanges(true)) {
                        ranges.add(rowString("start", range.getMinAddress().toString(),
                                             "end", range.getMaxAddress().toString()));
                        Address cursor = range.getMinAddress();
                        while (cursor.compareTo(range.getMaxAddress()) <= 0) {
                            Instruction instruction = currentProgram.getListing().getInstructionAt(cursor);
                            if (instruction == null)
                                throw new IllegalStateException("Function body includes undefined instruction at " + cursor);
                            bytes.update(instruction.getBytes());
                            instructionCount++;
                            cursor = instruction.getMaxAddress().next();
                            if (cursor == null) break;
                        }
                    }
                    result.put("body_ranges", ranges);
                    result.put("body_size", function.getBody().getNumAddresses());
                    result.put("instruction_count", instructionCount);
                    result.put("body_sha256", hex(bytes.digest()));
                    result.put("name", function.getName());
                }
                write(stream, result);
            }
            write(stream, row("record", "footer", "complete", true, "seeds", args.length - 1,
                              "functions_present_or_created", recovered,
                              "saved_project_policy", "headless -readOnly; no save"));
        }
    }

    private static Map<String, String> rowString(String a, String av, String b, String bv) {
        Map<String, String> value = new LinkedHashMap<>();
        value.put(a, av);
        value.put(b, bv);
        return value;
    }
}
