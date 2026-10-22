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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitC.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Precompiled header emitter

ଶ୍ରେଣୀ EmitCPch final : ସର୍ବସାଧାରଣ EmitCBaseVisitorConst {
    // METHODS

    ଶୂନ୍ୟ emitPch() {
        openNewOutputHeaderFile(EmitCUtil::pchClassName(), "Precompiled header");
        puts("//\n");
        puts("// Internal details; most user sources do not need this header,\n");
        puts("// unless using verilator public meta comments.\n");
        puts("// Suggest use " + EmitCUtil::ଶୀର୍ଷ_Class_Name() + ".h instead.\n");

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
        ଯଦି (v3Global.dpi()) puts("#include \"verilated_dpi.h\"\n");

        puts("\n");
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("#include \"" + EmitCUtil::ଶୀର୍ଷ_Class_Name() + ".h\"\n");

        puts("\n// Additional include files added using '--compiler-include'\n");
        ପାଇଁ (ଅଚଳ string& filename : v3Global.opt.compilerIncludes()) {
            puts("#include \"" + filename + "\"\n");
        }

        ofp()->putsEndGuard();

        closeOutputFile();
    }

    // VISITOR
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { nodep->v3fatalSrc("Unused"); }

ସର୍ବସାଧାରଣ:
    explicit EmitCPch() { emitPch(); }
};

//######################################################################
// EmitC static functions

ଶୂନ୍ୟ V3EmitC::emitcPch() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCPch{};
}
