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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Name.h"

#अन्तर्भाव "V3LanguageWords.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Name state, as a visitor of each AstNode

वर्ग NameVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstCell::user1()        -> bool.  Set true if already processed
    //  AstScope::user1()       -> bool.  Set true if already processed
    //  AstVar::user1()         -> bool.  Set true if already processed
    ध्रुव VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    ध्रुव Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module

    // Rename struct / union field properly
    std::vector<V3UniqueNames> m_name_राशिः;  // Hierarchy-based renames

    शून्य renameKeywordCheck(Ast_ग्रन्थिः* nodep) {
        ध्रुव std::string rsvd = V3_Language_शब्दाः::isKeyword(nodep->name());
        जइ (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            ध्रुव string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_गणनम्_Inc();
        }
    }

    // METHODS
    शून्य rename(Ast_ग्रन्थिः* nodep, तार्किक addPvt) {
        जइ (!nodep->user1()) {  // Not already done
            जइ (addPvt) {
                ध्रुव string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_गणनम्_Inc();
            } अण्णहा जइ (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } अण्णहा जइ (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->isSigPublic()) {
                ध्रुव string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_गणनम्_Inc();
            } अण्णहा {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        rename(nodep, मिच्छ);
        // Avoid clashing with the classes of the library model itself
        जइ (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    शून्य visit(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        rename(nodep,
               ((!m_modp || !m_modp->is_शिखरम्()) && !nodep->isSigPublic()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->isTemp()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCFunc* nodep) override {
        जइ (!nodep->user1()) {
            iterateChildrenConst(nodep);
            rename(nodep, मिच्छ);
        }
    }
    शून्य visit(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    शून्य visit(AstCell* nodep) override {
        जइ (!nodep->user1()) {
            rename(nodep, (!nodep->modp()->modPublic() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_UOr_Struct_DType* nodep) override {
        जइ (nodep->packed()) {
            m_name_राशिः.emplace_back("", मिच्छ);
            m_name_राशिः.back().get("get");
            m_name_राशिः.back().get("set");
        }
        iterateChildrenConst(nodep);
        जइ (nodep->packed()) m_name_राशिः.निष्कासय_back();
    }
    शून्य visit(AstMemberDType* nodep) override {
        जइ (!nodep->user1()) {
            जइ (!m_name_राशिः.empty()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_राशिः.back().get(nodep->name()));
                nodep->user1(1);
            } अण्णहा {
                rename(nodep, सच्च);
            }
            iterateChildrenConst(nodep);
        }
    }
    शून्य visit(AstMemberSel* nodep) override {
        जइ (!nodep->user1()) {
            rename(nodep, सच्च);
            iterateChildrenConst(nodep);
        }
    }
    शून्य visit(AstStructSel* nodep) override {
        जइ (!nodep->user1()) {
            rename(nodep, सच्च);
            iterateChildrenConst(nodep);
        }
    }
    शून्य visit(AstScope* nodep) override {
        जइ (!nodep->user1SetOnce()) {
            जइ (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            जइ (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
            // Always recompute name (as many levels above scope may have changed)
            // Same formula as V3Scope
            nodep->name(nodep->is_शिखरम्()         ? "TOP"
                        : VN_IS(m_modp, Class) ? ("TOP." + m_modp->name())
                        : VN_IS(m_modp, ClassPackage)
                            ? ("TOP." + m_modp->name())
                            : (nodep->aboveScopep()->name() + "." + nodep->aboveCellp()->name()));
            nodep->edit_गणनम्_Inc();
            iterateChildrenConst(nodep);
        }
    }

    //--------------------
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = स्वभाव;
};

//######################################################################
// Name class functions

शून्य V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("name", 0, dump_वृक्षः_Either_Level() >= 6);
}
