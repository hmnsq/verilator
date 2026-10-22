// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Change names for __PVT__'s
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
// V3Name's Transformations:
//
//      Cell/Var's
//              Prepend __PVT__ to variable names
//*************************************************************************

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Name.h"

#अन्तर्भाव "V3LanguageWords.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Name state, as a visitor of each AstNode

वर्ग NameVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstCell::user1()        -> bool.  Set true if already processed
    //  AstScope::user1()       -> bool.  Set true if already processed
    //  AstVar::user1()         -> bool.  Set true if already processed
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    ध्रुव Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module

    // Rename struct / union field properly
    std::सदिशम्<V3_Unique_नामानि> m_name_राशिः;  // Hierarchy-based renames

    शून्य renameKeywordCheck(Ast_ग्रन्थिः* nodep) {
        ध्रुव std::string rsvd = V3_Language_शब्दाः::isKeyword(nodep->name());
        यदि (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            ध्रुव string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_गणनम्_Inc();
        }
    }

    // METHODS
    शून्य rename(Ast_ग्रन्थिः* nodep, तार्किक सङ्कलय_Pvt) {
        यदि (!nodep->user1()) {  // Not already done
            यदि (सङ्कलय_Pvt) {
                ध्रुव string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_गणनम्_Inc();
            } अन्यथा यदि (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } अन्यथा यदि (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->is_Sig_लौकिकम्()) {
                ध्रुव string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_गणनम्_Inc();
            } अन्यथा {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        rename(nodep, असत्य);
        // Avoid clashing with the classes of the library model itself
        यदि (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    शून्य दर्शय(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        rename(nodep,
               ((!m_modp || !m_modp->is_शिखरम्()) && !nodep->is_Sig_लौकिकम्()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->is_अस्थायी()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstCFunc* nodep) override {
        यदि (!nodep->user1()) {
            iterateChildrenConst(nodep);
            rename(nodep, असत्य);
        }
    }
    शून्य दर्शय(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    शून्य दर्शय(AstCell* nodep) override {
        यदि (!nodep->user1()) {
            rename(nodep, (!nodep->modp()->mod_लौकिकम्() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_UOr_Struct_DType* nodep) override {
        यदि (nodep->packed()) {
            m_name_राशिः.emplace_पृष्ठभागः("", असत्य);
            m_name_राशिः.पृष्ठभागः().get("get");
            m_name_राशिः.पृष्ठभागः().get("set");
        }
        iterateChildrenConst(nodep);
        यदि (nodep->packed()) m_name_राशिः.निष्कासय_पृष्ठभागः();
    }
    शून्य दर्शय(AstMemberDType* nodep) override {
        यदि (!nodep->user1()) {
            यदि (!m_name_राशिः.रिक्तानि()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_राशिः.पृष्ठभागः().get(nodep->name()));
                nodep->user1(1);
            } अन्यथा {
                rename(nodep, सत्य);
            }
            iterateChildrenConst(nodep);
        }
    }
    शून्य दर्शय(AstMemberSel* nodep) override {
        यदि (!nodep->user1()) {
            rename(nodep, सत्य);
            iterateChildrenConst(nodep);
        }
    }
    शून्य दर्शय(AstStructSel* nodep) override {
        यदि (!nodep->user1()) {
            rename(nodep, सत्य);
            iterateChildrenConst(nodep);
        }
    }
    शून्य दर्शय(AstScope* nodep) override {
        यदि (!nodep->user1_समुच्चयः_Once()) {
            यदि (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            यदि (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
            // Always recompute name (as many levels above scope may have changed)
            // Same formula as V3Scope
            nodep->name(nodep->is_शिखरम्()         ? "TOP"
                        : VN_IS(m_modp, Class) ? ("TOP." + m_modp->name())
                        : VN_IS(m_modp, ClassPackage)
                            ? ("TOP." + m_modp->name())
                            : (nodep->aboveScopep()->name() + "." + nodep->aboveCellp()->name()));
            nodep->edit_गणनम्_Inc();
            iterateChildrenConst(nodep);
        }
    }

    //--------------------
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = स्वभाव;
};

//######################################################################
// Name class functions

शून्य V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("name", 0, dump_वृक्षः_Either_Level() >= 6);
}
