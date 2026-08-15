// BootRomAnalyzeAndDump.java -- disassembliert den echten Q9-Flux-Boot-ROM
// (romimage.dev.running.BIN, lokal unter Q9-Flux/local_images/roms/, NICHT
// im Repo) ab dem Reset-Einsprung (Vektor 1 = 0xFE000494) und dumpt die
// Funktionsliste + volle Disassemblierung nach stdout.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class BootRomAnalyzeAndDump extends GhidraScript {
    @Override
    public void run() throws Exception {
        Address base = currentProgram.getMinAddress(); // 0xFE000000 (per -loader-baseAddr)
        Address entry = base.add(0x494);
        disassemble(entry);
        try { createFunction(entry, "Q9_bootrom_reset_entry"); } catch (Exception e) { println("createFunction: " + e.getMessage()); }
        // die vier ROM-residenten Exception-Handler aus der Vektortabelle
        int[] handlerOffsets = {0x1b0e, 0x1b14, 0x1b1a, 0x1b20, 0x1b08};
        String[] handlerNames = {"Q9_bootrom_buserr", "Q9_bootrom_addrerr", "Q9_bootrom_illinstr", "Q9_bootrom_trace", "Q9_bootrom_nmi"};
        for (int i = 0; i < handlerOffsets.length; i++) {
            Address a = base.add(handlerOffsets[i]);
            disassemble(a);
            try { createFunction(a, handlerNames[i]); } catch (Exception e) { println("createFunction failed @ " + a + ": " + e.getMessage()); }
        }
        analyzeAll(currentProgram);

        println("=== FUNCTIONS ===");
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            println(f.getName() + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses());
        }

        println("=== DISASSEMBLY 0xFE000480-0xFE000B00 (Reset-Handler + Sprungtabelle) ===");
        Address rangeStart = base.add(0x480);
        Address rangeEnd = base.add(0xb00);
        InstructionIterator it = currentProgram.getListing().getInstructions(rangeStart, true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            if (ins.getAddress().compareTo(rangeEnd) > 0) break;
            println(ins.getAddress() + ": " + ins.toString());
        }
    }
}
