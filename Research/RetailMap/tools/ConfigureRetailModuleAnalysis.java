// Deterministic headless-analysis guard for canonical retail PE modules.
// Disables the optional Windows resource-reference analyzer, whose bundled
// script may be unavailable under the selected JDK. No program bytes change.
// @category CFW.RetailMap

import ghidra.app.script.GhidraScript;
import java.util.Map;

public class ConfigureRetailModuleAnalysis extends GhidraScript {
    @Override
    public void run() throws Exception {
        Map<String, String> options = getCurrentAnalysisOptionsAndValues(currentProgram);
        int disabled = 0;
        for (String name : options.keySet()) {
            String normalized = name.toLowerCase();
            if (normalized.contains("resource") && normalized.contains("reference")) {
                setAnalysisOption(currentProgram, name, "false");
                println("Disabled analysis option: " + name);
                disabled++;
            }
        }
        println("Disabled resource-reference options: " + disabled);
        String[] args = getScriptArgs();
        if (args.length == 1 && args[0].equals("--analyze")) {
            analyzeAll(currentProgram);
        }
    }
}
