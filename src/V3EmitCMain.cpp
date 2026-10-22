// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for tree
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3EmitCMain.h"

#ಅನ್ತರ್ಭಾವ "V3EmitC.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"

#ಅನ್ತರ್ಭಾವ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ EmitCMain final : EmitCBaseVisitorConst {
    // METHODS

    // VISITORS
    // This visitor doesn't really iterate, but exist to appease base class
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }  // LCOV_EXCL_LINE

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    EmitCMain() { emitInt(); }

ಖಾಸಗಿ:
    // MAIN METHOD
    ಶೂನ್ಯ emitInt() {
        // Not defining main_time/vl_time_stamp, so
        v3Global.opt.ಕೂಡಿಸು_CFlags("-DVL_TIME_CONTEXT");  // On MSVC++ anyways

        // Optional main top name argument, with empty string replacement
        string ತುದಿ_Name = v3Global.opt.main_ತುದಿ_Name();
        ಆದರೆ (ತುದಿ_Name == "-") ತುದಿ_Name = "";

        open_New_Output_ಮೂಲ_File(EmitCUtil::ತುದಿ_Class_Name() + "__main", ಸುಳ್ಳು, ಸುಳ್ಳು,
                                "main() simulation loop, created with --main");
        puts("\n");

        // Heavily commented output, as users are likely to look at or copy this code

        puts("#include \"verilated.h\"\n");
        ಆದರೆ (v3Global.opt.vpi()) puts("#include \"verilated_vpi.h\"\n");
        puts("#include \"" + EmitCUtil::ತುದಿ_Class_Name() + ".h\"\n");
        ಆದರೆ (v3Global.opt.debugRuntimeTimeout()) {
            puts("\n");
            puts("#include <csignal>\n");
        }
        puts("\n");

        ಆದರೆ (v3Global.opt.vpi()) {
            puts("// User VPI code adds to this array to get startup main() callbacks\n");
            puts("extern \"C\" void (*vlog_startup_routines[])() VL_ATTR_WEAK;\n");
            puts("\n");
        }

        puts("//======================\n\n");

        ಆದರೆ (v3Global.opt.debugRuntimeTimeout()) {
            puts("void alarmHandler(int signum) {\n");
            // Add newline so %Error is at beginning-of-line, as might get the alarm
            // when in the middle of some other print with no newline
            puts("    VL_PRINTF_MT(\"\\n\");\n");
            puts("    VL_FATAL_MT(\"\", 0, \"\", \"Alarm signal received,"s
                 + " '--debug-runtime-timeout "s
                 + std::ಗಮ್ಯ_string(v3Global.opt.debugRuntimeTimeout()) + "' exceeded\\n\");\n");
            puts("}\n");
            puts("\n");
        }

        puts("int main(int argc, char** argv, char**) {\n");
        puts("// Setup context, defaults, and parse command line\n");
        puts("Verilated::debug(0);\n");
        puts("const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};\n");
        ಆದರೆ (v3Global.opt.trace()) puts("contextp->traceEverOn(true);\n");
        puts("contextp->threads(" + std::ಗಮ್ಯ_string(v3Global.opt.ಎಳೆಗಳು()) + ");\n");
        puts("contextp->commandArgs(argc, argv);\n");
        puts("\n");

        ಆದರೆ (v3Global.opt.debugRuntimeTimeout()) {
            puts("signal(SIGALRM, alarmHandler);\n");
            puts("alarm("s + std::ಗಮ್ಯ_string(v3Global.opt.debugRuntimeTimeout()) + ");\n");
            puts("\n");
        }

        puts("// Construct the Verilated model, from Vtop.h generated from Verilating\n");
        puts("const std::unique_ptr<" + EmitCUtil::ತುದಿ_Class_Name() + "> topp{new "
             + EmitCUtil::ತುದಿ_Class_Name() + "{contextp.get(), \"" + ತುದಿ_Name + "\"}};\n");
        puts("\n");

        ಆದರೆ (v3Global.opt.vpi()) {
            // VPI shared libraries requested via +verilator+vpi+<lib> are loaded by
            // contextp->commandArgs() above, before the statically-linked startup routines.
            puts("// Hook VPI startup routines and invoke callback\n");
            puts("if (vlog_startup_routines) {\n");
            puts(/**/ "for (auto routinep = &vlog_startup_routines[0]; *routinep; routinep++)"
                      " (*routinep)();\n");
            puts("}\n");
            puts("VerilatedVpi::callCbs(cbStartOfSimulation);\n");
            puts("\n");
        }

        puts("// Simulate until $finish\n");
        puts("while (VL_LIKELY(!contextp->gotFinish())) {\n");
        ಆದರೆ (v3Global.opt.vpi()) {
            puts(/**/ "// VPI callbacks\n");
            puts(/**/ "VerilatedVpi::callTimedCbs();\n");
            puts(/**/ "VerilatedVpi::callCbs(cbNextSimTime);\n");  // Before next event queue
            puts(/**/ "VerilatedVpi::callCbs(cbAtStartOfSimTime);\n");  // Before time queue
        }
        puts(/**/ "// Evaluate model\n");
        puts(/**/ "topp->eval();\n");
        ಆದರೆ (v3Global.opt.vpi()) {
            puts(/**/ "// VPI callbacks\n");
            puts(/**/ "VerilatedVpi::callValueCbs();\n");
            puts(/**/ "VerilatedVpi::callCbs(cbAtEndOfSimTime);\n");  // After nonblocking events
            puts(/**/ "VerilatedVpi::callCbs(cbReadWriteSynch);\n");  // After a specified time
            puts(/**/ "VerilatedVpi::callCbs(cbReadOnlySynch);\n");  // After cbReadWriteSynch
        }
        puts(/**/ "// Advance time\n");
        ಆದರೆ (v3Global.rootp()->delaySchedulerp() || v3Global.opt.timing()) {
            puts("if (!topp->eventsPending()) break;\n");
        }
        ಸ್ಥಿರಾಂಕ std::string ಮುಂದಿನ_ಸ್ಥಾನ = v3Global.rootp()->delaySchedulerp() ? "topp->nextTimeSlot()"
                                                                         : "contextp->time() + 1";
        ಆದರೆ (v3Global.opt.vpi()) {
            puts("contextp->time(std::min("s + ಮುಂದಿನ_ಸ್ಥಾನ + ", VerilatedVpi::cbNextDeadline()));\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("contextp->time("s + ಮುಂದಿನ_ಸ್ಥಾನ + ");\n");
        }

        puts("}\n");
        puts("\n");

        puts("if (VL_LIKELY(!contextp->gotFinish())) {\n");
        puts(/**/ "VL_DEBUG_IF(VL_PRINTF(\"+ Exiting without $finish; no events left\\n\"););\n");
        puts("}\n");
        puts("\n");

        puts("// Execute 'final' processes\n");
        puts("topp->final();\n");
        ಆದರೆ (v3Global.opt.vpi()) puts("VerilatedVpi::callCbs(cbEndOfSimulation);\n");
        puts("\n");

        ಆದರೆ (v3Global.opt.coverage()) {
            puts("// Write coverage data (since Verilated with --coverage)\n");
            ಆದರೆ (v3Global.opt.coverage_ಪ್ರತಿ_Instance()) {
                puts("contextp->coveragep()->forcePerInstance(true);\n");
            }
            puts("contextp->coveragep()->write();\n");
            puts("\n");
        }

        puts("// Print statistical summary report\n");
        puts("contextp->statsPrintSummary();\n");
        puts("\n");

        puts("return 0;\n");
        puts("}\n");

        closeOutputFile();
    }
};

//######################################################################
// EmitC class functions

ಶೂನ್ಯ V3EmitCMain::emit() {
    UINFO(2, __FUNCTION__ << ":");
    { ಸ್ಥಿರಾಂಕ EmitCMain visitor; }
}
