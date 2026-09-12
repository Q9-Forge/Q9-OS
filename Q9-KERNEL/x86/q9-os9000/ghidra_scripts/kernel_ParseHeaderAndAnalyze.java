// kernel_ParseHeaderAndAnalyze.java
// Dumps the OS-9000/x86 module header fields for the live-extracted
// `kernel` module (vendor-live/kernel). x86 header differs from the 68K
// layout: 4-byte little-endian name-offset at 0x0C (vs 68K's 2-byte),
// language byte at 0x12, type byte at 0x13 (same absolute offsets/codes as
// 68K, confirmed in ../docs/FINDINGS.md Fund 1). Everything from 0x14
// onward was previously unmapped ("PLATZHALTER") -- this script's raw hex
// dump plus manual byte analysis (see ../docs/KERNEL_INIT.md) identified
// offset 0x24 as the execution-offset field (x86 analogue of 68K's
// M$Exec), pointing to a short `jmp` trampoline at file offset 0xA4.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.listing.CodeUnit;

public class kernel_ParseHeaderAndAnalyze extends GhidraScript {
    @Override
    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        Address base = currentProgram.getMinAddress();

        int mId       = mem.getShort(base.add(0x00)) & 0xFFFF; // stored as fc4a (LE) = sync 0x4AFC
        int sysRev    = mem.getShort(base.add(0x02)) & 0xFFFF;
        int modSize   = mem.getInt(base.add(0x04));
        int modOwner  = mem.getInt(base.add(0x08));
        int nameOff   = mem.getInt(base.add(0x0C));
        int accsErr   = mem.getShort(base.add(0x10)) & 0xFFFF;
        int langByte  = mem.getByte(base.add(0x12)) & 0xFF;
        int typeByte  = mem.getByte(base.add(0x13)) & 0xFF;
        int excptOff  = mem.getInt(base.add(0x20));
        int execOff   = mem.getInt(base.add(0x24));

        println("=== OS-9000/x86 kernel module header ===");
        println(String.format("M$ID        = 0x%04X (sync)", mId));
        println(String.format("M$SysRev    = 0x%04X", sysRev));
        println(String.format("M$Size      = 0x%08X (%d)", modSize, modSize));
        println(String.format("M$Owner     = 0x%08X", modOwner));
        println(String.format("Name-Offset = 0x%08X", nameOff));
        println(String.format("Accs/Err    = 0x%04X", accsErr));
        println(String.format("Lang byte@0x12 = 0x%02X", langByte));
        println(String.format("Type byte@0x13 = 0x%02X (0x0C=Systm expected)", typeByte));
        println(String.format("Field@0x20 (candidate M$Excpt) = 0x%08X", excptOff));
        println(String.format("Field@0x24 (candidate M$Exec)  = 0x%08X", execOff));

        StringBuilder name = new StringBuilder();
        Address nameAddr = base.add(nameOff);
        for (int i = 0; i < 64; i++) {
            byte b = mem.getByte(nameAddr.add(i));
            if (b == 0) break;
            name.append((char)(b & 0x7F));
        }
        println("Module name = \"" + name.toString() + "\"");

        println("=== Raw bytes 0x14-0x60 (previously unmapped header region) ===");
        StringBuilder hex = new StringBuilder();
        for (int i = 0; i < (0x60 - 0x14); i++) {
            hex.append(String.format("%02X ", mem.getByte(base.add(0x14 + i)) & 0xFF));
            if (i % 16 == 15) { println(hex.toString()); hex.setLength(0); }
        }
        if (hex.length() > 0) println(hex.toString());

        // 0xB0BD "structure signature" cross-check (see docs/REVERSE_ENGINEERING.md,
        // the 68K kernel's equivalent finding at module-header offset 0x40-0x43).
        // Here it shows up right after the M$Exec jmp trampoline, twice in a row.
        println("=== 0xB0BD magic-constant check (repeated LE words near entry trampoline) ===");
        for (int off = 0xA0; off < 0xC0; off += 2) {
            int w = mem.getShort(base.add(off)) & 0xFFFF;
            if (w == 0xB0BD) println(String.format("  0x%04X found at offset 0x%02X", w, off));
        }

        println("=== Auto-analysis coverage (before entry-point disassembly) ===");
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
        println(String.format("Total module length: %d bytes", totalLen));
        println(String.format("Disassembled as code: %d bytes (%.1f%%)", codeLen, 100.0*codeLen/totalLen));
        println(String.format("Defined data: %d bytes (%.1f%%)", dataLen, 100.0*dataLen/totalLen));
        println(String.format("Undefined: %d bytes (%.1f%%)", totalLen-codeLen-dataLen, 100.0*(totalLen-codeLen-dataLen)/totalLen));
        println(String.format("Functions found so far: %d", funcCount));
    }
}
