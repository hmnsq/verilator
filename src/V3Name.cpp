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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Name.h"

#شامل "V3LanguageWords.h"
#شامل "V3MemberMap.h"
#شامل "V3UniqueNames.h"

#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Name state, as a visitor of each AstNode

رده NameVisitor final : عمومی VNVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstCell::user1()        -> bool.  Set true if already processed
    //  AstScope::user1()       -> bool.  Set true if already processed
    //  AstVar::user1()         -> bool.  Set true if already processed
    ثابت VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    ثابت Ast_گره_Module* m_modp = nullptr;  // Current module

    // Rename struct / union field properly
    std::vector<V3UniqueNames> m_name_پشته;  // Hierarchy-based renames

    تهی renameKeywordCheck(Ast_گره* nodep) {
        ثابت std::string rsvd = V3_Language_واژگان::isKeyword(nodep->name());
        اگر (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            ثابت string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_شمار_Inc();
        }
    }

    // METHODS
    تهی rename(Ast_گره* nodep, منطقی addPvt) {
        اگر (!nodep->user1()) {  // Not already done
            اگر (addPvt) {
                ثابت string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_شمار_Inc();
            } وگرنه اگر (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } وگرنه اگر (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->isSigPublic()) {
                ثابت string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_شمار_Inc();
            } وگرنه {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        rename(nodep, نادرست);
        // Avoid clashing with the classes of the library model itself
        اگر (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    تهی visit(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        rename(nodep,
               ((!m_modp || !m_modp->is_بالاترین()) && !nodep->isSigPublic()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->isTemp()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    تهی visit(AstCFunc* nodep) override {
        اگر (!nodep->user1()) {
            iterateChildrenConst(nodep);
            rename(nodep, نادرست);
        }
    }
    تهی visit(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    تهی visit(AstCell* nodep) override {
        اگر (!nodep->user1()) {
            rename(nodep, (!nodep->modp()->modPublic() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    تهی visit(Ast_گره_UOr_Struct_DType* nodep) override {
        اگر (nodep->packed()) {
            m_name_پشته.emplace_back("", نادرست);
            m_name_پشته.back().get("get");
            m_name_پشته.back().get("set");
        }
        iterateChildrenConst(nodep);
        اگر (nodep->packed()) m_name_پشته.برداشت_back();
    }
    تهی visit(AstMemberDType* nodep) override {
        اگر (!nodep->user1()) {
            اگر (!m_name_پشته.empty()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_پشته.back().get(nodep->name()));
                nodep->user1(1);
            } وگرنه {
                rename(nodep, درست);
            }
            iterateChildrenConst(nodep);
        }
    }
    تهی visit(AstMemberSel* nodep) override {
        اگر (!nodep->user1()) {
            rename(nodep, درست);
            iterateChildrenConst(nodep);
        }
    }
    تهی visit(AstStructSel* nodep) override {
        اگر (!nodep->user1()) {
            rename(nodep, درست);
            iterateChildrenConst(nodep);
        }
    }
    تهی visit(AstScope* nodep) override {
        اگر (!nodep->user1SetOnce()) {
            اگر (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            اگر (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
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
    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = پیشفرض;
};

//######################################################################
// Name class functions

تهی V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("name", 0, dump_درخت_Either_Level() >= 6);
}
