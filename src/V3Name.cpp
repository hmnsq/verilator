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
    ثابت VNUser1_اندريون_Use m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    ثابت Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module

    // Rename struct / union field properly
    std::ويڪٽر<V3_Unique_نالا> m_name_ڍير;  // Hierarchy-based renames

    فراغ renameKeywordCheck(Ast_ڳنڍ* nodep) {
        ثابت std::string rsvd = V3_Language_لفظ::isKeyword(nodep->name());
        إذا (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            ثابت string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_ڳڻپ_Inc();
        }
    }

    // METHODS
    فراغ rename(Ast_ڳنڍ* nodep, منطقي جوڙيو_Pvt) {
        إذا (!nodep->user1()) {  // Not already done
            إذا (جوڙيو_Pvt) {
                ثابت string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_ڳڻپ_Inc();
            } وإلا إذا (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } وإلا إذا (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->is_Sig_عوامي()) {
                ثابت string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_ڳڻپ_Inc();
            } وإلا {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        rename(nodep, خطأ);
        // Avoid clashing with the classes of the library model itself
        إذا (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    فراغ گهمو(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        rename(nodep,
               ((!m_modp || !m_modp->is_چوٽي()) && !nodep->is_Sig_عوامي()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->is_عارضي()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstCFunc* nodep) override {
        إذا (!nodep->user1()) {
            iterateChildrenConst(nodep);
            rename(nodep, خطأ);
        }
    }
    فراغ گهمو(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    فراغ گهمو(AstCell* nodep) override {
        إذا (!nodep->user1()) {
            rename(nodep, (!nodep->modp()->mod_عوامي() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_UOr_Struct_DType* nodep) override {
        إذا (nodep->packed()) {
            m_name_ڍير.emplace_پٺ("", خطأ);
            m_name_ڍير.پٺ().get("get");
            m_name_ڍير.پٺ().get("set");
        }
        iterateChildrenConst(nodep);
        إذا (nodep->packed()) m_name_ڍير.ڪڍو_پٺ();
    }
    فراغ گهمو(AstMemberDType* nodep) override {
        إذا (!nodep->user1()) {
            إذا (!m_name_ڍير.خالي_جايون()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_ڍير.پٺ().get(nodep->name()));
                nodep->user1(1);
            } وإلا {
                rename(nodep, سچ);
            }
            iterateChildrenConst(nodep);
        }
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        إذا (!nodep->user1()) {
            rename(nodep, سچ);
            iterateChildrenConst(nodep);
        }
    }
    فراغ گهمو(AstStructSel* nodep) override {
        إذا (!nodep->user1()) {
            rename(nodep, سچ);
            iterateChildrenConst(nodep);
        }
    }
    فراغ گهمو(AstScope* nodep) override {
        إذا (!nodep->user1_سيٽ_Once()) {
            إذا (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            إذا (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
            // Always recompute name (as many levels above scope may have changed)
            // Same formula as V3Scope
            nodep->name(nodep->is_چوٽي()         ? "TOP"
                        : VN_IS(m_modp, Class) ? ("TOP." + m_modp->name())
                        : VN_IS(m_modp, ClassPackage)
                            ? ("TOP." + m_modp->name())
                            : (nodep->aboveScopep()->name() + "." + nodep->aboveCellp()->name()));
            nodep->edit_ڳڻپ_Inc();
            iterateChildrenConst(nodep);
        }
    }

    //--------------------
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    واضح NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = افتراضي;
};

//######################################################################
// Name class functions

فراغ V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("name", 0, dump_وڻ_Either_Level() >= 6);
}
