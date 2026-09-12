import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class scllio_AnalyzeAndDump extends GhidraScript {
    @Override
    public void run() throws Exception {
        Address base = currentProgram.getMinAddress();
        int execOff = 0x78; // driver-type module: exec offset field sits at header 0x2C, not 0x24 (empirically found)
        Address entry = base.add(execOff);
        disassemble(entry);
        try { createFunction(entry, "Q9X_scllio_entry"); } catch (Exception e) { println("createFunction: " + e.getMessage()); }
        analyzeAll(currentProgram);

        println("=== FUNCTIONS ===");
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            println(f.getName() + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses());
        }

        println("=== FULL DISASSEMBLY ===");
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            println(ins.getAddress() + ": " + ins.toString());
        }
    }
}
