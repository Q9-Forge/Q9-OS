import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.util.exception.DuplicateNameException;
import ghidra.program.model.symbol.SourceType;

public class kernel_RenameAndDecompileBootChain extends GhidraScript {
    void rn(long addr, String name) throws Exception {
        Address a = currentProgram.getMinAddress().getAddressSpace().getAddress(addr);
        Function f = getFunctionContaining(a);
        if (f == null) {
            println("no function at all containing " + a + " for " + name + " -- trying createFunction");
            boolean ok = createFunction(a, name) != null;
            println("createFunction ok=" + ok);
            f = getFunctionAt(a);
        }
        if (f == null) { println("STILL NULL for " + name + " @ " + a); return; }
        try { f.setName(name, SourceType.USER_DEFINED); println("renamed " + f.getEntryPoint() + " -> " + name); }
        catch (DuplicateNameException e) { println("dup name skip: " + name); }
    }

    @Override
    public void run() throws Exception {
        rn(0x21e4a4, "Q9X_entry_trampolin");
        rn(0x21e4c0, "Q9X_kernel_init");
        rn(0x21f17e, "Q9X_query_memsize");
        rn(0x22246c, "Q9X_module_check_reloc");
        rn(0x21eb26, "Q9X_kernel_globals_init");
        rn(0x221540, "Q9X_dispatch_table_build");
        rn(0x21f4bc, "Q9X_device_module_init_loop");
        rn(0x22e9b0, "Q9X_trivial_ret_stub");

        DecompInterface decomp = new DecompInterface();
        decomp.setOptions(new DecompileOptions());
        decomp.openProgram(currentProgram);
        decomp.setSimplificationStyle("decompile");

        long[] addrs = {0x21e4c0, 0x22246c, 0x21eb26, 0x221540};
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

        println("=== FUNCTION LIST (renamed) ===");
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            if (f.getName().startsWith("Q9X_")) {
                println(f.getName() + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses());
            }
        }
    }
}
