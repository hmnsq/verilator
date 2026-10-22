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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3EmitCMain.h"

#अन्तर्भाव "V3EmitC.h"
#अन्तर्भाव "V3EmitCBase.h"

#अन्तर्भाव <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग EmitCMain final : EmitCBaseVisitorConst {
    // METHODS

    // VISITORS
    // This visitor doesn't really iterate, but exist to appease base class
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }  // LCOV_EXCL_LINE

सार्वजनिक:
    // CONSTRUCTORS
    EmitCMain() { emitInt(); }

गुप्त:
    // MAIN METHOD
    शून्य emitInt() {
        // Not defining main_time/vl_time_stamp, so
        v3Global.opt.addCFlags("-DVL_TIME_CONTEXT");  // On MSVC++ anyways

        // Optional main top name argument, with empty string replacement
        string शीर्ष_Name = v3Global.opt.main_शीर्ष_Name();
        यदि (शीर्ष_Name == "-") शीर्ष_Name = "";

        openNewOutputSourceFile(EmitCUtil::शीर्ष_Class_Name() + "__main", असत्य, असत्य,
                                "main() simulation loop, created with --main");
        माला_दो("\n");

        // Heavily commented output, as users are likely to look at or copy this code

        माला_दो("#include \"verilated.h\"\n");
        यदि (v3Global.opt.vpi()) माला_दो("#include \"verilated_vpi.h\"\n");
        माला_दो("#include \"" + EmitCUtil::शीर्ष_Class_Name() + ".h\"\n");
        यदि (v3Global.opt.debugRuntimeTimeout()) {
            माला_दो("\n");
            माला_दो("#include <csignal>\n");
        }
        माला_दो("\n");

        यदि (v3Global.opt.vpi()) {
            माला_दो("// User VPI code adds to this array to get startup main() callbacks\n");
            माला_दो("extern \"C\" void (*vlog_startup_routines[])() VL_ATTR_WEAK;\n");
            माला_दो("\n");
        }

        माला_दो("//======================\n\n");

        यदि (v3Global.opt.debugRuntimeTimeout()) {
            माला_दो("void alarmHandler(int signum) {\n");
            // Add newline so %Error is at beginning-of-line, as might get the alarm
            // when in the middle of some other print with no newline
            माला_दो("    VL_PRINTF_MT(\"\\n\");\n");
            माला_दो("    VL_FATAL_MT(\"\", 0, \"\", \"Alarm signal received,"s
                 + " '--debug-runtime-timeout "s
                 + std::to_string(v3Global.opt.debugRuntimeTimeout()) + "' exceeded\\n\");\n");
            माला_दो("}\n");
            माला_दो("\n");
        }

        माला_दो("int main(int argc, char** argv, char**) {\n");
        माला_दो("// Setup context, defaults, and parse command line\n");
        माला_दो("Verilated::debug(0);\n");
        माला_दो("const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};\n");
        यदि (v3Global.opt.trace()) माला_दो("contextp->traceEverOn(true);\n");
        माला_दो("contextp->threads(" + std::to_string(v3Global.opt.threads()) + ");\n");
        माला_दो("contextp->commandArgs(argc, argv);\n");
        माला_दो("\n");

        यदि (v3Global.opt.debugRuntimeTimeout()) {
            माला_दो("signal(SIGALRM, alarmHandler);\n");
            माला_दो("alarm("s + std::to_string(v3Global.opt.debugRuntimeTimeout()) + ");\n");
            माला_दो("\n");
        }

        माला_दो("// Construct the Verilated model, from Vtop.h generated from Verilating\n");
        माला_दो("const std::unique_ptr<" + EmitCUtil::शीर्ष_Class_Name() + "> topp{new "
             + EmitCUtil::शीर्ष_Class_Name() + "{contextp.get(), \"" + शीर्ष_Name + "\"}};\n");
        माला_दो("\n");

        यदि (v3Global.opt.vpi()) {
            // VPI shared libraries requested via +verilator+vpi+<lib> are loaded by
            // contextp->commandArgs() above, before the statically-linked startup routines.
            माला_दो("// Hook VPI startup routines and invoke callback\n");
            माला_दो("if (vlog_startup_routines) {\n");
            माला_दो(/**/ "for (auto routinep = &vlog_startup_routines[0]; *routinep; routinep++)"
                      " (*routinep)();\n");
            माला_दो("}\n");
            माला_दो("VerilatedVpi::callCbs(cbStartOfSimulation);\n");
            माला_दो("\n");
        }

        माला_दो("// Simulate until $finish\n");
        माला_दो("while (VL_LIKELY(!contextp->gotFinish())) {\n");
        यदि (v3Global.opt.vpi()) {
            माला_दो(/**/ "// VPI callbacks\n");
            माला_दो(/**/ "VerilatedVpi::callTimedCbs();\n");
            माला_दो(/**/ "VerilatedVpi::callCbs(cbNextSimTime);\n");  // Before next event queue
            माला_दो(/**/ "VerilatedVpi::callCbs(cbAtStartOfSimTime);\n");  // Before time queue
        }
        माला_दो(/**/ "// Evaluate model\n");
        माला_दो(/**/ "topp->eval();\n");
        यदि (v3Global.opt.vpi()) {
            माला_दो(/**/ "// VPI callbacks\n");
            माला_दो(/**/ "VerilatedVpi::callValueCbs();\n");
            माला_दो(/**/ "VerilatedVpi::callCbs(cbAtEndOfSimTime);\n");  // After nonblocking events
            माला_दो(/**/ "VerilatedVpi::callCbs(cbReadWriteSynch);\n");  // After a specified time
            माला_दो(/**/ "VerilatedVpi::callCbs(cbReadOnlySynch);\n");  // After cbReadWriteSynch
        }
        माला_दो(/**/ "// Advance time\n");
        यदि (v3Global.rootp()->delaySchedulerp() || v3Global.opt.timing()) {
            माला_दो("if (!topp->eventsPending()) break;\n");
        }
        ध्रुव std::string nextSlot = v3Global.rootp()->delaySchedulerp() ? "topp->nextTimeSlot()"
                                                                         : "contextp->time() + 1";
        यदि (v3Global.opt.vpi()) {
            माला_दो("contextp->time(std::min("s + nextSlot + ", VerilatedVpi::cbNextDeadline()));\n");
        } अन्यथा {
            माला_दो("contextp->time("s + nextSlot + ");\n");
        }

        माला_दो("}\n");
        माला_दो("\n");

        माला_दो("if (VL_LIKELY(!contextp->gotFinish())) {\n");
        माला_दो(/**/ "VL_DEBUG_IF(VL_PRINTF(\"+ Exiting without $finish; no events left\\n\"););\n");
        माला_दो("}\n");
        माला_दो("\n");

        माला_दो("// Execute 'final' processes\n");
        माला_दो("topp->final();\n");
        यदि (v3Global.opt.vpi()) माला_दो("VerilatedVpi::callCbs(cbEndOfSimulation);\n");
        माला_दो("\n");

        यदि (v3Global.opt.coverage()) {
            माला_दो("// Write coverage data (since Verilated with --coverage)\n");
            यदि (v3Global.opt.coveragePerInstance()) {
                माला_दो("contextp->coveragep()->forcePerInstance(true);\n");
            }
            माला_दो("contextp->coveragep()->write();\n");
            माला_दो("\n");
        }

        माला_दो("// Print statistical summary report\n");
        माला_दो("contextp->statsPrintSummary();\n");
        माला_दो("\n");

        माला_दो("return 0;\n");
        माला_दो("}\n");

        closeOutputFile();
    }
};

//######################################################################
// EmitC class functions

शून्य V3EmitCMain::emit() {
    UINFO(2, __FUNCTION__ << ":");
    { ध्रुव EmitCMain visitor; }
}
