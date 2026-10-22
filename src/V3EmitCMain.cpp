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
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }  // LCOV_EXCL_LINE

सार्वजनिक:
    // CONSTRUCTORS
    EmitCMain() { emitInt(); }

गुप्त:
    // MAIN METHOD
    शून्य emitInt() {
        // Not defining main_time/vl_time_stamp, so
        v3Global.opt.सङ्कलय_CFlags("-DVL_TIME_CONTEXT");  // On MSVC++ anyways

        // Optional main top name argument, with empty string replacement
        string शिखरम्_Name = v3Global.opt.main_शिखरम्_Name();
        यदि (शिखरम्_Name == "-") शिखरम्_Name = "";

        openNewOutputSourceFile(EmitCUtil::शिखरम्_Class_Name() + "__main", असत्य, असत्य,
                                "main() simulation loop, created with --main");
        पङ्क्तिं_लिख("\n");

        // Heavily commented output, as users are likely to look at or copy this code

        पङ्क्तिं_लिख("#include \"verilated.h\"\n");
        यदि (v3Global.opt.vpi()) पङ्क्तिं_लिख("#include \"verilated_vpi.h\"\n");
        पङ्क्तिं_लिख("#include \"" + EmitCUtil::शिखरम्_Class_Name() + ".h\"\n");
        यदि (v3Global.opt.debugRuntimeTimeout()) {
            पङ्क्तिं_लिख("\n");
            पङ्क्तिं_लिख("#include <csignal>\n");
        }
        पङ्क्तिं_लिख("\n");

        यदि (v3Global.opt.vpi()) {
            पङ्क्तिं_लिख("// User VPI code adds to this array to get startup main() callbacks\n");
            पङ्क्तिं_लिख("extern \"C\" void (*vlog_startup_routines[])() VL_ATTR_WEAK;\n");
            पङ्क्तिं_लिख("\n");
        }

        पङ्क्तिं_लिख("//======================\n\n");

        यदि (v3Global.opt.debugRuntimeTimeout()) {
            पङ्क्तिं_लिख("void alarmHandler(int signum) {\n");
            // Add newline so %Error is at beginning-of-line, as might get the alarm
            // when in the middle of some other print with no newline
            पङ्क्तिं_लिख("    VL_PRINTF_MT(\"\\n\");\n");
            पङ्क्तिं_लिख("    VL_FATAL_MT(\"\", 0, \"\", \"Alarm signal received,"s
                 + " '--debug-runtime-timeout "s
                 + std::गन्तव्यम्_string(v3Global.opt.debugRuntimeTimeout()) + "' exceeded\\n\");\n");
            पङ्क्तिं_लिख("}\n");
            पङ्क्तिं_लिख("\n");
        }

        पङ्क्तिं_लिख("int main(int argc, char** argv, char**) {\n");
        पङ्क्तिं_लिख("// Setup context, defaults, and parse command line\n");
        पङ्क्तिं_लिख("Verilated::debug(0);\n");
        पङ्क्तिं_लिख("const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};\n");
        यदि (v3Global.opt.trace()) पङ्क्तिं_लिख("contextp->traceEverOn(true);\n");
        पङ्क्तिं_लिख("contextp->threads(" + std::गन्तव्यम्_string(v3Global.opt.तन्तवः()) + ");\n");
        पङ्क्तिं_लिख("contextp->commandArgs(argc, argv);\n");
        पङ्क्तिं_लिख("\n");

        यदि (v3Global.opt.debugRuntimeTimeout()) {
            पङ्क्तिं_लिख("signal(SIGALRM, alarmHandler);\n");
            पङ्क्तिं_लिख("alarm("s + std::गन्तव्यम्_string(v3Global.opt.debugRuntimeTimeout()) + ");\n");
            पङ्क्तिं_लिख("\n");
        }

        पङ्क्तिं_लिख("// Construct the Verilated model, from Vtop.h generated from Verilating\n");
        पङ्क्तिं_लिख("const std::unique_ptr<" + EmitCUtil::शिखरम्_Class_Name() + "> topp{new "
             + EmitCUtil::शिखरम्_Class_Name() + "{contextp.get(), \"" + शिखरम्_Name + "\"}};\n");
        पङ्क्तिं_लिख("\n");

        यदि (v3Global.opt.vpi()) {
            // VPI shared libraries requested via +verilator+vpi+<lib> are loaded by
            // contextp->commandArgs() above, before the statically-linked startup routines.
            पङ्क्तिं_लिख("// Hook VPI startup routines and invoke callback\n");
            पङ्क्तिं_लिख("if (vlog_startup_routines) {\n");
            पङ्क्तिं_लिख(/**/ "for (auto routinep = &vlog_startup_routines[0]; *routinep; routinep++)"
                      " (*routinep)();\n");
            पङ्क्तिं_लिख("}\n");
            पङ्क्तिं_लिख("VerilatedVpi::callCbs(cbStartOfSimulation);\n");
            पङ्क्तिं_लिख("\n");
        }

        पङ्क्तिं_लिख("// Simulate until $finish\n");
        पङ्क्तिं_लिख("while (VL_LIKELY(!contextp->gotFinish())) {\n");
        यदि (v3Global.opt.vpi()) {
            पङ्क्तिं_लिख(/**/ "// VPI callbacks\n");
            पङ्क्तिं_लिख(/**/ "VerilatedVpi::callTimedCbs();\n");
            पङ्क्तिं_लिख(/**/ "VerilatedVpi::callCbs(cbNextSimTime);\n");  // Before next event queue
            पङ्क्तिं_लिख(/**/ "VerilatedVpi::callCbs(cbAtStartOfSimTime);\n");  // Before time queue
        }
        पङ्क्तिं_लिख(/**/ "// Evaluate model\n");
        पङ्क्तिं_लिख(/**/ "topp->eval();\n");
        यदि (v3Global.opt.vpi()) {
            पङ्क्तिं_लिख(/**/ "// VPI callbacks\n");
            पङ्क्तिं_लिख(/**/ "VerilatedVpi::callValueCbs();\n");
            पङ्क्तिं_लिख(/**/ "VerilatedVpi::callCbs(cbAtEndOfSimTime);\n");  // After nonblocking events
            पङ्क्तिं_लिख(/**/ "VerilatedVpi::callCbs(cbReadWriteSynch);\n");  // After a specified time
            पङ्क्तिं_लिख(/**/ "VerilatedVpi::callCbs(cbReadOnlySynch);\n");  // After cbReadWriteSynch
        }
        पङ्क्तिं_लिख(/**/ "// Advance time\n");
        यदि (v3Global.rootp()->delaySchedulerp() || v3Global.opt.timing()) {
            पङ्क्तिं_लिख("if (!topp->eventsPending()) break;\n");
        }
        ध्रुव std::string अग्र_Slot = v3Global.rootp()->delaySchedulerp() ? "topp->nextTimeSlot()"
                                                                         : "contextp->time() + 1";
        यदि (v3Global.opt.vpi()) {
            पङ्क्तिं_लिख("contextp->time(std::min("s + अग्र_Slot + ", VerilatedVpi::cbNextDeadline()));\n");
        } अन्यथा {
            पङ्क्तिं_लिख("contextp->time("s + अग्र_Slot + ");\n");
        }

        पङ्क्तिं_लिख("}\n");
        पङ्क्तिं_लिख("\n");

        पङ्क्तिं_लिख("if (VL_LIKELY(!contextp->gotFinish())) {\n");
        पङ्क्तिं_लिख(/**/ "VL_DEBUG_IF(VL_PRINTF(\"+ Exiting without $finish; no events left\\n\"););\n");
        पङ्क्तिं_लिख("}\n");
        पङ्क्तिं_लिख("\n");

        पङ्क्तिं_लिख("// Execute 'final' processes\n");
        पङ्क्तिं_लिख("topp->final();\n");
        यदि (v3Global.opt.vpi()) पङ्क्तिं_लिख("VerilatedVpi::callCbs(cbEndOfSimulation);\n");
        पङ्क्तिं_लिख("\n");

        यदि (v3Global.opt.coverage()) {
            पङ्क्तिं_लिख("// Write coverage data (since Verilated with --coverage)\n");
            यदि (v3Global.opt.coverage_प्रत्येकम्_Instance()) {
                पङ्क्तिं_लिख("contextp->coveragep()->forcePerInstance(true);\n");
            }
            पङ्क्तिं_लिख("contextp->coveragep()->write();\n");
            पङ्क्तिं_लिख("\n");
        }

        पङ्क्तिं_लिख("// Print statistical summary report\n");
        पङ्क्तिं_लिख("contextp->statsPrintSummary();\n");
        पङ्क्तिं_लिख("\n");

        पङ्क्तिं_लिख("return 0;\n");
        पङ्क्तिं_लिख("}\n");

        closeOutputFile();
    }
};

//######################################################################
// EmitC class functions

शून्य V3EmitCMain::emit() {
    UINFO(2, __FUNCTION__ << ":");
    { ध्रुव EmitCMain visitor; }
}
