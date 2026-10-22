// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert BLOCKTEMPs to local variables
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
// LOCALIZE TRANSFORMATIONS:
//      All modules:
//          VARSCOPE(BLOCKTEMP...
//             if only referenced in one CFUNC, make it local
//          VARSCOPE
//             if non-public, always written before used, make it local
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Localize.h"

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LocalizeVisitor

తరగతి LocalizeVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()    ->  bool.  VarScope is not optimizable
    //  AstCFunc::user1()       ->  bool.  CFunc is not a leaf function
    //  AstVarScope::user2()    ->  bool.  VarScope was fully assigned in current function
    //  AstVarScope::user3p()   ->  Set of CFuncs referencing this VarScope. (via m_accessors)
    //  AstCFunc::user4p()      ->  Multimap of 'VarScope -> VarRefs that reference that VarScope'
    //                              in this function. (via m_references)
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;
    స్థిరరాశి VNUser3_లోపల_Use m_user3_లోపల_Use;
    స్థిరరాశి VNUser4_లోపల_Use m_user4_లోపల_Use;

    AstUser3Allocator<AstVarScope, std::unordered_సమితి<AstCFunc*>> m_accessors;
    AstUser4Allocator<AstCFunc, std::unordered_multimap<స్థిరరాశి AstVarScope*, AstVarRef*>>
        m_references;

    // STATE - across all visitors
    std::సదిశ<AstVarScope*> m_varScopeps;  // List of variables to consider for localization
    VDouble0 m_statLocVars;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = శూన్యసూచిక;  // Current active function
    uint32_t m_కణుపు_లోతు = 0;  // Node depth under m_cfuncp

    // METHODS
    తార్కిక isOptimizable(AstVarScope* nodep) {
        // Don't want to malloc/free the backing store all the time
        ఒకవేళ (VN_IS(nodep->dtypep(), NBACommit_వరుస_DType)) తిరిగి అబద్ధం;
        // Do not localize strings. They result in unnecessary initialization
        // and bloated code size due to destructor calls when unused.
        // TODO: Local variables should be pushed into the narrowest scope rather
        // than emitted at the top of the function. See discussion in #6969.
        ఒకవేళ (nodep->dtypep()->skipRefp()->isString()) తిరిగి అబద్ధం;
        // Variables used in super constructor call can't be localized, because
        // in C++ there is no way to declare them before base class constructor call
        తిరిగి ((!nodep->user1()  // Not marked as not optimizable, or ...
                                  // .. a block temp used in a single CFunc
                 || (nodep->varp()->varType() == VVarType::BLOCKTEMP
                     && m_accessors(nodep).కొలత() == 1))
                // and under size limit
                && nodep->varp()->dtypep()->వెడల్పు_మొత్తం_బైట్లు() <= v3Global.opt.localize_Max_కొలత());
    }

    స్థిర తార్కిక exists_Non_ఆకు(స్థిరరాశి std::unordered_సమితి<AstCFunc*>& funcps) {
        కొరకు (స్థిరరాశి AstCFunc* స్థిరరాశి funcp : funcps) {
            ఒకవేళ (funcp->user1()) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    శూన్య moveVarScopes() {
        కొరకు (AstVarScope* స్థిరరాశి nodep : m_varScopeps) {
            ఒకవేళ (!isOptimizable(nodep)) కొనసాగు;  // Not optimizable

            స్థిరరాశి std::unordered_సమితి<AstCFunc*>& funcps = m_accessors(nodep);
            ఒకవేళ (funcps.ఖాళీలు()) కొనసాగు;  // No referencing functions at all

            // If more than one referencing function, but not all are leaf
            // functions, then don't localize, as one of the referencing
            // functions might be calling another, which the current analysis
            // cannot cope with. This should be rare (introduced by V3Depth).
            ఒకవేళ (funcps.కొలత() > 1 && exists_Non_ఆకు(funcps)) కొనసాగు;

            UINFO(4, "Localizing " << nodep);
            ++m_statLocVars;

            // Yank the VarScope from it's parent and schedule them for deletion. Leave the Var
            // for now, as not all VarScopes referencing this Var might be localized.
            నెట్టు_Deletep(nodep->unlink_Fr_వెనుక());  // TODO nodep used past here

            // In each referencing function, create a replacement local variable
            AstVar* స్థిరరాశి oldVarp = nodep->varp();
            కొరకు (AstCFunc* స్థిరరాశి funcp : funcps) {
                // Create the new local variable.
                స్థిరరాశి string newName
                    = nodep->scopep() == funcp->scopep()
                          ? oldVarp->name()
                          : nodep->scopep()->nameDotless() + "__DOT__" + oldVarp->name();
                AstVar* స్థిరరాశి newVarp
                    = కొత్త AstVar{oldVarp->fileline(), oldVarp->varType(), newName, oldVarp};
                newVarp->funcLocal(నిజం);
                newVarp->noReset(oldVarp->noReset());
                newVarp->noSubst(oldVarp->noSubst());
                funcp->కూడు_Varsp(newVarp);

                // Fix up all the references within this function
                స్థిరరాశి స్వయంచాలక er = m_references(funcp).equal_range(nodep);
                కొరకు (స్వయంచాలక it = er.మొదటి; it != er.రెండవ; ++it) {
                    AstVarRef* స్థిరరాశి refp = it->రెండవ;
                    refp->varScopep(శూన్యసూచిక);
                    refp->varp(newVarp);
                }
            }
        }
        m_varScopeps.clear();
    }

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {
        iterateChildrenConst(nodep);
        moveVarScopes();
    }

    శూన్య సందర్శించు(AstCAwait* nodep) override {
        m_cfuncp->user1(నిజం);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(AstCFunc* nodep) override {
        UINFO(4, "  CFUNC " << nodep);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_కణుపు_లోతు);
        m_cfuncp = nodep;
        m_కణుపు_లోతు = 0;
        స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(AstCNew* nodep) override {
        m_cfuncp->user1(నిజం);  // Mark caller as not a leaf function
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstCCall* nodep) override {
        m_cfuncp->user1(నిజం);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        // Analyze RHS first so "a = a + 1" is detected as a read before write
        iterate(nodep->rhsp());
        // For now we only consider an assignment that is directly under the function, (in
        // particular: not under an AstIf, or other kind of branch). This could be improved with
        // proper data flow analysis.
        ఒకవేళ (m_కణుపు_లోతు == 0) {
            // Check if simple "VARREF = ..." assignment, i.e.: this assignment sets the whole
            // variable (and in particular, it is not assigned only in part).
            ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(nodep->lhsp(), VarRef)) {
                // Mark this VarScope as assigned in this function
                refp->varScopep()->user2(1);
            }
        }
        // Analyze LHS (in case it's not the above simple case
        iterate(nodep->lhsp());
    }

    శూన్య సందర్శించు(AstVarScope* nodep) override {
        ఒకవేళ (!nodep->varp()->isPrimaryIO()  // Not an IO the user wants to interact with
            && !nodep->varp()->is_Sig_ప్రజా()  // Not something the user wants to interact with
            && !nodep->varp()->isFuncLocal()  // Not already a function local (e.g.: argument)
            && !nodep->varp()->isClassMember()  // Statically exists in design hierarchy
            && !nodep->varp()->sensIfacep()  // Not sensitive to an interface
            && !nodep->varp()->isVirtIface()  // Not interface pointer
            && !nodep->varp()->const_Pool_నమోదు()  // Not a const pool entry
            && !nodep->varp()->valuep()  // Does not have an initializer
        ) {
            UINFO(4, "Consider for localization: " << nodep);
            m_varScopeps.నెట్టు_వెనుక(nodep);
        }
        // No iterate; Don't want varrefs under it (e.g.: in child dtype?)
    }

    శూన్య సందర్శించు(AstVarRef* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "AstVarRef not under function");

        AstVarScope* స్థిరరాశి varScopep = nodep->varScopep();
        // Remember this function accesses this VarScope (we always need this as we might optimize
        // this VarScope into a local, even if it's not assigned. See 'isOptimizable')
        m_accessors(varScopep).చొప్పించు(m_cfuncp);  // emplace performs a temporary malloc
        // Remember the reference so we can fix it up later (we always need this as well)
        m_references(m_cfuncp).emplace(varScopep, nodep);

        ఒకవేళ (!varScopep->user1()) {  // Check if already marked as not optimizable
            // Note: we only check read variables, as it's ok to localize (and in fact discard)
            // any variables that are only written but never read.
            ఒకవేళ (nodep->access().isReadOrRW() && !varScopep->user2()) {
                // Variable is read, but is not known to have been assigned in this function. Mark
                // as not optimizable.
                UINFO(4, "Not optimizable (not written): " << nodep);
                varScopep->user1(1);
            }
        }
        // No iterate; Don't want varrefs under it  (e.g.: in child dtype?)
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        VL_RESTORER(m_కణుపు_లోతు);
        ++m_కణుపు_లోతు;
        iterateChildrenConst(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన LocalizeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LocalizeVisitor() override {
        V3Stats::కూడు_Stat("Optimizations, Vars localized", m_statLocVars);
    }
};

//######################################################################
// Localize class functions

శూన్య V3Localize::localizeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LocalizeVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("localize", 0, dump_వృక్షం_Either_Level() >= 6);
}
