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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Name.h"

#ಅನ್ತರ್ಭಾವ "V3LanguageWords.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Name state, as a visitor of each AstNode

ವರ್ಗ NameVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstCell::user1()        -> bool.  Set true if already processed
    //  AstScope::user1()       -> bool.  Set true if already processed
    //  AstVar::user1()         -> bool.  Set true if already processed
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module

    // Rename struct / union field properly
    std::ಸದಿಶ<V3_Unique_ಹೆಸರುಗಳು> m_name_ರಾಶಿ;  // Hierarchy-based renames

    ಶೂನ್ಯ renameKeywordCheck(Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ std::string rsvd = V3_Language_ಪದಗಳು::isKeyword(nodep->name());
        ಆದರೆ (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            ಸ್ಥಿರಾಂಕ string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_ಎಣಿಕೆ_Inc();
        }
    }

    // METHODS
    ಶೂನ್ಯ rename(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ ಕೂಡಿಸು_Pvt) {
        ಆದರೆ (!nodep->user1()) {  // Not already done
            ಆದರೆ (ಕೂಡಿಸು_Pvt) {
                ಸ್ಥಿರಾಂಕ string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_ಎಣಿಕೆ_Inc();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->is_Sig_ಬಹಿರಂಗ()) {
                ಸ್ಥಿರಾಂಕ string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_ಎಣಿಕೆ_Inc();
            } ಇಲ್ಲದಿದ್ದರೆ {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        rename(nodep, ಸುಳ್ಳು);
        // Avoid clashing with the classes of the library model itself
        ಆದರೆ (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        rename(nodep,
               ((!m_modp || !m_modp->is_ತುದಿ()) && !nodep->is_Sig_ಬಹಿರಂಗ()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->is_ತಾತ್ಕಾಲಿಕ()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        ಆದರೆ (!nodep->user1()) {
            iterateChildrenConst(nodep);
            rename(nodep, ಸುಳ್ಳು);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        ಆದರೆ (!nodep->user1()) {
            rename(nodep, (!nodep->modp()->mod_ಬಹಿರಂಗ() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        ಆದರೆ (nodep->packed()) {
            m_name_ರಾಶಿ.emplace_ಹಿಂಭಾಗ("", ಸುಳ್ಳು);
            m_name_ರಾಶಿ.ಹಿಂಭಾಗ().get("get");
            m_name_ರಾಶಿ.ಹಿಂಭಾಗ().get("set");
        }
        iterateChildrenConst(nodep);
        ಆದರೆ (nodep->packed()) m_name_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberDType* nodep) override {
        ಆದರೆ (!nodep->user1()) {
            ಆದರೆ (!m_name_ರಾಶಿ.ಖಾಲಿಗಳು()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_ರಾಶಿ.ಹಿಂಭಾಗ().get(nodep->name()));
                nodep->user1(1);
            } ಇಲ್ಲದಿದ್ದರೆ {
                rename(nodep, ನಿಜ);
            }
            iterateChildrenConst(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        ಆದರೆ (!nodep->user1()) {
            rename(nodep, ನಿಜ);
            iterateChildrenConst(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStructSel* nodep) override {
        ಆದರೆ (!nodep->user1()) {
            rename(nodep, ನಿಜ);
            iterateChildrenConst(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        ಆದರೆ (!nodep->user1_ಗಣ_Once()) {
            ಆದರೆ (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            ಆದರೆ (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
            // Always recompute name (as many levels above scope may have changed)
            // Same formula as V3Scope
            nodep->name(nodep->is_ತುದಿ()         ? "TOP"
                        : VN_IS(m_modp, Class) ? ("TOP." + m_modp->name())
                        : VN_IS(m_modp, ClassPackage)
                            ? ("TOP." + m_modp->name())
                            : (nodep->aboveScopep()->name() + "." + nodep->aboveCellp()->name()));
            nodep->edit_ಎಣಿಕೆ_Inc();
            iterateChildrenConst(nodep);
        }
    }

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Name class functions

ಶೂನ್ಯ V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("name", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
