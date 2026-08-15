import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class vectx86_AnalyzeAndDump extends GhidraScript {
    @Override
    public void run() throws Exception {
        Address base = currentProgram.getMinAddress();
        Address entry = base.add(0x60); // m_exec laut Header-Offset 0x24 = 0x60
        disassemble(entry);
        try { createFunction(entry, "Q9X_vectx86_entry"); } catch (Exception e) { println("createFunction: " + e.getMessage()); }
        analyzeAll(currentProgram);
        println("=== FUNCTIONS ===");
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            println(f.getName() + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses());
        }
        println("=== DISASSEMBLY ===");
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            println(ins.getAddress() + ": " + ins.toString());
        }
    }
}
