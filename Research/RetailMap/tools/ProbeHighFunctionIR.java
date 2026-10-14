// Independently written research metadata probe. No C text or retail bytes exported.
// Run canonical projects with -readOnly -noanalysis. Explicit addresses only.
// @category CFW.RetailMap
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.decompiler.signature.SignatureResult;
import ghidra.features.bsim.query.FunctionDatabase;
import ghidra.framework.Application;
import ghidra.program.model.listing.*;
import ghidra.program.model.pcode.*;
import ghidra.feature.fid.service.FidService;
import ghidra.feature.fid.hash.FidHashQuad;
import com.google.gson.Gson;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.io.BufferedWriter;
import java.util.*;

public class ProbeHighFunctionIR extends GhidraScript {
    private final Gson gson = new Gson();
    private Map<String,Object> row(Object... values) {
        Map<String,Object> result = new LinkedHashMap<>();
        for(int i=0;i<values.length;i+=2) result.put((String)values[i],values[i+1]);
        return result;
    }
    private void emit(BufferedWriter out, Object value) throws Exception {
        out.write(gson.toJson(value)); out.newLine();
    }
    private void count(Map<String,Integer> counts, String key) {
        counts.put(key, counts.getOrDefault(key,0)+1);
    }
    private String shape(Varnode value) {
        if(value==null) return "void";
        return (value.isConstant()?"constant":value.isRegister()?"register":value.isUnique()?"unique":value.isAddress()?"address":"other")+":"+value.getSize();
    }
    private void op(Map<String,Integer> counts, PcodeOp operation) {
        StringBuilder key = new StringBuilder(operation.getMnemonic()).append("|").append(shape(operation.getOutput()));
        for(int i=0;i<operation.getNumInputs();i++) key.append("|").append(shape(operation.getInput(i)));
        count(counts,key.toString());
    }
    @Override public void run() throws Exception {
        String[] args=getScriptArgs();
        if(args.length<3) throw new IllegalArgumentException("output expected-sha256 VA...");
        if(!args[1].equalsIgnoreCase(currentProgram.getExecutableSHA256())) throw new IllegalStateException("Imported image hash mismatch");
        Path path=Paths.get(args[0]).toAbsolutePath().normalize();
        if(!path.toString().replace('\\','/').contains("/Evidence/RetailMap/local/")) throw new IllegalArgumentException("Private evidence output required");
        DecompInterface decompiler=new DecompInterface();
        decompiler.toggleCCode(false); decompiler.toggleSyntaxTree(true);
        decompiler.setSimplificationStyle("decompile");
        if(!decompiler.openProgram(currentProgram)) throw new IllegalStateException(decompiler.getLastMessage());
        int signatureSettings=FunctionDatabase.loadConfigurationTemplate("medium_nosize").info.settings;
        if(!decompiler.setSignatureSettings(signatureSettings)) throw new IllegalStateException("BSim settings rejected");
        try(BufferedWriter out=Files.newBufferedWriter(path,StandardCharsets.UTF_8,StandardOpenOption.CREATE_NEW)) {
            emit(out,row("record","header","schema","cfw.highfunction-ir-probe.v1","image_name",currentProgram.getName(),"image_sha256",currentProgram.getExecutableSHA256(),"project",state.getProject().getProjectLocator().toString(),"domain_path",currentProgram.getDomainFile().getPathname(),"language",currentProgram.getLanguageID().toString(),"compiler",currentProgram.getCompilerSpec().getCompilerSpecID().toString(),"ghidra",Application.getApplicationVersion(),"bsim_settings",signatureSettings,"policy","explicit-address development feasibility; transient function recovery; no pseudocode; no saved changes"));
            for(int i=2;i<args.length;i++) {
                monitor.checkCancelled();
                Function function=currentProgram.getFunctionManager().getFunctionAt(toAddr(args[i]));
                boolean saved=function!=null;
                if(currentProgram.getListing().getInstructionAt(toAddr(args[i]))==null) disassemble(toAddr(args[i]));
                if(function==null) function=createFunction(toAddr(args[i]),null);
                if(function==null) {emit(out,row("record","function","va",args[i],"status","NO_FUNCTION")); continue;}
                Map<String,Integer> raw=new TreeMap<>(), high=new TreeMap<>();
                for(Instruction instruction:currentProgram.getListing().getInstructions(function.getBody(),true)) for(PcodeOp operation:instruction.getPcode()) op(raw,operation);
                long start=System.nanoTime();
                DecompileResults result=decompiler.decompileFunction(function,45,monitor);
                HighFunction hf=result.getHighFunction();
                if(hf!=null) {Iterator<PcodeOpAST> operations=hf.getPcodeOps(); while(operations.hasNext()) op(high,operations.next());}
                SignatureResult signatures=decompiler.generateSignatures(function,false,45,monitor);
                FidHashQuad fid=null; String fidError="";
                try {fid=new FidService().hashFunction(function);} catch(Exception e) {fidError=e.getClass().getSimpleName()+":"+e.getMessage();}
                emit(out,row("record","function","va",function.getEntryPoint().toString(),"saved_function",saved,"body_bytes",function.getBody().getNumAddresses(),"body_min",function.getBody().getMinAddress().toString(),"body_max",function.getBody().getMaxAddress().toString(),"decompile_completed",result.decompileCompleted(),"error",result.getErrorMessage(),"elapsed_ms",(System.nanoTime()-start)/1000000,"raw_pcode_shapes",raw,"high_pcode_shapes",high,"high_blocks",hf==null?0:hf.getBasicBlocks().size(),"bsim_features",signatures==null?new int[0]:signatures.features,"bsim_bad_data",signatures==null?true:signatures.hasbaddata,"bsim_unimplemented",signatures==null?true:signatures.hasunimplemented,"fid",fid==null?"":fid.toString(),"fid_error",fidError));
            }
            emit(out,row("record","footer","complete",true,"requested",args.length-2));
        } finally {decompiler.dispose();}
    }
}
