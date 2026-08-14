// Quick peek at the first ~20 instructions of FUN_002524fc (the helper
// Q9X_rbf_i_write calls into) -- just enough to see whether it looks
// like a driver-dispatch trampoline or a buffer-copy routine, not a
// full read.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.listing.Function;

public class rbf_PeekWriteHelper extends GhidraScript {
    @Override
    public void run() throws Exception {
        for (long addr : new long[]{0x2524fcL, 0x2503dcL}) {
            Address a = currentProgram.getMinAddress().getAddressSpace().getAddress(addr);
            Function f = getFunctionContaining(a);
            println("=== " + Long.toHexString(addr) + " -> function " + (f != null ? f.getName() + " size=" + f.getBody().getNumAddresses() : "NONE") + " ===");
            InstructionIterator it = currentProgram.getListing().getInstructions(a, true);
            int n = 0;
            while (it.hasNext() && n < 25) {
                Instruction ins = it.next();
                println(ins.getAddress() + ": " + ins.toString());
                n++;
            }
        }
    }
}
