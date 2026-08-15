// DumpReadWriteHandlers.java -- disassembles the cfide driver's Read/Write
// slot handlers (hypothesized offsets 0x72/0x1DE from FINDINGS.md) and
// dumps full disassembly + basic control-flow structure for both.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class DumpReadWriteHandlers extends GhidraScript {
    @Override
    public void run() throws Exception {
        Address base = currentProgram.getMinAddress();
        int[] offsets = {0x4A, 0x72, 0x1DE, 0x35E, 0x4EE, 0x4F8};
        String[] names = {"Q9_cfide_init", "Q9_cfide_read", "Q9_cfide_write", "Q9_cfide_getstat", "Q9_cfide_setstat", "Q9_cfide_term"};
        for (int i = 0; i < offsets.length; i++) {
            Address a = base.add(offsets[i]);
            disassemble(a);
            try { createFunction(a, names[i]); } catch (Exception e) { println("createFunction failed @ " + a + ": " + e.getMessage()); }
        }
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
