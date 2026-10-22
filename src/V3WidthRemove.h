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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3WIDTHREMOVE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3WIDTHREMOVE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

// clang-format off
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3_ਚੌੜਾਈ_CPP
# error "V3WidthRemove for V3Width internal use only"
#ਜੇ_ਅੰਤ
// clang-format on

//######################################################################

class ਚੌੜਾਈ_Remove_Visitor final : ਜਨਤਕ VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    ਖਾਲੀ replace_With_Signed_ਸੰਸਕਰਣ(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstSigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_ਸੰਸਕਰਣ(nodep, nodep->lhsp()->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_ਸੰਸਕਰਣ(nodep, nodep->lhsp()->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਚੌੜਾਈ_Remove_Visitor() = ਮੂਲ;
    ~ਚੌੜਾਈ_Remove_Visitor() override = ਮੂਲ;
    Ast_ਗੰਢ* mainAcceptEdit(Ast_ਗੰਢ* nodep) { ਵਾਪਸ iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#ਜੇ_ਅੰਤ  // Guard
