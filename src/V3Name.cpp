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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Name.h"

#समावेश "V3LanguageWords.h"
#समावेश "V3MemberMap.h"
#समावेश "V3UniqueNames.h"

#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Name state, as a visitor of each AstNode

श्रेणी NameVisitor final : खुला VNVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstCell::user1()        -> bool.  Set true if already processed
    //  AstScope::user1()       -> bool.  Set true if already processed
    //  AstVar::user1()         -> bool.  Set true if already processed
    स्थिर VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    स्थिर Ast_गाँठ_Module* m_modp = nullptr;  // Current module

    // Rename struct / union field properly
    std::vector<V3UniqueNames> m_name_ढेर;  // Hierarchy-based renames

    व्योम renameKeywordCheck(Ast_गाँठ* nodep) {
        स्थिर std::string rsvd = V3_Language_शब्दगण::isKeyword(nodep->name());
        अगर (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            स्थिर string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_गिनती_Inc();
        }
    }

    // METHODS
    व्योम नाम(Ast_गाँठ* nodep, bool addPvt) {
        अगर (!nodep->user1()) {  // Not already done
            अगर (addPvt) {
                स्थिर string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_गिनती_Inc();
            } अन्यथा अगर (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } अन्यथा अगर (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->isSigPublic()) {
                स्थिर string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_गिनती_Inc();
            } अन्यथा {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        नाम(nodep, false);
        // Avoid clashing with the classes of the library model itself
        अगर (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    व्योम visit(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        नाम(nodep,
               ((!m_modp || !m_modp->is_शीर्ष()) && !nodep->isSigPublic()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->is_अस्थायी()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstCFunc* nodep) override {
        अगर (!nodep->user1()) {
            iterateChildrenConst(nodep);
            नाम(nodep, false);
        }
    }
    व्योम visit(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    व्योम visit(AstCell* nodep) override {
        अगर (!nodep->user1()) {
            नाम(nodep, (!nodep->modp()->modPublic() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    व्योम visit(Ast_गाँठ_UOr_Struct_DType* nodep) override {
        अगर (nodep->packed()) {
            m_name_ढेर.emplace_back("", false);
            m_name_ढेर.back().लो("get");
            m_name_ढेर.back().लो("set");
        }
        iterateChildrenConst(nodep);
        अगर (nodep->packed()) m_name_ढेर.निकालो_back();
    }
    व्योम visit(AstMemberDType* nodep) override {
        अगर (!nodep->user1()) {
            अगर (!m_name_ढेर.empty()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_ढेर.back().लो(nodep->name()));
                nodep->user1(1);
            } अन्यथा {
                नाम(nodep, true);
            }
            iterateChildrenConst(nodep);
        }
    }
    व्योम visit(AstMemberSel* nodep) override {
        अगर (!nodep->user1()) {
            नाम(nodep, true);
            iterateChildrenConst(nodep);
        }
    }
    व्योम visit(AstStructSel* nodep) override {
        अगर (!nodep->user1()) {
            नाम(nodep, true);
            iterateChildrenConst(nodep);
        }
    }
    व्योम visit(AstScope* nodep) override {
        अगर (!nodep->user1SetOnce()) {
            अगर (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            अगर (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
            // Always recompute name (as many levels above scope may have changed)
            // Same formula as V3Scope
            nodep->name(nodep->is_शीर्ष()         ? "TOP"
                        : VN_IS(m_modp, Class) ? ("TOP." + m_modp->name())
                        : VN_IS(m_modp, ClassPackage)
                            ? ("TOP." + m_modp->name())
                            : (nodep->aboveScopep()->name() + "." + nodep->aboveCellp()->name()));
            nodep->edit_गिनती_Inc();
            iterateChildrenConst(nodep);
        }
    }

    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    // CONSTRUCTORS
    explicit NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = शेष;
};

//######################################################################
// Name class functions

व्योम V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("name", 0, dump_वृक्ष_Either_Level() >= 6);
}
