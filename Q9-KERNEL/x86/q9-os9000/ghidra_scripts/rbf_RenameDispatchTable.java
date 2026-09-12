import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
import ghidra.util.exception.DuplicateNameException;

public class rbf_RenameDispatchTable extends GhidraScript {
    void rn(long addr, String name) throws Exception {
        Address a = currentProgram.getMinAddress().getAddressSpace().getAddress(addr);
        Function f = getFunctionAt(a);
        if (f == null) {
            boolean ok = createFunction(a, name) != null;
            println("createFunction " + name + " ok=" + ok);
            f = getFunctionAt(a);
        }
        if (f == null) { println("STILL NULL for " + name + " @ " + a); return; }
        try { f.setName(name, SourceType.USER_DEFINED); println("renamed " + f.getEntryPoint() + " -> " + name); }
        catch (DuplicateNameException e) { println("dup skip: " + name); }
    }

    @Override
    public void run() throws Exception {
        rn(0x24d904, "Q9X_rbf_entry_trampolin");
        rn(0x24d910, "Q9X_rbf_entry_stub");   // trivial XOR EAX,EAX; RET
        rn(0x24d918, "Q9X_rbf_checksum");     // XOR-loop checksum, same pattern as kernel FUN_00227a2e
        rn(0x24d942, "Q9X_rbf_init_or_setup"); // large 1171-byte function right after entry

        // The 16-slot I$ callcode dispatch table found at file offset 0x9124
        // (absolute 0x256984), positioned right after a 16-byte header at
        // m_idata (0x9118) whose third field (0x9120) = 0x10 = 16 = the slot
        // count. Order matches the 68K RBF table's proven order for the
        // first 13 slots (I$Create..I$Close); slots 13-15 are OS-9000-only
        // extensions, not identified further this round.
        String[] names = {
            "Q9X_rbf_i_create","Q9X_rbf_i_open","Q9X_rbf_i_makdir","Q9X_rbf_i_chgdir",
            "Q9X_rbf_i_delete","Q9X_rbf_i_seek","Q9X_rbf_i_read","Q9X_rbf_i_write",
            "Q9X_rbf_i_readln","Q9X_rbf_i_writln","Q9X_rbf_i_getstt","Q9X_rbf_i_setstt",
            "Q9X_rbf_i_close","Q9X_rbf_i_slot13_unk","Q9X_rbf_i_slot14_unk","Q9X_rbf_i_slot15_unk"
        };
        long[] addrs = {
            0x2536e8L,0x2545c8L,0x254628L,0x2539b6L,0x25473aL,0x2537f8L,0x254f4aL,0x251ea6L,
            0x254d3cL,0x25437aL,0x25163eL,0x251262L,0x251d54L,0x25253cL,0x251a42L,0x251a00L
        };
        for (int i = 0; i < names.length; i++) rn(addrs[i], names[i]);
    }
}
