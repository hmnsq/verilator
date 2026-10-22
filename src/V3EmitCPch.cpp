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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3EmitC.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Precompiled header emitter

वर्ग EmitCPch final : सार्वजनिक EmitCBaseVisitorConst {
    // METHODS

    शून्य emitPch() {
        open_New_Output_शीर्षकम्_File(EmitCUtil::pchClassName(), "Precompiled header");
        पङ्क्तिं_लिख("//\n");
        पङ्क्तिं_लिख("// Internal details; most user sources do not need this header,\n");
        पङ्क्तिं_लिख("// unless using verilator public meta comments.\n");
        पङ्क्तिं_लिख("// Suggest use " + EmitCUtil::शिखरम्_Class_Name() + ".h instead.\n");

        ofp()->putsGuard();

        पङ्क्तिं_लिख("\n");
        पङ्क्तिं_लिख("// GCC and Clang only will precompile headers (PCH) for the first header.\n");
        पङ्क्तिं_लिख("// So, make sure this is the one and only PCH.\n");
        पङ्क्तिं_लिख("// If multiple module's includes are needed, use individual includes.\n");
        पङ्क्तिं_लिख("#ifdef VL_PCH_INCLUDED\n");
        पङ्क्तिं_लिख("# error \"Including multiple precompiled header files\"\n");
        पङ्क्तिं_लिख("#endif\n");
        पङ्क्तिं_लिख("#define VL_PCH_INCLUDED\n");

        पङ्क्तिं_लिख("\n");
        पङ्क्तिं_लिख("\n#include \"verilated.h\"\n");
        यदि (v3Global.dpi()) पङ्क्तिं_लिख("#include \"verilated_dpi.h\"\n");

        पङ्क्तिं_लिख("\n");
        पङ्क्तिं_लिख("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        पङ्क्तिं_लिख("#include \"" + EmitCUtil::शिखरम्_Class_Name() + ".h\"\n");

        पङ्क्तिं_लिख("\n// Additional include files added using '--compiler-include'\n");
        प्रति (ध्रुव string& filename : v3Global.opt.compilerIncludes()) {
            पङ्क्तिं_लिख("#include \"" + filename + "\"\n");
        }

        ofp()->puts_अन्तः_Guard();

        closeOutputFile();
    }

    // VISITOR
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { nodep->v3fatalSrc("Unused"); }

सार्वजनिक:
    स्पष्ट EmitCPch() { emitPch(); }
};

//######################################################################
// EmitC static functions

शून्य V3EmitC::emitcPch() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCPch{};
}
