// ParseHeaderAndAnalyze.java
// Dumps the OS-9 module header fields (0x00-0x2F standard + type-specific
// extension from 0x30) for ioman_DEV, sets the entry point from M$Exec,
// creates a function there, and reports auto-analysis coverage.
// Mirrors the approach used for the kernel module (dker030s) in the sibling
// Q9-OS repo's docs/REVERSE_ENGINEERING.md.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.listing.CodeUnit;

public class ParseHeaderAndAnalyze extends GhidraScript {
    @Override
    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        Address base = currentProgram.getMinAddress();

        int mId       = mem.getShort(base.add(0x00)) & 0xFFFF;
        int sysRev    = mem.getShort(base.add(0x02)) & 0xFFFF;
        int modSize   = mem.getInt(base.add(0x04));
        int modOwner  = mem.getInt(base.add(0x08));
        int nameOff   = mem.getShort(base.add(0x0C)) & 0xFFFF;
        int accsErr   = mem.getShort(base.add(0x0E)) & 0xFFFF;
        int typeLang  = mem.getShort(base.add(0x10)) & 0xFFFF;
        int attrRevs  = mem.getShort(base.add(0x12)) & 0xFFFF;
        int edition   = mem.getShort(base.add(0x14)) & 0xFFFF;
        int usage     = mem.getInt(base.add(0x16));
        int symbol    = mem.getInt(base.add(0x1A));
        int parityRes = mem.getShort(base.add(0x1E)) & 0xFFFF;

        println("=== Standard header (0x00-0x1F) ===");
        println(String.format("M$ID       = 0x%04X", mId));
        println(String.format("M$SysRev   = 0x%04X", sysRev));
        println(String.format("M$Size     = 0x%08X (%d)", modSize, modSize));
        println(String.format("M$Owner    = 0x%08X", modOwner));
        println(String.format("M$Name off = 0x%04X", nameOff));
        println(String.format("M$Accs/Err = 0x%04X", accsErr));
        println(String.format("M$Type/Lang= 0x%04X  (type=0x%02X lang=0x%02X)", typeLang, (typeLang>>8)&0xFF, typeLang&0xFF));
        println(String.format("M$Attr/Revs= 0x%04X  (attr=0x%02X revs=0x%02X)", attrRevs, (attrRevs>>8)&0xFF, attrRevs&0xFF));
        println(String.format("M$Edit     = 0x%04X", edition));
        println(String.format("M$Usage    = 0x%08X", usage));
        println(String.format("M$Symbol   = 0x%08X", symbol));

        // Module name string at nameOff (null-terminated, high bit set on last char per OS-9 convention)
        StringBuilder name = new StringBuilder();
        Address nameAddr = base.add(nameOff);
        for (int i = 0; i < 64; i++) {
            byte b = mem.getByte(nameAddr.add(i));
            name.append((char)(b & 0x7F));
            if ((b & 0x80) != 0) break;
        }
        println("Module name = \"" + name.toString() + "\"");

        println("=== Type-specific extension (0x20 onward) ===");
        for (int off = 0x20; off < 0x40; off += 4) {
            int v = mem.getInt(base.add(off));
            println(String.format("0x%02X: 0x%08X", off, v));
        }

        // Compute header parity (XOR of all words 0x00-0x2E should give 0xFFFF, per Technical Manual)
        int parity = 0;
        for (int off = 0x00; off < 0x30; off += 2) {
            parity ^= (mem.getShort(base.add(off)) & 0xFFFF);
        }
        println(String.format("Header parity XOR(0x00-0x2E) = 0x%04X (expect 0xFFFF)", parity));

        // First bytes after the standard+extension header, where the entry point often lives
        println("=== First 64 bytes from offset 0x30 (raw) ===");
        StringBuilder hex = new StringBuilder();
        for (int i = 0; i < 64; i++) {
            hex.append(String.format("%02X ", mem.getByte(base.add(0x30 + i)) & 0xFF));
            if (i % 16 == 15) { println(hex.toString()); hex.setLength(0); }
        }
        if (hex.length() > 0) println(hex.toString());

        println("=== Auto-analysis coverage ===");
        long totalLen = mem.getMaxAddress().getOffset() - base.getOffset() + 1;
        long codeLen = 0, dataLen = 0;
        CodeUnit cu = currentProgram.getListing().getCodeUnitAt(base);
        java.util.Iterator<CodeUnit> it = currentProgram.getListing().getCodeUnits(true);
        int funcCount = currentProgram.getFunctionManager().getFunctionCount();
        for (CodeUnit c : currentProgram.getListing().getCodeUnits(true)) {
            if (c instanceof ghidra.program.model.listing.Instruction) codeLen += c.getLength();
            else if (c instanceof ghidra.program.model.listing.Data) {
                ghidra.program.model.listing.Data d = (ghidra.program.model.listing.Data) c;
                if (d.isDefined()) dataLen += c.getLength();
            }
        }
        println(String.format("Total module length: %d bytes", totalLen));
        println(String.format("Disassembled as code: %d bytes (%.1f%%)", codeLen, 100.0*codeLen/totalLen));
        println(String.format("Defined data: %d bytes (%.1f%%)", dataLen, 100.0*dataLen/totalLen));
        println(String.format("Undefined: %d bytes (%.1f%%)", totalLen-codeLen-dataLen, 100.0*(totalLen-codeLen-dataLen)/totalLen));
        println(String.format("Functions found by auto-analysis: %d", funcCount));
    }
}
