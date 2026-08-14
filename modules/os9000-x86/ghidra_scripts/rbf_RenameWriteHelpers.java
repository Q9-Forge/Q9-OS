import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;

public class rbf_RenameWriteHelpers extends GhidraScript {
    void rn(long addr, String name) throws Exception {
        Address a = currentProgram.getMinAddress().getAddressSpace().getAddress(addr);
        Function f = getFunctionContaining(a);
        if (f == null) { println("no function at " + a); return; }
        f.setName(name, SourceType.USER_DEFINED);
        println("renamed " + f.getEntryPoint() + " -> " + name);
    }
    @Override
    public void run() throws Exception {
        rn(0x2524fcL, "Q9X_rbf_driver_dispatch");
        rn(0x2503dcL, "Q9X_rbf_write_error_retry");
    }
}
