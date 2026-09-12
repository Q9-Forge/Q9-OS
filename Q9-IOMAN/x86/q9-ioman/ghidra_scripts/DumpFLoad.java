// DumpFLoad.java -- disassembliert F$Load im IOMan-Modul. Physische
// Zieladresse laut docs/REVERSE_ENGINEERING.md Syscall-Tabelle: 0xE712
// (D_SysDis und D_UsrDis identisch). IOMan-Modulbasis laut
// Q9-Flux/docs/OS9_SYSCALL_OWNERSHIP.md: 0xE03C -> modulinterner Offset
// 0xE712-0xE03C = 0x6D6.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class DumpFLoad extends GhidraScript {
    @Override
    public void run() throws Exception {
        Address base = currentProgram.getMinAddress();
        Address entry = base.add(0x6d6);
        disassemble(entry);
        try { createFunction(entry, "Q9_ioman_f_load"); } catch (Exception e) { println("createFunction: " + e.getMessage()); }
        analyzeAll(currentProgram);

        println("=== FUNCTIONS (rund um F$Load) ===");
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            long off = f.getEntryPoint().subtract(base);
            if (off >= 0x600 && off <= 0xb00) {
                println(f.getName() + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses());
            }
        }

        println("=== DISASSEMBLY 0x6D6-0xB00 ===");
        Address rangeStart = base.add(0x6d6);
        Address rangeEnd = base.add(0xb00);
        InstructionIterator it = currentProgram.getListing().getInstructions(rangeStart, true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            if (ins.getAddress().compareTo(rangeEnd) > 0) break;
            println(ins.getAddress() + ": " + ins.toString());
        }
    }
}
