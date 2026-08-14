// Reads I$Read (0xB2 table slot, target 0x5D0) and I$Write (0xB4 slot,
// target 0x778) fully -- offsets already confirmed byte-exactly in
// docs/FINDINGS.md via raw Python analysis. This script disassembles
// both handler bodies via Ghidra so we get real control-flow-aware
// function boundaries instead of guessing where they end.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;

public class DumpReadWriteHandlers extends GhidraScript {
    @Override
    public void run() throws Exception {
        Address base = currentProgram.getMinAddress();

        // Set the module's own entry point first so auto-analysis has a
        // starting point for general control flow before we target the
        // two handlers specifically.
        Address entry = base.add(0xA6);
        disassemble(entry);
        createFunction(entry, "Q9_rbf_entry");

        Address readAddr = base.add(0x5D0);
        disassemble(readAddr);
        Function readFn = createFunction(readAddr, "Q9_rbf_i_read");
        println("=== Q9_rbf_i_read created @ " + readAddr + " ===");

        Address writeAddr = base.add(0x778);
        disassemble(writeAddr);
        Function writeFn = createFunction(writeAddr, "Q9_rbf_i_write");
        println("=== Q9_rbf_i_write created @ " + writeAddr + " ===");

        analyzeAll(currentProgram);

        for (Function f : new Function[]{
                currentProgram.getFunctionManager().getFunctionAt(readAddr),
                currentProgram.getFunctionManager().getFunctionAt(writeAddr)}) {
            if (f == null) { println("FUNCTION MISSING"); continue; }
            println("=== " + f.getName() + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses() + " ===");
            InstructionIterator it = currentProgram.getListing().getInstructions(f.getBody(), true);
            while (it.hasNext()) {
                Instruction ins = it.next();
                println(ins.getAddress() + ": " + ins.toString());
            }
        }
    }
}
