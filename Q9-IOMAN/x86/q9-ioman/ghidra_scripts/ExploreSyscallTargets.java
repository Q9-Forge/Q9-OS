// ExploreSyscallTargets.java
// For each known IOMan syscall target offset (from D_SysDis/D_UsrDis,
// converted to module-relative offsets using the live-measured IOMan base
// 0xE03C), create a function there if one doesn't already exist and dump
// its disassembly. Purpose: find out whether IOMan implements these
// syscalls itself or just dispatches onward to a File Manager/Driver.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.address.AddressSetView;

public class ExploreSyscallTargets extends GhidraScript {
    // offset -> label, from the merged SYSCALL_MODULE_MAP.md cross-reference
    static final Object[][] TARGETS = {
        {0x0446, "F$AllBit(sys)"}, {0x044a, "F$AllBit(usr)"},
        {0x04aa, "F$DelBit(usr)"}, {0x04ae, "F$DelBit(sys)"},
        {0x0500, "F$SchBit(usr)"}, {0x0506, "F$SchBit(sys)"},
        {0x069a, "F$IODel(sys)"}, {0x06d6, "F$Load"},
        {0x0a42, "F$PErr"}, {0x0b3a, "I$Attach"},
        {0x0dbe, "I$ChgDir"}, {0x0e14, "I$Close(usr)"},
        {0x0e24, "I$Close(sys)"}, {0x0e56, "I$Delete"},
        {0x0e5e, "I$Detach"}, {0x0f56, "I$Dup(usr)"},
        {0x0f84, "I$Dup(sys)"}, {0x0f90, "I$GetStt(usr)"},
        {0x0f98, "I$GetStt(sys)"}, {0x1026, "I$SGetSt"},
        {0x10d8, "F$IOQu(sys)"}, {0x11b6, "I$MakDir"},
        {0x1228, "I$Create(usr)"}, {0x124a, "I$Create(sys)"},
        {0x1268, "I$Read(usr)"}, {0x12a6, "I$Read(sys)"},
        {0x12c0, "I$Seek(usr)"}, {0x12c6, "I$Seek(sys)"},
        {0x12d0, "I$SetStt(usr)"}, {0x12d6, "I$SetStt(sys)"},
        {0x1316, "I$Write(usr)"}, {0x1358, "I$Write(sys)"},
    };

    @Override
    public void run() throws Exception {
        Address base = currentProgram.getMinAddress();
        for (Object[] t : TARGETS) {
            int off = (Integer) t[0];
            String label = (String) t[1];
            Address a = base.add(off);
            println("--- " + label + " @ offset 0x" + Integer.toHexString(off) + " (" + a + ") ---");
            Function f = getFunctionContaining(a);
            if (f == null) {
                try {
                    disassemble(a);
                    f = createFunction(a, "ioman_" + label.replaceAll("[^A-Za-z0-9]", "_") + "_" + Integer.toHexString(off));
                } catch (Exception e) {
                    println("  (could not create function: " + e.getMessage() + ")");
                }
            } else {
                println("  already inside function " + f.getName() + " @ " + f.getEntryPoint());
            }
            if (f != null) {
                AddressSetView body = f.getBody();
                InstructionIterator it = currentProgram.getListing().getInstructions(body, true);
                int count = 0;
                while (it.hasNext() && count < 40) {
                    Instruction ins = it.next();
                    println(String.format("  %s: %s", ins.getAddress(), ins.toString()));
                    count++;
                }
            } else {
                // dump raw bytes/instructions for a fixed window even if function creation failed
                Address cur = a;
                for (int i = 0; i < 20; i++) {
                    Instruction ins = getInstructionAt(cur);
                    if (ins == null) { ins = disassemble(cur) ? getInstructionAt(cur) : null; }
                    if (ins == null) break;
                    println(String.format("  %s: %s", ins.getAddress(), ins.toString()));
                    cur = ins.getAddress().add(ins.getLength());
                }
            }
            println("");
        }
    }
}
