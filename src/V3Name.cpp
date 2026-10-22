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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Name.h"

#అన్తర్భావ "V3LanguageWords.h"
#అన్తర్భావ "V3MemberMap.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Name state, as a visitor of each AstNode

తరగతి NameVisitor final : బహిరంగ VNVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstCell::user1()        -> bool.  Set true if already processed
    //  AstScope::user1()       -> bool.  Set true if already processed
    //  AstVar::user1()         -> bool.  Set true if already processed
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    స్థిరరాశి Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module

    // Rename struct / union field properly
    std::సదిశ<V3_Unique_పేర్లు> m_name_దొంతర;  // Hierarchy-based renames

    శూన్య renameKeywordCheck(Ast_కణుపు* nodep) {
        స్థిరరాశి std::string rsvd = V3_Language_పదాలు::isKeyword(nodep->name());
        ఒకవేళ (rsvd != "") {
            nodep->v3warn(SYMRSVDWORD, "Symbol matches " + rsvd + ": " << nodep->prettyNameQ());
            స్థిరరాశి string newname = "__SYM__"s + nodep->name();
            nodep->name(newname);
            nodep->edit_లెక్క_Inc();
        }
    }

    // METHODS
    శూన్య rename(Ast_కణుపు* nodep, తార్కిక కూడు_Pvt) {
        ఒకవేళ (!nodep->user1()) {  // Not already done
            ఒకవేళ (కూడు_Pvt) {
                స్థిరరాశి string newname = "__PVT__"s + nodep->name();
                nodep->name(newname);
                nodep->edit_లెక్క_Inc();
            } లేకపోతే ఒకవేళ (VN_IS(nodep, CFunc) && VN_AS(nodep, CFunc)->isConstructor()) {
            } లేకపోతే ఒకవేళ (v3Global.opt.emitAccessors() && VN_IS(nodep, Var)
                       && VN_AS(nodep, Var)->is_Sig_ప్రజా()) {
                స్థిరరాశి string newname = "__Vm_sig_" + nodep->name();
                nodep->name(newname);
                nodep->edit_లెక్క_Inc();
            } లేకపోతే {
                renameKeywordCheck(nodep);
            }
            nodep->user1(1);
        }
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
        rename(nodep, అబద్ధం);
        // Avoid clashing with the classes of the library model itself
        ఒకవేళ (nodep->verilatorLib()) nodep->name(nodep->name() + "__Vlibstub");
    }
    // Add __PVT__ to names of local signals
    శూన్య సందర్శించు(AstVar* nodep) override {
        // Don't iterate... Don't need temps for RANGES under the Var.
        rename(nodep,
               ((!m_modp || !m_modp->is_శిఖరం()) && !nodep->is_Sig_ప్రజా()
                && !nodep->isFuncLocal()  // Isn't exposed, and would mess up dpi import wrappers
                && !nodep->is_తాత్కాలిక()  // Don't bother to rename internal signals
                // Special case, hardcoded m_process references in verilated_std.h and elsewhere
                && !(m_modp && m_modp->name() == "std__03a__03aprocess"
                     && nodep->name() == "m_process")));
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        ఒకవేళ (!nodep->user1()) {
            iterateChildrenConst(nodep);
            rename(nodep, అబద్ధం);
        }
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override { iterateConst(nodep->varp()); }
    శూన్య సందర్శించు(AstCell* nodep) override {
        ఒకవేళ (!nodep->user1()) {
            rename(nodep, (!nodep->modp()->mod_ప్రజా() && !VN_IS(nodep->modp(), ClassPackage)));
            iterateChildrenConst(nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_UOr_Struct_DType* nodep) override {
        ఒకవేళ (nodep->packed()) {
            m_name_దొంతర.emplace_వెనుక("", అబద్ధం);
            m_name_దొంతర.వెనుక().get("get");
            m_name_దొంతర.వెనుక().get("set");
        }
        iterateChildrenConst(nodep);
        ఒకవేళ (nodep->packed()) m_name_దొంతర.తీయి_వెనుక();
    }
    శూన్య సందర్శించు(AstMemberDType* nodep) override {
        ఒకవేళ (!nodep->user1()) {
            ఒకవేళ (!m_name_దొంతర.ఖాళీలు()) {  // Packed member field
                renameKeywordCheck(nodep);
                nodep->name(m_name_దొంతర.వెనుక().get(nodep->name()));
                nodep->user1(1);
            } లేకపోతే {
                rename(nodep, నిజం);
            }
            iterateChildrenConst(nodep);
        }
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        ఒకవేళ (!nodep->user1()) {
            rename(nodep, నిజం);
            iterateChildrenConst(nodep);
        }
    }
    శూన్య సందర్శించు(AstStructSel* nodep) override {
        ఒకవేళ (!nodep->user1()) {
            rename(nodep, నిజం);
            iterateChildrenConst(nodep);
        }
    }
    శూన్య సందర్శించు(AstScope* nodep) override {
        ఒకవేళ (!nodep->user1_సమితి_Once()) {
            ఒకవేళ (nodep->aboveScopep()) iterateConst(nodep->aboveScopep());
            ఒకవేళ (nodep->aboveCellp()) iterateConst(nodep->aboveCellp());
            // Always recompute name (as many levels above scope may have changed)
            // Same formula as V3Scope
            nodep->name(nodep->is_శిఖరం()         ? "TOP"
                        : VN_IS(m_modp, Class) ? ("TOP." + m_modp->name())
                        : VN_IS(m_modp, ClassPackage)
                            ? ("TOP." + m_modp->name())
                            : (nodep->aboveScopep()->name() + "." + nodep->aboveCellp()->name()));
            nodep->edit_లెక్క_Inc();
            iterateChildrenConst(nodep);
        }
    }

    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన NameVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~NameVisitor() override = అప్రమేయ;
};

//######################################################################
// Name class functions

శూన్య V3Name::nameAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { NameVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("name", 0, dump_వృక్షం_Either_Level() >= 6);
}
