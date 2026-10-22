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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Name.h"

#અન્તર્ભાવ "V3LanguageWords.h"
#અન્તર્ભાવ "V3MemberMap.h"
#અન્તર્ભાવ "V3UniqueNames.h"

#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Name state, as a visitor of each AstNode

વર્ગ NameVisitor final : જાહેર VNVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstCell::user1()        -> bool.  Set true if already processed
    //  AstScope::user1()       -> bool.  Set true if already processed
    //  AstVar::user1()         -> bool.  Set true if already processed
    અચલ VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    અચલ Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module

    // Rename struct / union field properly
    std::vector<V3UniqueNames> m_name_ઢગલો;  // Hierarchy-based renames

    શૂન્ય renameKeywordCheck(Ast_ગાંઠ* nodep) {
        અચલ std::string rsvd = V3_Language_શબ્દો::isKeyword(nodep->name());
        જો (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            અચલ string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_ગણતરી_Inc();
        }
    }

    // METHODS
    શૂન્ય rename(Ast_ગાંઠ* nodep, તાર્કિક addPvt) {
        જો (!nodep->user1()) {  // Not already done
            જો (addPvt) {
                અચલ string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_ગણતરી_Inc();
            } નહીંતર જો (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } નહીંતર જો (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->isSigPublic()) {
                અચલ string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_ગણતરી_Inc();
            } નહીંતર {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        rename(nodep, ખોટું);
        // Avoid clashing with the classes of the library model itself
        જો (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    શૂન્ય visit(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        rename(nodep,
               ((!m_modp || !m_modp->is_ટોચ()) && !nodep->isSigPublic()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->isTemp()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        જો (!nodep->user1()) {
            iterateChildrenConst(nodep);
            rename(nodep, ખોટું);
        }
    }
    શૂન્ય visit(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    શૂન્ય visit(AstCell* nodep) override {
        જો (!nodep->user1()) {
            rename(nodep, (!nodep->modp()->modPublic() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_UOr_Struct_DType* nodep) override {
        જો (nodep->packed()) {
            m_name_ઢગલો.emplace_back("", ખોટું);
            m_name_ઢગલો.back().get("get");
            m_name_ઢગલો.back().get("set");
        }
        iterateChildrenConst(nodep);
        જો (nodep->packed()) m_name_ઢગલો.કાઢો_back();
    }
    શૂન્ય visit(AstMemberDType* nodep) override {
        જો (!nodep->user1()) {
            જો (!m_name_ઢગલો.empty()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_ઢગલો.back().get(nodep->name()));
                nodep->user1(1);
            } નહીંતર {
                rename(nodep, સાચું);
            }
            iterateChildrenConst(nodep);
        }
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        જો (!nodep->user1()) {
            rename(nodep, સાચું);
            iterateChildrenConst(nodep);
        }
    }
    શૂન્ય visit(AstStructSel* nodep) override {
        જો (!nodep->user1()) {
            rename(nodep, સાચું);
            iterateChildrenConst(nodep);
        }
    }
    શૂન્ય visit(AstScope* nodep) override {
        જો (!nodep->user1SetOnce()) {
            જો (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            જો (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
            // Always recompute name (as many levels above scope may have changed)
            // Same formula as V3Scope
            nodep->name(nodep->is_ટોચ()         ? "TOP"
                        : VN_IS(m_modp, Class) ? ("TOP." + m_modp->name())
                        : VN_IS(m_modp, ClassPackage)
                            ? ("TOP." + m_modp->name())
                            : (nodep->aboveScopep()->name() + "." + nodep->aboveCellp()->name()));
            nodep->edit_ગણતરી_Inc();
            iterateChildrenConst(nodep);
        }
    }

    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = મૂળભૂત;
};

//######################################################################
// Name class functions

શૂન્ય V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("name", 0, dump_વૃક્ષ_Either_Level() >= 6);
}
