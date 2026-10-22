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
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    ಧ್ರುವ Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module

    // Rename struct / union field properly
    std::vector<V3UniqueNames> m_name_ರಾಶಿ;  // Hierarchy-based renames

    ಶೂನ್ಯ renameKeywordCheck(Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ std::string rsvd = V3_Language_ಪದಗಳು::isKeyword(nodep->name());
        ಯದಿ (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            ಧ್ರುವ string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_ಎಣಿಕೆ_Inc();
        }
    }

    // METHODS
    ಶೂನ್ಯ rename(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ addPvt) {
        ಯದಿ (!nodep->user1()) {  // Not already done
            ಯದಿ (addPvt) {
                ಧ್ರುವ string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_ಎಣಿಕೆ_Inc();
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } ಅನ್ಯಥಾ ಯದಿ (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->isSigPublic()) {
                ಧ್ರುವ string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_ಎಣಿಕೆ_Inc();
            } ಅನ್ಯಥಾ {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        rename(nodep, ಅಸತ್ಯ);
        // Avoid clashing with the classes of the library model itself
        ಯದಿ (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        rename(nodep,
               ((!m_modp || !m_modp->is_ತುದಿ()) && !nodep->isSigPublic()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->isTemp()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        ಯದಿ (!nodep->user1()) {
            iterateChildrenConst(nodep);
            rename(nodep, ಅಸತ್ಯ);
        }
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    ಶೂನ್ಯ visit(AstCell* nodep) override {
        ಯದಿ (!nodep->user1()) {
            rename(nodep, (!nodep->modp()->modPublic() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        ಯದಿ (nodep->packed()) {
            m_name_ರಾಶಿ.emplace_back("", ಅಸತ್ಯ);
            m_name_ರಾಶಿ.back().get("get");
            m_name_ರಾಶಿ.back().get("set");
        }
        iterateChildrenConst(nodep);
        ಯದಿ (nodep->packed()) m_name_ರಾಶಿ.ತೆಗೆ_back();
    }
    ಶೂನ್ಯ visit(AstMemberDType* nodep) override {
        ಯದಿ (!nodep->user1()) {
            ಯದಿ (!m_name_ರಾಶಿ.empty()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_ರಾಶಿ.back().get(nodep->name()));
                nodep->user1(1);
            } ಅನ್ಯಥಾ {
                rename(nodep, ಸತ್ಯ);
            }
            iterateChildrenConst(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        ಯದಿ (!nodep->user1()) {
            rename(nodep, ಸತ್ಯ);
            iterateChildrenConst(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstStructSel* nodep) override {
        ಯದಿ (!nodep->user1()) {
            rename(nodep, ಸತ್ಯ);
            iterateChildrenConst(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        ಯದಿ (!nodep->user1SetOnce()) {
            ಯದಿ (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            ಯದಿ (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
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
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Name class functions

ಶೂನ್ಯ V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("name", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
