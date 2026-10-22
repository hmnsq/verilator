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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Name.h"

#অন্তর্ভাব "V3LanguageWords.h"
#অন্তর্ভাব "V3MemberMap.h"
#অন্তর্ভাব "V3UniqueNames.h"

#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Name state, as a visitor of each AstNode

বর্গ NameVisitor final : সার্বজনিক VNVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstCell::user1()        -> bool.  Set true if already processed
    //  AstScope::user1()       -> bool.  Set true if already processed
    //  AstVar::user1()         -> bool.  Set true if already processed
    ধ্রুব VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    ধ্রুব Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module

    // Rename struct / union field properly
    std::vector<V3UniqueNames> m_name_স্তূপ;  // Hierarchy-based renames

    শূন্য renameKeywordCheck(Ast_গ্রন্থি* nodep) {
        ধ্রুব std::string rsvd = V3_Language_শব্দগুলি::isKeyword(nodep->name());
        যদি (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            ধ্রুব string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_গুনতি_Inc();
        }
    }

    // METHODS
    শূন্য rename(Ast_গ্রন্থি* nodep, তার্কিক addPvt) {
        যদি (!nodep->user1()) {  // Not already done
            যদি (addPvt) {
                ধ্রুব string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_গুনতি_Inc();
            } অন্যথা যদি (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } অন্যথা যদি (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->isSigPublic()) {
                ধ্রুব string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_গুনতি_Inc();
            } অন্যথা {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        rename(nodep, অসত্য);
        // Avoid clashing with the classes of the library model itself
        যদি (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    শূন্য visit(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        rename(nodep,
               ((!m_modp || !m_modp->is_শীর্ষ()) && !nodep->isSigPublic()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->isTemp()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCFunc* nodep) override {
        যদি (!nodep->user1()) {
            iterateChildrenConst(nodep);
            rename(nodep, অসত্য);
        }
    }
    শূন্য visit(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    শূন্য visit(AstCell* nodep) override {
        যদি (!nodep->user1()) {
            rename(nodep, (!nodep->modp()->modPublic() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_UOr_Struct_DType* nodep) override {
        যদি (nodep->packed()) {
            m_name_স্তূপ.emplace_back("", অসত্য);
            m_name_স্তূপ.back().get("get");
            m_name_স্তূপ.back().get("set");
        }
        iterateChildrenConst(nodep);
        যদি (nodep->packed()) m_name_স্তূপ.বের_করো_back();
    }
    শূন্য visit(AstMemberDType* nodep) override {
        যদি (!nodep->user1()) {
            যদি (!m_name_স্তূপ.empty()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_স্তূপ.back().get(nodep->name()));
                nodep->user1(1);
            } অন্যথা {
                rename(nodep, সত্য);
            }
            iterateChildrenConst(nodep);
        }
    }
    শূন্য visit(AstMemberSel* nodep) override {
        যদি (!nodep->user1()) {
            rename(nodep, সত্য);
            iterateChildrenConst(nodep);
        }
    }
    শূন্য visit(AstStructSel* nodep) override {
        যদি (!nodep->user1()) {
            rename(nodep, সত্য);
            iterateChildrenConst(nodep);
        }
    }
    শূন্য visit(AstScope* nodep) override {
        যদি (!nodep->user1SetOnce()) {
            যদি (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            যদি (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
            // Always recompute name (as many levels above scope may have changed)
            // Same formula as V3Scope
            nodep->name(nodep->is_শীর্ষ()         ? "TOP"
                        : VN_IS(m_modp, Class) ? ("TOP." + m_modp->name())
                        : VN_IS(m_modp, ClassPackage)
                            ? ("TOP." + m_modp->name())
                            : (nodep->aboveScopep()->name() + "." + nodep->aboveCellp()->name()));
            nodep->edit_গুনতি_Inc();
            iterateChildrenConst(nodep);
        }
    }

    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = স্বভাব;
};

//######################################################################
// Name class functions

শূন্য V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("name", 0, dump_বৃক্ষ_Either_Level() >= 6);
}
