// SetEntryAndAnalyze.java
// Sets the module entry point from M$Exec (type-specific extension offset
// 0x30, same convention confirmed for the kernel module), creates a
// function there, then runs full auto-analysis and reports coverage.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.listing.CodeUnit;

public class SetEntryAndAnalyze extends GhidraScript {
    @Override
    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        Address base = currentProgram.getMinAddress();

        int mExec = mem.getInt(base.add(0x30));
        println(String.format("M$Exec (entry offset) = 0x%08X", mExec));
        Address entry = base.add(mExec);

        disassemble(entry);
        createFunction(entry, "ioman_entry_" + Integer.toHexString(mExec));

        analyzeAll(currentProgram);

        long totalLen = mem.getMaxAddress().getOffset() - base.getOffset() + 1;
        long codeLen = 0, dataLen = 0;
        for (CodeUnit c : currentProgram.getListing().getCodeUnits(true)) {
            if (c instanceof ghidra.program.model.listing.Instruction) codeLen += c.getLength();
            else if (c instanceof ghidra.program.model.listing.Data) {
                ghidra.program.model.listing.Data d = (ghidra.program.model.listing.Data) c;
                if (d.isDefined()) dataLen += c.getLength();
            }
        }
        int funcCount = currentProgram.getFunctionManager().getFunctionCount();
        println("=== After entry-point analysis ===");
        println(String.format("Total module length: %d bytes", totalLen));
        println(String.format("Disassembled as code: %d bytes (%.1f%%)", codeLen, 100.0*codeLen/totalLen));
        println(String.format("Defined data: %d bytes (%.1f%%)", dataLen, 100.0*dataLen/totalLen));
        println(String.format("Undefined: %d bytes (%.1f%%)", totalLen-codeLen-dataLen, 100.0*(totalLen-codeLen-dataLen)/totalLen));
        println(String.format("Functions: %d", funcCount));

        println("=== Function list ===");
        for (ghidra.program.model.listing.Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            println(String.format("  %s @ %s  size=%d", f.getName(), f.getEntryPoint(), f.getBody().getNumAddresses()));
        }
    }
}
