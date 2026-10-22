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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Localize.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LocalizeVisitor

ವರ್ಗ LocalizeVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()    ->  bool.  VarScope is not optimizable
    //  AstCFunc::user1()       ->  bool.  CFunc is not a leaf function
    //  AstVarScope::user2()    ->  bool.  VarScope was fully assigned in current function
    //  AstVarScope::user3p()   ->  Set of CFuncs referencing this VarScope. (via m_accessors)
    //  AstCFunc::user4p()      ->  Multimap of 'VarScope -> VarRefs that reference that VarScope'
    //                              in this function. (via m_references)
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_user3_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use m_user4_ಒಳ_Use;

    AstUser3Allocator<AstVarScope, std::unordered_ಗಣ<AstCFunc*>> m_accessors;
    AstUser4Allocator<AstCFunc, std::unordered_multimap<ಸ್ಥಿರಾಂಕ AstVarScope*, AstVarRef*>>
        m_references;

    // STATE - across all visitors
    std::ಸದಿಶ<AstVarScope*> m_varScopeps;  // List of variables to consider for localization
    VDouble0 m_statLocVars;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = ಶೂನ್ಯಸೂಚಕ;  // Current active function
    uint32_t m_ಗಂಟು_ಆಳ = 0;  // Node depth under m_cfuncp

    // METHODS
    ತಾರ್ಕಿಕ isOptimizable(AstVarScope* nodep) {
        // Don't want to malloc/free the backing store all the time
        ಆದರೆ (VN_IS(nodep->dtypep(), NBACommit_ಸರತಿ_DType)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Do not localize strings. They result in unnecessary initialization
        // and bloated code size due to destructor calls when unused.
        // TODO: Local variables should be pushed into the narrowest scope rather
        // than emitted at the top of the function. See discussion in #6969.
        ಆದರೆ (nodep->dtypep()->skipRefp()->isString()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Variables used in super constructor call can't be localized, because
        // in C++ there is no way to declare them before base class constructor call
        ಹಿಂತಿರುಗಿ ((!nodep->user1()  // Not marked as not optimizable, or ...
                                  // .. a block temp used in a single CFunc
                 || (nodep->varp()->varType() == VVarType::BLOCKTEMP
                     && m_accessors(nodep).ಗಾತ್ರ_2() == 1))
                // and under size limit
                && nodep->varp()->dtypep()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() <= v3Global.opt.localize_Max_ಗಾತ್ರ());
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ exists_Non_ಎಲೆ(ಸ್ಥಿರಾಂಕ std::unordered_ಗಣ<AstCFunc*>& funcps) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ funcp : funcps) {
            ಆದರೆ (funcp->user1()) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಶೂನ್ಯ moveVarScopes() {
        ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ nodep : m_varScopeps) {
            ಆದರೆ (!isOptimizable(nodep)) ಮುಂದುವರಿ;  // Not optimizable

            ಸ್ಥಿರಾಂಕ std::unordered_ಗಣ<AstCFunc*>& funcps = m_accessors(nodep);
            ಆದರೆ (funcps.ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;  // No referencing functions at all

            // If more than one referencing function, but not all are leaf
            // functions, then don't localize, as one of the referencing
            // functions might be calling another, which the current analysis
            // cannot cope with. This should be rare (introduced by V3Depth).
            ಆದರೆ (funcps.ಗಾತ್ರ_2() > 1 && exists_Non_ಎಲೆ(funcps)) ಮುಂದುವರಿ;

            UINFO(4, "Localizing " << nodep);
            ++m_statLocVars;

            // Yank the VarScope from it's parent and schedule them for deletion. Leave the Var
            // for now, as not all VarScopes referencing this Var might be localized.
            ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ());  // TODO nodep used past here

            // In each referencing function, create a replacement local variable
            AstVar* ಸ್ಥಿರಾಂಕ oldVarp = nodep->varp();
            ಗಾಗಿ (AstCFunc* ಸ್ಥಿರಾಂಕ funcp : funcps) {
                // Create the new local variable.
                ಸ್ಥಿರಾಂಕ string newName
                    = nodep->scopep() == funcp->scopep()
                          ? oldVarp->name()
                          : nodep->scopep()->nameDotless() + "__DOT__" + oldVarp->name();
                AstVar* ಸ್ಥಿರಾಂಕ newVarp
                    = ಹೊಸ AstVar{oldVarp->fileline(), oldVarp->varType(), newName, oldVarp};
                newVarp->funcLocal(ನಿಜ);
                newVarp->noReset(oldVarp->noReset());
                newVarp->noSubst(oldVarp->noSubst());
                funcp->ಕೂಡಿಸು_Varsp(newVarp);

                // Fix up all the references within this function
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ er = m_references(funcp).equal_range(nodep);
                ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = er.ಮೊದಲ; it != er.ಎರಡನೇ; ++it) {
                    AstVarRef* ಸ್ಥಿರಾಂಕ refp = it->ಎರಡನೇ;
                    refp->varScopep(ಶೂನ್ಯಸೂಚಕ);
                    refp->varp(newVarp);
                }
            }
        }
        m_varScopeps.clear();
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override {
        iterateChildrenConst(nodep);
        moveVarScopes();
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCAwait* nodep) override {
        m_cfuncp->user1(ನಿಜ);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        UINFO(4, "  CFUNC " << nodep);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_ಗಂಟು_ಆಳ);
        m_cfuncp = nodep;
        m_ಗಂಟು_ಆಳ = 0;
        ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCNew* nodep) override {
        m_cfuncp->user1(ನಿಜ);  // Mark caller as not a leaf function
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCCall* nodep) override {
        m_cfuncp->user1(ನಿಜ);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        // Analyze RHS first so "a = a + 1" is detected as a read before write
        iterate(nodep->rhsp());
        // For now we only consider an assignment that is directly under the function, (in
        // particular: not under an AstIf, or other kind of branch). This could be improved with
        // proper data flow analysis.
        ಆದರೆ (m_ಗಂಟು_ಆಳ == 0) {
            // Check if simple "VARREF = ..." assignment, i.e.: this assignment sets the whole
            // variable (and in particular, it is not assigned only in part).
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->lhsp(), VarRef)) {
                // Mark this VarScope as assigned in this function
                refp->varScopep()->user2(1);
            }
        }
        // Analyze LHS (in case it's not the above simple case
        iterate(nodep->lhsp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope* nodep) override {
        ಆದರೆ (!nodep->varp()->isPrimaryIO()  // Not an IO the user wants to interact with
            && !nodep->varp()->is_Sig_ಬಹಿರಂಗ()  // Not something the user wants to interact with
            && !nodep->varp()->isFuncLocal()  // Not already a function local (e.g.: argument)
            && !nodep->varp()->isClassMember()  // Statically exists in design hierarchy
            && !nodep->varp()->sensIfacep()  // Not sensitive to an interface
            && !nodep->varp()->isVirtIface()  // Not interface pointer
            && !nodep->varp()->const_Pool_ನಮೂದು()  // Not a const pool entry
            && !nodep->varp()->valuep()  // Does not have an initializer
        ) {
            UINFO(4, "Consider for localization: " << nodep);
            m_varScopeps.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        }
        // No iterate; Don't want varrefs under it (e.g.: in child dtype?)
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "AstVarRef not under function");

        AstVarScope* ಸ್ಥಿರಾಂಕ varScopep = nodep->varScopep();
        // Remember this function accesses this VarScope (we always need this as we might optimize
        // this VarScope into a local, even if it's not assigned. See 'isOptimizable')
        m_accessors(varScopep).ಸೇರಿಸು(m_cfuncp);  // emplace performs a temporary malloc
        // Remember the reference so we can fix it up later (we always need this as well)
        m_references(m_cfuncp).emplace(varScopep, nodep);

        ಆದರೆ (!varScopep->user1()) {  // Check if already marked as not optimizable
            // Note: we only check read variables, as it's ok to localize (and in fact discard)
            // any variables that are only written but never read.
            ಆದರೆ (nodep->access().isReadOrRW() && !varScopep->user2()) {
                // Variable is read, but is not known to have been assigned in this function. Mark
                // as not optimizable.
                UINFO(4, "Not optimizable (not written): " << nodep);
                varScopep->user1(1);
            }
        }
        // No iterate; Don't want varrefs under it  (e.g.: in child dtype?)
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        VL_RESTORER(m_ಗಂಟು_ಆಳ);
        ++m_ಗಂಟು_ಆಳ;
        iterateChildrenConst(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ LocalizeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LocalizeVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Vars localized", m_statLocVars);
    }
};

//######################################################################
// Localize class functions

ಶೂನ್ಯ V3Localize::localizeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LocalizeVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("localize", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
