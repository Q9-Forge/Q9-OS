// DumpFLoadCore.java -- disassembliert die eigentliche Such-/Lade-Logik,
// die F$Load (0x6D6, s. DumpFLoad.java) bei 0x124A aufruft.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class DumpFLoadCore extends GhidraScript {
    @Override
    public void run() throws Exception {
        Address base = currentProgram.getMinAddress();
        Address entry = base.add(0x124a);
        disassemble(entry);
        try { createFunction(entry, "Q9_ioman_load_search_core"); } catch (Exception e) { println("createFunction: " + e.getMessage()); }
        analyzeAll(currentProgram);

        println("=== DISASSEMBLY 0x124A-0x1500 ===");
        Address rangeStart = base.add(0x124a);
        Address rangeEnd = base.add(0x1500);
        InstructionIterator it = currentProgram.getListing().getInstructions(rangeStart, true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            if (ins.getAddress().compareTo(rangeEnd) > 0) break;
            println(ins.getAddress() + ": " + ins.toString());
        }
    }
}
