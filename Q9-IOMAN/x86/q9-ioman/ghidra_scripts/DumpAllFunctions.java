// DumpAllFunctions.java
// Dumps full disassembly listing (mnemonic + operands + comments where
// present) for every function currently defined in the program, in
// address order. Used to read IOMan's small early functions in one pass.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.address.AddressSetView;

public class DumpAllFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            println("=== FUNCTION " + f.getName() + " @ " + f.getEntryPoint() +
                    " size=" + f.getBody().getNumAddresses() + " ===");
            AddressSetView body = f.getBody();
            InstructionIterator it = currentProgram.getListing().getInstructions(body, true);
            while (it.hasNext()) {
                Instruction ins = it.next();
                println(String.format("%s: %-30s ; bytes=%s",
                        ins.getAddress(), ins.toString(), ins.getBytes().length));
            }
            println("");
        }
    }
}
