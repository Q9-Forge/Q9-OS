// ioman_ParseHeaderSetEntryAnalyze.java
// Parses the OS-9000/x86 module header of vendor-live/ioman (same layout
// as the kernel, see kernel_ParseHeaderAndAnalyze.java / FINDINGS.md
// Fund 1), sets the entry point from m_exec (header offset 0x24), and
// runs full auto-analysis. Unlike the kernel, ioman's entry offset points
// DIRECTLY at real code (no JMP trampoline over an ID string) -- notable
// difference documented in docs/kernel-walkthrough/02-io-manager-syscall-dispatch/.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.listing.CodeUnit;

public class ioman_ParseHeaderSetEntryAnalyze extends GhidraScript {
    @Override
    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        Address base = currentProgram.getMinAddress();

        int mId = mem.getShort(base.add(0x00)) & 0xFFFF;
        int modSize = mem.getInt(base.add(0x04));
        int nameOff = mem.getInt(base.add(0x0C));
        int lang = mem.getByte(base.add(0x12)) & 0xFF;
        int type = mem.getByte(base.add(0x13)) & 0xFF;
        int execOff = mem.getInt(base.add(0x24));

        println(String.format("M$ID=0x%04X size=%d nameOff=0x%X lang=0x%02X type=0x%02X exec=0x%X",
            mId, modSize, nameOff, lang, type, execOff));

        StringBuilder name = new StringBuilder();
        Address nameAddr = base.add(nameOff);
        for (int i = 0; i < 32; i++) {
            byte b = mem.getByte(nameAddr.add(i));
            if (b == 0) break;
            name.append((char) (b & 0x7F));
        }
        println("name=" + name);

        Address entry = base.add(execOff);
        disassemble(entry);
        createFunction(entry, "Q9X_ioman_entry_raw");
        analyzeAll(currentProgram);

        long totalLen = mem.getMaxAddress().getOffset() - base.getOffset() + 1;
        long codeLen = 0, dataLen = 0;
        for (CodeUnit c : currentProgram.getListing().getCodeUnits(true)) {
            if (c instanceof ghidra.program.model.listing.Instruction) codeLen += c.getLength();
            else if (c instanceof ghidra.program.model.listing.Data) {
                ghidra.program.model.listing.Data d = (ghidra.program.model.listing.Data) c;
                if (d.isDefined()) dataLen += c.getLength();
            }
        }
        int funcCount = currentProgram.getFunctionManager().getFunctionCount();
        println(String.format("Coverage: code=%.1f%% data=%.1f%% undef=%.1f%% funcs=%d",
            100.0 * codeLen / totalLen, 100.0 * dataLen / totalLen,
            100.0 * (totalLen - codeLen - dataLen) / totalLen, funcCount));
    }
}
