// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Cleanup stage in V3Width
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

#अगर_अघोषित VERILATOR_V3WIDTHREMOVE_H_
#घोषणा VERILATOR_V3WIDTHREMOVE_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Error.h"

// clang-format off
#अगर_अघोषित VERILATOR_V3WIDTH_CPP_
# error "V3WidthRemove for V3Width internal use only"
#पूर्ण_अगर
// clang-format on

//######################################################################

class WidthRemoveVisitor final : खुला VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    व्योम replaceWithSignedVersion(Ast_गाँठ* nodep, Ast_गाँठ* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    // VISITORS
    व्योम visit(AstSigned* nodep) override {
        VL_DO_DANGLING(replaceWithSignedVersion(nodep, nodep->lhsp()->unlinkFrBack()), nodep);
    }
    व्योम visit(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replaceWithSignedVersion(nodep, nodep->lhsp()->unlinkFrBack()), nodep);
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    WidthRemoveVisitor() = शेष;
    ~WidthRemoveVisitor() override = शेष;
    Ast_गाँठ* mainAcceptEdit(Ast_गाँठ* nodep) { वापस iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#पूर्ण_अगर  // Guard
