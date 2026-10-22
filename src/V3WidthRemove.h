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

#यदि_अपरिभाषितम् VERILATOR_V3WIDTHREMOVE_H_
#परिभाषय VERILATOR_V3WIDTHREMOVE_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Error.h"

// clang-format off
#यदि_अपरिभाषितम् VERILATOR_V3_विस्तारः_CPP
# error "V3WidthRemove for V3Width internal use only"
#यदि_अन्तः
// clang-format on

//######################################################################

class विस्तारः_Remove_Visitor final : सार्वजनिक VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    शून्य replace_With_Signed_संस्करणम्(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    // VISITORS
    शून्य दर्शय(AstSigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_संस्करणम्(nodep, nodep->lhsp()->unlink_Fr_पृष्ठभागः()), nodep);
    }
    शून्य दर्शय(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_संस्करणम्(nodep, nodep->lhsp()->unlink_Fr_पृष्ठभागः()), nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    विस्तारः_Remove_Visitor() = स्वभाव;
    ~विस्तारः_Remove_Visitor() override = स्वभाव;
    Ast_ग्रन्थिः* mainAcceptEdit(Ast_ग्रन्थिः* nodep) { प्रत्यागम iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#यदि_अन्तः  // Guard
