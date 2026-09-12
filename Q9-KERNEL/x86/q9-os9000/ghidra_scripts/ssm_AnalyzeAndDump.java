import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.Memory;

public class ssm_AnalyzeAndDump extends GhidraScript {
    @Override
    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        Address base = currentProgram.getMinAddress();
        int execOff = mem.getInt(base.add(0x24));
        println("m_exec = 0x" + Integer.toHexString(execOff));
        Address entry = base.add(execOff);
        disassemble(entry);
        createFunction(entry, "Q9X_ssm_entry");
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
