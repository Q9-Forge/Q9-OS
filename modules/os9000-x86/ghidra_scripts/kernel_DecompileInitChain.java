// kernel_DecompileInitChain.java
// Decompiles the kernel init function (found via the M$Exec-style trampoline
// at header offset 0x24) and follows its direct callees a few levels deep,
// printing pseudo-C for each so the initialization sequence can be read
// function-by-function rather than in raw x86 assembly.

import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.Function;
import ghidra.program.model.address.Address;
import ghidra.program.model.symbol.Reference;
import ghidra.util.task.ConsoleTaskMonitor;

import java.util.*;

public class kernel_DecompileInitChain extends GhidraScript {

    private DecompInterface decomp;
    private Set<Long> visited = new HashSet<>();

    @Override
    public void run() throws Exception {
        decomp = new DecompInterface();
        decomp.setOptions(new DecompileOptions());
        decomp.openProgram(currentProgram);

        Address base = currentProgram.getMinAddress();
        int execOff = currentProgram.getMemory().getInt(base.add(0x24));
        Address trampoline = base.add(execOff);

        Function entryFn = currentProgram.getFunctionManager().getFunctionContaining(
            getInstructionAt(trampoline).getFlows()[0]);

        println("=== Init-Funktion und direkte Aufrufer-Kette (Tiefe 3) ===");
        walk(entryFn, 0, 3);

        decomp.dispose();
    }

    private void walk(Function f, int depth, int maxDepth) throws Exception {
        if (f == null || depth > maxDepth) return;
        long key = f.getEntryPoint().getOffset();
        if (visited.contains(key)) {
            println("--- " + f.getName() + " @ " + f.getEntryPoint() + " [bereits gezeigt, übersprungen] ---");
            return;
        }
        visited.add(key);

        println("\n########## Tiefe " + depth + ": " + f.getName() + " @ " + f.getEntryPoint()
                + "  (size=" + f.getBody().getNumAddresses() + ") ##########");

        DecompileResults res = decomp.decompileFunction(f, 60, new ConsoleTaskMonitor());
        if (res != null && res.decompileCompleted()) {
            println(res.getDecompiledFunction().getC());
        } else {
            println("[Dekompilierung fehlgeschlagen: " + (res != null ? res.getErrorMessage() : "null") + "]");
        }

        // find direct callees (functions this one calls)
        List<Function> callees = new ArrayList<>();
        for (var instrIt = currentProgram.getListing().getInstructions(f.getBody(), true); instrIt.hasNext(); ) {
            var insn = instrIt.next();
            if (insn.getFlowType().isCall()) {
                for (Address flow : insn.getFlows()) {
                    Function callee = currentProgram.getFunctionManager().getFunctionAt(flow);
                    if (callee == null) callee = currentProgram.getFunctionManager().getFunctionContaining(flow);
                    if (callee != null && !callee.equals(f)) callees.add(callee);
                }
            }
        }
        if (depth < maxDepth) {
            for (Function callee : callees) {
                walk(callee, depth + 1, maxDepth);
            }
        } else if (!callees.isEmpty()) {
            println("  [Aufgerufene Funktionen bei maximaler Tiefe, nicht weiter verfolgt: "
                + callees.stream().map(c -> c.getName() + "@" + c.getEntryPoint()).distinct().reduce((a,b)->a+", "+b).orElse("") + "]");
        }
    }
}
