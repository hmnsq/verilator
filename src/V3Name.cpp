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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Name.h"

#ଅନ୍ତର୍ଭାଵ "V3LanguageWords.h"
#ଅନ୍ତର୍ଭାଵ "V3MemberMap.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Name state, as a visitor of each AstNode

ଶ୍ରେଣୀ NameVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstCell::user1()        -> bool.  Set true if already processed
    //  AstScope::user1()       -> bool.  Set true if already processed
    //  AstVar::user1()         -> bool.  Set true if already processed
    ଅଚଳ VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    ଅଚଳ Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module

    // Rename struct / union field properly
    std::vector<V3UniqueNames> m_name_ଗଦା;  // Hierarchy-based renames

    ଶୂନ୍ୟ renameKeywordCheck(Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ std::string rsvd = V3_Language_ଶବ୍ଦଗୁଡ଼ିକ::isKeyword(nodep->name());
        ଯଦି (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            ଅଚଳ string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_ଗଣନା_Inc();
        }
    }

    // METHODS
    ଶୂନ୍ୟ rename(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ addPvt) {
        ଯଦି (!nodep->user1()) {  // Not already done
            ଯଦି (addPvt) {
                ଅଚଳ string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_ଗଣନା_Inc();
            } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } ଅନ୍ୟଥା ଯଦି (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->isSigPublic()) {
                ଅଚଳ string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_ଗଣନା_Inc();
            } ଅନ୍ୟଥା {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        rename(nodep, ମିଥ୍ୟା);
        // Avoid clashing with the classes of the library model itself
        ଯଦି (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        rename(nodep,
               ((!m_modp || !m_modp->is_ଶୀର୍ଷ()) && !nodep->isSigPublic()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->isTemp()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        ଯଦି (!nodep->user1()) {
            iterateChildrenConst(nodep);
            rename(nodep, ମିଥ୍ୟା);
        }
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        ଯଦି (!nodep->user1()) {
            rename(nodep, (!nodep->modp()->modPublic() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_UOr_Struct_DType* nodep) override {
        ଯଦି (nodep->packed()) {
            m_name_ଗଦା.emplace_back("", ମିଥ୍ୟା);
            m_name_ଗଦା.back().get("get");
            m_name_ଗଦା.back().get("set");
        }
        iterateChildrenConst(nodep);
        ଯଦି (nodep->packed()) m_name_ଗଦା.କାଢ଼_back();
    }
    ଶୂନ୍ୟ visit(AstMemberDType* nodep) override {
        ଯଦି (!nodep->user1()) {
            ଯଦି (!m_name_ଗଦା.empty()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_ଗଦା.back().get(nodep->name()));
                nodep->user1(1);
            } ଅନ୍ୟଥା {
                rename(nodep, ସତ୍ୟ);
            }
            iterateChildrenConst(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        ଯଦି (!nodep->user1()) {
            rename(nodep, ସତ୍ୟ);
            iterateChildrenConst(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstStructSel* nodep) override {
        ଯଦି (!nodep->user1()) {
            rename(nodep, ସତ୍ୟ);
            iterateChildrenConst(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        ଯଦି (!nodep->user1SetOnce()) {
            ଯଦି (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            ଯଦି (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
            // Always recompute name (as many levels above scope may have changed)
            // Same formula as V3Scope
            nodep->name(nodep->is_ଶୀର୍ଷ()         ? "TOP"
                        : VN_IS(m_modp, Class) ? ("TOP." + m_modp->name())
                        : VN_IS(m_modp, ClassPackage)
                            ? ("TOP." + m_modp->name())
                            : (nodep->aboveScopep()->name() + "." + nodep->aboveCellp()->name()));
            nodep->edit_ଗଣନା_Inc();
            iterateChildrenConst(nodep);
        }
    }

    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Name class functions

ଶୂନ୍ୟ V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("name", 0, dump_ବୃକ୍ଷ_Either_Level() >= 6);
}
