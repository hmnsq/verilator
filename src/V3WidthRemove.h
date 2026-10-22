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

#که_نه_تعریف_شوی VERILATOR_V3WIDTHREMOVE_H_
#تعریف_کړه VERILATOR_V3WIDTHREMOVE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"

// clang-format off
#که_نه_تعریف_شوی VERILATOR_V3_پلنوالی_CPP
# error "V3WidthRemove for V3Width internal use only"
#د_که_پای
// clang-format on

//######################################################################

class پلنوالی_Remove_Visitor final : عام VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    فراغ replace_With_Signed_نسخه(Ast_غوټه* nodep, Ast_غوټه* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    // VISITORS
    فراغ لیدنه(AstSigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_نسخه(nodep, nodep->lhsp()->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_نسخه(nodep, nodep->lhsp()->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    پلنوالی_Remove_Visitor() = افتراضي;
    ~پلنوالی_Remove_Visitor() override = افتراضي;
    Ast_غوټه* mainAcceptEdit(Ast_غوټه* nodep) { أرجع iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#د_که_پای  // Guard
