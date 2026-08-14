import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.util.exception.DuplicateNameException;
import ghidra.program.model.symbol.SourceType;

public class kernel_RenameMemAlloc extends GhidraScript {
    void rn(long addr, String name) throws Exception {
        Address a = currentProgram.getMinAddress().getAddressSpace().getAddress(addr);
        Function f = getFunctionContaining(a);
        if (f == null) { println("no function at " + a + " for " + name); return; }
        try { f.setName(name, SourceType.USER_DEFINED); println("renamed " + f.getEntryPoint() + " -> " + name); }
        catch (DuplicateNameException e) { println("dup skip: " + name); }
    }

    @Override
    public void run() throws Exception {
        rn(0x222b90, "Q9X_arena_lookup_or_create");
        rn(0x222680, "Q9X_arena_reserve_space");

        DecompInterface decomp = new DecompInterface();
        decomp.setOptions(new DecompileOptions());
        decomp.openProgram(currentProgram);
        decomp.setSimplificationStyle("decompile");

        long[] addrs = {0x222b90, 0x222680};
        for (long addr : addrs) {
            Address a = currentProgram.getMinAddress().getAddressSpace().getAddress(addr);
            Function f = getFunctionContaining(a);
            if (f == null) { println("MISSING FUNC @ " + a); continue; }
            println("=== DECOMPILE " + f.getName() + " @ " + f.getEntryPoint() + " ===");
            DecompileResults res = decomp.decompileFunction(f, 180, monitor);
            if (res != null && res.decompileCompleted()) {
                println(res.getDecompiledFunction().getC());
            } else {
                println("decompile failed: " + (res != null ? res.getErrorMessage() : "null"));
            }
        }
        decomp.dispose();
    }
}
