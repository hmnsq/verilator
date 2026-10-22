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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Name.h"

#அந்தர்பாவ "V3LanguageWords.h"
#அந்தர்பாவ "V3MemberMap.h"
#அந்தர்பாவ "V3UniqueNames.h"

#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Name state, as a visitor of each AstNode

வகுப்பு NameVisitor final : பொது VNVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstCell::user1()        -> bool.  Set true if already processed
    //  AstScope::user1()       -> bool.  Set true if already processed
    //  AstVar::user1()         -> bool.  Set true if already processed
    மாறிலி VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    மாறிலி Ast_முனை_Module* m_modp = nullptr;  // Current module

    // Rename struct / union field properly
    std::vector<V3UniqueNames> m_name_அடுக்கு;  // Hierarchy-based renames

    வெறுமை renameKeywordCheck(Ast_முனை* nodep) {
        மாறிலி std::string rsvd = V3_Language_சொற்கள்::isKeyword(nodep->name());
        எனில் (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            மாறிலி string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_மொத்தம்_Inc();
        }
    }

    // METHODS
    வெறுமை rename(Ast_முனை* nodep, தருக்க addPvt) {
        எனில் (!nodep->user1()) {  // Not already done
            எனில் (addPvt) {
                மாறிலி string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_மொத்தம்_Inc();
            } இல்லையெனில் எனில் (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } இல்லையெனில் எனில் (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->isSigPublic()) {
                மாறிலி string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_மொத்தம்_Inc();
            } இல்லையெனில் {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        rename(nodep, பொய்);
        // Avoid clashing with the classes of the library model itself
        எனில் (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    வெறுமை visit(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        rename(nodep,
               ((!m_modp || !m_modp->is_உச்சி()) && !nodep->isSigPublic()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->isTemp()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstCFunc* nodep) override {
        எனில் (!nodep->user1()) {
            iterateChildrenConst(nodep);
            rename(nodep, பொய்);
        }
    }
    வெறுமை visit(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    வெறுமை visit(AstCell* nodep) override {
        எனில் (!nodep->user1()) {
            rename(nodep, (!nodep->modp()->modPublic() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    வெறுமை visit(Ast_முனை_UOr_Struct_DType* nodep) override {
        எனில் (nodep->packed()) {
            m_name_அடுக்கு.emplace_back("", பொய்);
            m_name_அடுக்கு.back().get("get");
            m_name_அடுக்கு.back().get("set");
        }
        iterateChildrenConst(nodep);
        எனில் (nodep->packed()) m_name_அடுக்கு.எடு_back();
    }
    வெறுமை visit(AstMemberDType* nodep) override {
        எனில் (!nodep->user1()) {
            எனில் (!m_name_அடுக்கு.empty()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_அடுக்கு.back().get(nodep->name()));
                nodep->user1(1);
            } இல்லையெனில் {
                rename(nodep, உண்மை);
            }
            iterateChildrenConst(nodep);
        }
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        எனில் (!nodep->user1()) {
            rename(nodep, உண்மை);
            iterateChildrenConst(nodep);
        }
    }
    வெறுமை visit(AstStructSel* nodep) override {
        எனில் (!nodep->user1()) {
            rename(nodep, உண்மை);
            iterateChildrenConst(nodep);
        }
    }
    வெறுமை visit(AstScope* nodep) override {
        எனில் (!nodep->user1SetOnce()) {
            எனில் (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            எனில் (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
            // Always recompute name (as many levels above scope may have changed)
            // Same formula as V3Scope
            nodep->name(nodep->is_உச்சி()         ? "TOP"
                        : VN_IS(m_modp, Class) ? ("TOP." + m_modp->name())
                        : VN_IS(m_modp, ClassPackage)
                            ? ("TOP." + m_modp->name())
                            : (nodep->aboveScopep()->name() + "." + nodep->aboveCellp()->name()));
            nodep->edit_மொத்தம்_Inc();
            iterateChildrenConst(nodep);
        }
    }

    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    // CONSTRUCTORS
    explicit NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = இயல்பு;
};

//######################################################################
// Name class functions

வெறுமை V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("name", 0, dump_மரம்_Either_Level() >= 6);
}
