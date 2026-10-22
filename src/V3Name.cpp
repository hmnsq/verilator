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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Name.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LanguageWords.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3MemberMap.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Name state, as a visitor of each AstNode

ਵਰਗ NameVisitor final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstCell::user1()        -> bool.  Set true if already processed
    //  AstScope::user1()       -> bool.  Set true if already processed
    //  AstVar::user1()         -> bool.  Set true if already processed
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    ਅਚਲ Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module

    // Rename struct / union field properly
    std::ਸਦਿਸ਼<V3_Unique_ਨਾਂ> m_name_ਢੇਰ;  // Hierarchy-based renames

    ਖਾਲੀ renameKeywordCheck(Ast_ਗੰਢ* nodep) {
        ਅਚਲ std::string rsvd = V3_Language_ਸ਼ਬਦ::isKeyword(nodep->name());
        ਜੇ (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            ਅਚਲ string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_ਗਿਣਤੀ_Inc();
        }
    }

    // METHODS
    ਖਾਲੀ rename(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ ਜੋੜੋ_Pvt) {
        ਜੇ (!nodep->user1()) {  // Not already done
            ਜੇ (ਜੋੜੋ_Pvt) {
                ਅਚਲ string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_ਗਿਣਤੀ_Inc();
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->is_Sig_ਜਨਤਕ()) {
                ਅਚਲ string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_ਗਿਣਤੀ_Inc();
            } ਨਹੀਂਤਾਂ {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        rename(nodep, ਝੂਠ);
        // Avoid clashing with the classes of the library model itself
        ਜੇ (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        rename(nodep,
               ((!m_modp || !m_modp->is_ਸਿਖਰ()) && !nodep->is_Sig_ਜਨਤਕ()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->is_ਆਰਜ਼ੀ()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        ਜੇ (!nodep->user1()) {
            iterateChildrenConst(nodep);
            rename(nodep, ਝੂਠ);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        ਜੇ (!nodep->user1()) {
            rename(nodep, (!nodep->modp()->mod_ਜਨਤਕ() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_UOr_Struct_DType* nodep) override {
        ਜੇ (nodep->packed()) {
            m_name_ਢੇਰ.emplace_ਪਿੱਠ("", ਝੂਠ);
            m_name_ਢੇਰ.ਪਿੱਠ().get("get");
            m_name_ਢੇਰ.ਪਿੱਠ().get("set");
        }
        iterateChildrenConst(nodep);
        ਜੇ (nodep->packed()) m_name_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberDType* nodep) override {
        ਜੇ (!nodep->user1()) {
            ਜੇ (!m_name_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_ਢੇਰ.ਪਿੱਠ().get(nodep->name()));
                nodep->user1(1);
            } ਨਹੀਂਤਾਂ {
                rename(nodep, ਸੱਚ);
            }
            iterateChildrenConst(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        ਜੇ (!nodep->user1()) {
            rename(nodep, ਸੱਚ);
            iterateChildrenConst(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstStructSel* nodep) override {
        ਜੇ (!nodep->user1()) {
            rename(nodep, ਸੱਚ);
            iterateChildrenConst(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        ਜੇ (!nodep->user1_ਸੈੱਟ_Once()) {
            ਜੇ (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            ਜੇ (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
            // Always recompute name (as many levels above scope may have changed)
            // Same formula as V3Scope
            nodep->name(nodep->is_ਸਿਖਰ()         ? "TOP"
                        : VN_IS(m_modp, Class) ? ("TOP." + m_modp->name())
                        : VN_IS(m_modp, ClassPackage)
                            ? ("TOP." + m_modp->name())
                            : (nodep->aboveScopep()->name() + "." + nodep->aboveCellp()->name()));
            nodep->edit_ਗਿਣਤੀ_Inc();
            iterateChildrenConst(nodep);
        }
    }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = ਮੂਲ;
};

//######################################################################
// Name class functions

ਖਾਲੀ V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("name", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}
