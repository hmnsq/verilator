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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Name.h"

#ضمن "V3LanguageWords.h"
#ضمن "V3MemberMap.h"
#ضمن "V3UniqueNames.h"

#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Name state, as a visitor of each AstNode

صنف NameVisitor final : عام VNVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstCell::user1()        -> bool.  Set true if already processed
    //  AstScope::user1()       -> bool.  Set true if already processed
    //  AstVar::user1()         -> bool.  Set true if already processed
    ثابت VNUser1_درون_Use m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    ثابت Ast_گره_Module* m_modp = اشاره_تهی;  // Current module

    // Rename struct / union field properly
    std::وکتور<V3_Unique_نامها> m_name_پشته;  // Hierarchy-based renames

    فراغ renameKeywordCheck(Ast_گره* nodep) {
        ثابت std::string rsvd = V3_Language_واژگان::isKeyword(nodep->name());
        إذا (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            ثابت string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_شمار_Inc();
        }
    }

    // METHODS
    فراغ rename(Ast_گره* nodep, منطقي جمع_کن_Pvt) {
        إذا (!nodep->user1()) {  // Not already done
            إذا (جمع_کن_Pvt) {
                ثابت string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_شمار_Inc();
            } وإلا إذا (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } وإلا إذا (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->is_Sig_عمومی()) {
                ثابت string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_شمار_Inc();
            } وإلا {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        rename(nodep, خطأ);
        // Avoid clashing with the classes of the library model itself
        إذا (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    فراغ بازدید(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        rename(nodep,
               ((!m_modp || !m_modp->is_بالاترین()) && !nodep->is_Sig_عمومی()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->is_موقت()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstCFunc* nodep) override {
        إذا (!nodep->user1()) {
            iterateChildrenConst(nodep);
            rename(nodep, خطأ);
        }
    }
    فراغ بازدید(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    فراغ بازدید(AstCell* nodep) override {
        إذا (!nodep->user1()) {
            rename(nodep, (!nodep->modp()->mod_عمومی() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    فراغ بازدید(Ast_گره_UOr_Struct_DType* nodep) override {
        إذا (nodep->packed()) {
            m_name_پشته.emplace_پشت("", خطأ);
            m_name_پشته.پشت().get("get");
            m_name_پشته.پشت().get("set");
        }
        iterateChildrenConst(nodep);
        إذا (nodep->packed()) m_name_پشته.برداشت_پشت();
    }
    فراغ بازدید(AstMemberDType* nodep) override {
        إذا (!nodep->user1()) {
            إذا (!m_name_پشته.خالی()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_پشته.پشت().get(nodep->name()));
                nodep->user1(1);
            } وإلا {
                rename(nodep, درست);
            }
            iterateChildrenConst(nodep);
        }
    }
    فراغ بازدید(AstMemberSel* nodep) override {
        إذا (!nodep->user1()) {
            rename(nodep, درست);
            iterateChildrenConst(nodep);
        }
    }
    فراغ بازدید(AstStructSel* nodep) override {
        إذا (!nodep->user1()) {
            rename(nodep, درست);
            iterateChildrenConst(nodep);
        }
    }
    فراغ بازدید(AstScope* nodep) override {
        إذا (!nodep->user1_مجموعه_Once()) {
            إذا (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            إذا (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
            // Always recompute name (as many levels above scope may have changed)
            // Same formula as V3Scope
            nodep->name(nodep->is_بالاترین()         ? "TOP"
                        : VN_IS(m_modp, Class) ? ("TOP." + m_modp->name())
                        : VN_IS(m_modp, ClassPackage)
                            ? ("TOP." + m_modp->name())
                            : (nodep->aboveScopep()->name() + "." + nodep->aboveCellp()->name()));
            nodep->edit_شمار_Inc();
            iterateChildrenConst(nodep);
        }
    }

    //--------------------
    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    صریح NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = افتراضي;
};

//######################################################################
// Name class functions

فراغ V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("name", 0, dump_درخت_Either_Level() >= 6);
}
