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

#समावेश "V3PchAstMT.h"

#समावेश "V3EmitC.h"
#समावेश "V3EmitCBase.h"
#समावेश "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Precompiled header emitter

श्रेणी EmitCPch final : खुला EmitCBaseVisitorConst {
    // METHODS

    व्योम emitPch() {
        openNewOutputHeaderFile(EmitCUtil::pchClassName(), "Precompiled header");
        माला_दो("//\n");
        माला_दो("// Internal details; most user sources do not need this header,\n");
        माला_दो("// unless using verilator public meta comments.\n");
        माला_दो("// Suggest use " + EmitCUtil::शीर्ष_Class_Name() + ".h instead.\n");

        ofp()->putsGuard();

        माला_दो("\n");
        माला_दो("// GCC and Clang only will precompile headers (PCH) for the first header.\n");
        माला_दो("// So, make sure this is the one and only PCH.\n");
        माला_दो("// If multiple module's includes are needed, use individual includes.\n");
        माला_दो("#ifdef VL_PCH_INCLUDED\n");
        माला_दो("# error \"Including multiple precompiled header files\"\n");
        माला_दो("#endif\n");
        माला_दो("#define VL_PCH_INCLUDED\n");

        माला_दो("\n");
        माला_दो("\n#include \"verilated.h\"\n");
        अगर (v3Global.dpi()) माला_दो("#include \"verilated_dpi.h\"\n");

        माला_दो("\n");
        माला_दो("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        माला_दो("#include \"" + EmitCUtil::शीर्ष_Class_Name() + ".h\"\n");

        माला_दो("\n// Additional include files added using '--compiler-include'\n");
        क्रम (स्थिर string& filename : v3Global.opt.compilerIncludes()) {
            माला_दो("#include \"" + filename + "\"\n");
        }

        ofp()->putsEndGuard();

        closeOutputFile();
    }

    // VISITOR
    व्योम visit(Ast_गाँठ* nodep) override { nodep->v3fatalSrc("Unused"); }

खुला:
    explicit EmitCPch() { emitPch(); }
};

//######################################################################
// EmitC static functions

व्योम V3EmitC::emitcPch() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCPch{};
}
