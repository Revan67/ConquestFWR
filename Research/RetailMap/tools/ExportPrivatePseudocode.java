// Independently written private-evidence helper. Retail pseudocode output is
// restricted to ignored local evidence and must not be committed.
// Run canonical projects with -readOnly -noanalysis and explicit addresses.
// @category CFW.RetailMap
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.framework.Application;
import ghidra.program.model.listing.Function;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;

public class ExportPrivatePseudocode extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 3) {
            throw new IllegalArgumentException("output expected-sha256 VA...");
        }
        if (!args[1].equalsIgnoreCase(currentProgram.getExecutableSHA256())) {
            throw new IllegalStateException("Imported image hash mismatch");
        }
        Path output = Paths.get(args[0]).toAbsolutePath().normalize();
        if (!output.toString().replace('\\', '/').contains("/Evidence/RetailMap/local/")) {
            throw new IllegalArgumentException("Private evidence output required");
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.toggleCCode(true);
        decompiler.toggleSyntaxTree(true);
        if (!decompiler.openProgram(currentProgram)) {
            throw new IllegalStateException(decompiler.getLastMessage());
        }

        try (BufferedWriter writer = Files.newBufferedWriter(
                output,
                StandardCharsets.UTF_8,
                StandardOpenOption.CREATE_NEW)) {
            writer.write("PRIVATE RETAIL PSEUDOCODE - DO NOT COMMIT\n");
            writer.write("image=" + currentProgram.getName() + "\n");
            writer.write("sha256=" + currentProgram.getExecutableSHA256() + "\n");
            writer.write("ghidra=" + Application.getApplicationVersion() + "\n");
            writer.write("policy=existing saved functions; explicit addresses; read-only; no analysis\n");

            for (int index = 2; index < args.length; index++) {
                monitor.checkCancelled();
                Function function = currentProgram.getFunctionManager().getFunctionAt(toAddr(args[index]));
                writer.write("\n===== " + args[index] + " =====\n");
                if (function == null) {
                    writer.write("NO_SAVED_FUNCTION\n");
                    continue;
                }
                DecompileResults result = decompiler.decompileFunction(function, 60, monitor);
                writer.write("entry=" + function.getEntryPoint() + "\n");
                writer.write("body_bytes=" + function.getBody().getNumAddresses() + "\n");
                writer.write("completed=" + result.decompileCompleted() + "\n");
                writer.write("error=" + result.getErrorMessage() + "\n");
                if (result.decompileCompleted() && result.getDecompiledFunction() != null) {
                    writer.write(result.getDecompiledFunction().getC());
                    writer.write("\n");
                }
            }
        } finally {
            decompiler.dispose();
        }
    }
}
