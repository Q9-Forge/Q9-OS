// Dumps Q9X_rbf_i_read (@0x254f4a) and Q9X_rbf_i_write (@0x251ea6) in
// full, plus a decompile of write since it's a real-sized function
// (read was already found to be a 5-byte stub in Topic 03 -- confirm
// that here and see if anything calls out of it).
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;

public class rbf_DumpReadWriteHandlers extends GhidraScript {
    @Override
    public void run() throws Exception {
        long[] addrs = {0x254f4aL, 0x251ea6L};
        String[] names = {"Q9X_rbf_i_read", "Q9X_rbf_i_write"};

        for (int i = 0; i < addrs.length; i++) {
            Address a = currentProgram.getMinAddress().getAddressSpace().getAddress(addrs[i]);
            Function f = getFunctionContaining(a);
            if (f == null) { println("no function at " + a); continue; }
            println("=== " + names[i] + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses() + " ===");
            InstructionIterator it = currentProgram.getListing().getInstructions(f.getBody(), true);
            while (it.hasNext()) {
                Instruction ins = it.next();
                println(ins.getAddress() + ": " + ins.toString());
            }
        }

        DecompInterface decomp = new DecompInterface();
        decomp.setOptions(new DecompileOptions());
        decomp.openProgram(currentProgram);
        Address wAddr = currentProgram.getMinAddress().getAddressSpace().getAddress(0x251ea6L);
        Function wf = getFunctionContaining(wAddr);
        if (wf != null) {
            println("=== DECOMPILE Q9X_rbf_i_write ===");
            DecompileResults res = decomp.decompileFunction(wf, 120, monitor);
            if (res != null && res.decompileCompleted()) println(res.getDecompiledFunction().getC());
            else println("decompile failed");
        }
        decomp.dispose();
    }
}
