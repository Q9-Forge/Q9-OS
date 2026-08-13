// kernel_SetEntryAndAnalyze.java
// Sets the module entry point from the header field at offset 0x24
// (empirically identified as the x86 analogue of 68K's M$Exec, see
// ../docs/KERNEL_INIT.md) -- it points to a short `jmp` trampoline, not
// directly to the init function body. Disassembles the trampoline, follows
// the jump, creates a function at the real target, then runs full
// auto-analysis and reports coverage.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Instruction;

public class kernel_SetEntryAndAnalyze extends GhidraScript {
    @Override
    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        Address base = currentProgram.getMinAddress();

        int execOff = mem.getInt(base.add(0x24));
        Address trampoline = base.add(execOff);
        println(String.format("Entry field @0x24 = 0x%X -> trampoline at %s", execOff, trampoline));

        Instruction jmpInsn = disassemble(trampoline) ? getInstructionAt(trampoline) : null;
        if (jmpInsn == null) {
            println("FEHLER: Trampolin-Adresse ließ sich nicht disassemblieren.");
            return;
        }
        println("Trampolin-Instruktion: " + jmpInsn.toString());

        Address realEntry = null;
        if (jmpInsn.getFlowType().isJump() && jmpInsn.getFlows().length > 0) {
            realEntry = jmpInsn.getFlows()[0];
        }
        if (realEntry == null) {
            println("FEHLER: Trampolin ist kein einfacher direkter Sprung, kein automatisches Ziel gefunden.");
            return;
        }
        println("Reales Kernel-Init-Entry: " + realEntry);

        disassemble(realEntry);
        createFunction(realEntry, "kernel_init_" + realEntry.toString().replace(":", "_"));
        createFunction(trampoline, "kernel_entry_trampoline");

        analyzeAll(currentProgram);

        long totalLen = mem.getMaxAddress().getOffset() - base.getOffset() + 1;
        long codeLen = 0, dataLen = 0;
        for (CodeUnit c : currentProgram.getListing().getCodeUnits(true)) {
            if (c instanceof Instruction) codeLen += c.getLength();
            else if (c instanceof ghidra.program.model.listing.Data) {
                ghidra.program.model.listing.Data d = (ghidra.program.model.listing.Data) c;
                if (d.isDefined()) dataLen += c.getLength();
            }
        }
        int funcCount = currentProgram.getFunctionManager().getFunctionCount();
        println("=== Nach Entry-Point-Analyse + analyzeAll ===");
        println(String.format("Total module length: %d bytes", totalLen));
        println(String.format("Disassembled as code: %d bytes (%.1f%%)", codeLen, 100.0*codeLen/totalLen));
        println(String.format("Defined data: %d bytes (%.1f%%)", dataLen, 100.0*dataLen/totalLen));
        println(String.format("Undefined: %d bytes (%.1f%%)", totalLen-codeLen-dataLen, 100.0*(totalLen-codeLen-dataLen)/totalLen));
        println(String.format("Functions: %d", funcCount));

        println("=== Function list (Adresse, Größe) ===");
        for (ghidra.program.model.listing.Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            println(String.format("  %s @ %s  size=%d", f.getName(), f.getEntryPoint(), f.getBody().getNumAddresses()));
        }
    }
}
