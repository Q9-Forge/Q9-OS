// ioman_RenameAndDecompileDispatch.java
// Renames the IOMan boot-analysis-round functions identified in
// docs/kernel-walkthrough/02-io-manager-syscall-dispatch/ with the Q9X_
// prefix (same convention as the kernel scripts), then decompiles the
// central ones (entry, attach/open, kernel-service-call primitives) for
// citation. Central finding this round: Q9X_ioman_attach links Descriptor/
// Driver/File-Manager with the SAME type filters (0xF00/0xE00/0xD00) as
// the 68K's I$Attach -- see FINDINGS.md Fund 8 for the full writeup.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.util.exception.DuplicateNameException;
import ghidra.program.model.symbol.SourceType;

public class ioman_RenameAndDecompileDispatch extends GhidraScript {
    void rn(long addr, String name) throws Exception {
        Address a = currentProgram.getMinAddress().getAddressSpace().getAddress(addr);
        Function f = getFunctionContaining(a);
        if (f == null) { println("no func for " + name + " @ " + a); return; }
        try { f.setName(name, SourceType.USER_DEFINED); println("renamed " + f.getEntryPoint() + " -> " + name); }
        catch (DuplicateNameException e) { println("dup skip " + name); }
    }

    @Override
    public void run() throws Exception {
        rn(0x233556, "Q9X_ioman_entry");
        rn(0x2334dc, "Q9X_type_filter_check");
        rn(0x233532, "Q9X_kernel_service_call");
        rn(0x233866, "Q9X_path_component_scan");
        rn(0x2338aa, "Q9X_path_sep_check");
        rn(0x233902, "Q9X_device_table_walk");
        rn(0x23413a, "Q9X_ioman_attach");
        rn(0x235a9a, "Q9X_ioman_open");
        rn(0x236ed2, "Q9X_link_module");
        rn(0x236e5e, "Q9X_alloc_memory");
        rn(0x235186, "Q9X_devtable_lock_insert");
        rn(0x236fba, "Q9X_free_memory");
        rn(0x233f30, "Q9X_ioman_detach_rollback");
        rn(0x234de2, "Q9X_report_error");
        rn(0x237175, "Q9X_get_current_proc");

        DecompInterface decomp = new DecompInterface();
        decomp.setOptions(new DecompileOptions());
        decomp.openProgram(currentProgram);

        long[] addrs = {0x233532, 0x23413a, 0x235a9a, 0x236ed2, 0x236e5e};
        for (long addr : addrs) {
            Address a = currentProgram.getMinAddress().getAddressSpace().getAddress(addr);
            Function f = getFunctionContaining(a);
            if (f == null) { println("MISSING FUNC @ " + a); continue; }
            println("=== DECOMPILE " + f.getName() + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses() + " ===");
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
