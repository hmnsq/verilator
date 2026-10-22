// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// Code available from: https://verilator.org
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for precompiled header include
//

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3EmitC.h"
#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Precompiled header emitter

વર્ગ EmitCPch final : જાહેર EmitCBaseVisitorConst {
    // METHODS

    શૂન્ય emitPch() {
        openNewOutputHeaderFile(EmitCUtil::pchClassName(), "Precompiled header");
        puts("//\n");
        puts("// Internal details; most user sources do not need this header,\n");
        puts("// unless using verilator public meta comments.\n");
        puts("// Suggest use " + EmitCUtil::ટોચ_Class_Name() + ".h instead.\n");

        ofp()->putsGuard();

        puts("\n");
        puts("// GCC and Clang only will precompile headers (PCH) for the first header.\n");
        puts("// So, make sure this is the one and only PCH.\n");
        puts("// If multiple module's includes are needed, use individual includes.\n");
        puts("#ifdef VL_PCH_INCLUDED\n");
        puts("# error \"Including multiple precompiled header files\"\n");
        puts("#endif\n");
        puts("#define VL_PCH_INCLUDED\n");

        puts("\n");
        puts("\n#include \"verilated.h\"\n");
        જો (v3Global.dpi()) puts("#include \"verilated_dpi.h\"\n");

        puts("\n");
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("#include \"" + EmitCUtil::ટોચ_Class_Name() + ".h\"\n");

        puts("\n// Additional include files added using '--compiler-include'\n");
        માટે (અચલ string& filename : v3Global.opt.compilerIncludes()) {
            puts("#include \"" + filename + "\"\n");
        }

        ofp()->putsEndGuard();

        closeOutputFile();
    }

    // VISITOR
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { nodep->v3fatalSrc("Unused"); }

જાહેર:
    explicit EmitCPch() { emitPch(); }
};

//######################################################################
// EmitC static functions

શૂન્ય V3EmitC::emitcPch() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCPch{};
}
