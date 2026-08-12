// Same as ioman's version, adapted for cfide. Dumps header + type-specific
// extension + runs auto-analysis + reports function list.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.address.AddressSetView;

public class ParseHeaderAndAnalyze extends GhidraScript {
    @Override
    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        Address base = currentProgram.getMinAddress();

        int mId = mem.getShort(base.add(0x00)) & 0xFFFF;
        int modSize = mem.getInt(base.add(0x04));
        int typeLang = mem.getShort(base.add(0x12)) & 0xFFFF;
        int attrRevs = mem.getShort(base.add(0x14)) & 0xFFFF;

        println(String.format("M$ID=0x%04X size=%d(0x%X) type=0x%02X lang=0x%02X attr=0x%02X revs=0x%02X",
                mId, modSize, modSize, (typeLang>>8)&0xFF, typeLang&0xFF, (attrRevs>>8)&0xFF, attrRevs&0xFF));

        int parity = 0;
        for (int off = 0x00; off < 0x30; off += 2) parity ^= (mem.getShort(base.add(off)) & 0xFFFF);
        println(String.format("Header parity = 0x%04X (expect 0xFFFF)", parity));

        println("=== bytes 0x20-0x50 ===");
        for (int off = 0x20; off < 0x50; off += 16) {
            StringBuilder hex = new StringBuilder();
            for (int i = 0; i < 16 && off+i < modSize; i++) hex.append(String.format("%02X ", mem.getByte(base.add(off+i)) & 0xFF));
            println(String.format("0x%02X: %s", off, hex.toString()));
        }

        int mExec = mem.getInt(base.add(0x30));
        println(String.format("M$Exec = 0x%X", mExec));
        Address entry = base.add(mExec);
        disassemble(entry);
        createFunction(entry, "cfide_entry_" + Integer.toHexString(mExec));
        analyzeAll(currentProgram);

        long totalLen = mem.getMaxAddress().getOffset() - base.getOffset() + 1;
        long codeLen = 0, dataLen = 0;
        for (CodeUnit c : currentProgram.getListing().getCodeUnits(true)) {
            if (c instanceof Instruction) codeLen += c.getLength();
            else if (c instanceof ghidra.program.model.listing.Data) {
                ghidra.program.model.listing.Data d = (ghidra.program.model.listing.Data) c;
                if (d.isDefined()) dataLen += c.getLength();
            }
        }
        println(String.format("Total=%d code=%d(%.1f%%) data=%d(%.1f%%) undef=%d(%.1f%%) funcs=%d",
                totalLen, codeLen, 100.0*codeLen/totalLen, dataLen, 100.0*dataLen/totalLen,
                totalLen-codeLen-dataLen, 100.0*(totalLen-codeLen-dataLen)/totalLen,
                currentProgram.getFunctionManager().getFunctionCount()));

        println("=== FUNCTIONS (full disasm) ===");
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            println("--- " + f.getName() + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses() + " ---");
            AddressSetView body = f.getBody();
            InstructionIterator it = currentProgram.getListing().getInstructions(body, true);
            while (it.hasNext()) {
                Instruction ins = it.next();
                println(String.format("  %s: %s", ins.getAddress(), ins.toString()));
            }
        }
    }
}
