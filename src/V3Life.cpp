// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Lifelicate variable assignment elimination
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
// LIFE TRANSFORMATIONS:
//      Build control-flow graph with assignments and var usages
//      All modules:
//          ASSIGN(x,...), ASSIGN(x,...) => delete first one
//          We also track across if statements:
//          ASSIGN(X,...) IF( ..., ASSIGN(X,...), ASSIGN(X,...)) => deletes first
//          We don't do the opposite yet though (remove assigns in if followed by outside if)
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Life.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Structure for global state

ವರ್ಗ LifeState final {
    // NODE STATE
    //   See below
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // STATE
ಸಾರ್ವಜನಿಕ:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~LifeState() {
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

ವರ್ಗ Life_Var_ನಮೂದು final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_ಗಂಟು_Stmt* m_assignp = ಶೂನ್ಯಸೂಚಕ;  // Last simple assignment
    AstConst* m_constp = ಶೂನ್ಯಸೂಚಕ;  // Known constant value
    ತಾರ್ಕಿಕ m_isNew = ನಿಜ;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    ತಾರ್ಕಿಕ m_ಗಣ_Before_Use = ಸುಳ್ಳು;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    ತಾರ್ಕಿಕ m_ever_ಗಣ = ಸುಳ್ಳು;

ಸಾರ್ವಜನಿಕ:
    Life_Var_ನಮೂದು() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~Life_Var_ನಮೂದು() = ಪೂರ್ವನಿಯೋಜಿತ;

    ಶೂನ್ಯ init(ತಾರ್ಕಿಕ ಗಣ_Before_Use) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = ಸುಳ್ಳು;
        m_ಗಣ_Before_Use = ಗಣ_Before_Use;
    }
    string ascii() ಸ್ಥಿರಾಂಕ {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        ಆದರೆ (m_isNew) os << " isNew";
        ಆದರೆ (m_ಗಣ_Before_Use) os << " setBeforeUse";
        ಆದರೆ (m_ever_ಗಣ) os << " everSet";
        ಆದರೆ (m_assignp) os << " assignp=" << Ast_ಗಂಟು::ಗಂಟು_Addr(m_assignp);
        ಆದರೆ (m_constp) os << " constp=" << Ast_ಗಂಟು::ಗಂಟು_Addr(m_constp);
        os << "]";
        ಹಿಂತಿರುಗಿ os.str();
    }  // LCOV_EXCL_STOP

    ಶೂನ್ಯ simple_ನಿಯೋಜಿಸು(Ast_ಗಂಟು_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = ಶೂನ್ಯಸೂಚಕ;
        m_ever_ಗಣ = ನಿಜ;
        ಆದರೆ (Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = VN_CAST(nodep, ನಿಯೋಜಿಸು)) {
            ಆದರೆ (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    ಶೂನ್ಯ complex_ನಿಯೋಜಿಸು() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = ಶೂನ್ಯಸೂಚಕ;
        m_constp = ಶೂನ್ಯಸೂಚಕ;
        m_ever_ಗಣ = ನಿಜ;
    }
    ಶೂನ್ಯ consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = ಶೂನ್ಯಸೂಚಕ;
    }
    Ast_ಗಂಟು_Stmt* assignp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_assignp; }
    AstConst* constNodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_constp; }
    ತಾರ್ಕಿಕ isNew() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isNew; }
    ತಾರ್ಕಿಕ ಗಣ_Before_Use() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಗಣ_Before_Use; }
    ತಾರ್ಕಿಕ ever_ಗಣ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ever_ಗಣ; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

ವರ್ಗ Life_ಖಂಡ final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, Life_Var_ನಮೂದು> m_map;
    Life_ಖಂಡ* ಸ್ಥಿರಾಂಕ m_aboveLifep;  // Upper life, or nullptr
    LifeState* ಸ್ಥಿರಾಂಕ m_statep;  // Current global state
    ತಾರ್ಕಿಕ m_noopt = ಸುಳ್ಳು;  // Opaque operation seen; invalidates the block above at exit
    ತಾರ್ಕಿಕ m_replacedVref = ಸುಳ್ಳು;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

ಸಾರ್ವಜನಿಕ:
    Life_ಖಂಡ(Life_ಖಂಡ* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~Life_ಖಂಡ() = ಪೂರ್ವನಿಯೋಜಿತ;
    // METHODS
    ಶೂನ್ಯ check_Remove_ನಿಯೋಜಿಸು(ಸ್ಥಿರಾಂಕ AstVarScope* vscp, Life_Var_ನಮೂದು& entr) {
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = vscp->varp();
        // We don't optimize any public sigs
        ಆದರೆ (varp->is_Sig_ಬಹಿರಂಗ()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (varp->isReadByDpi()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (varp->sensIfacep()) ಹಿಂತಿರುಗಿ;
        // Check the var entry, and remove if appropriate
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ oldassp = entr.assignp();
        ಆದರೆ (!oldassp) ಹಿಂತಿರುಗಿ;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complex_ನಿಯೋಜಿಸು();
        oldassp->unlink_Fr_ಹಿಂಭಾಗ();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.ತಳ್ಳು_Deletep(oldassp), oldassp);
    }
    ಶೂನ್ಯ simple_ನಿಯೋಜಿಸು(AstVarScope* nodep, Ast_ಗಂಟು_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        Life_Var_ನಮೂದು& entr = m_map[nodep];
        ಆದರೆ (entr.isNew()) {
            entr.init(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            check_Remove_ನಿಯೋಜಿಸು(nodep, entr);
        }
        entr.simple_ನಿಯೋಜಿಸು(assp);
        // lifeDump();
    }
    ಶೂನ್ಯ complex_ನಿಯೋಜಿಸು(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        Life_Var_ನಮೂದು& entr = m_map[nodep];
        ಆದರೆ (entr.isNew()) entr.init(ಸುಳ್ಳು);
        entr.complex_ನಿಯೋಜಿಸು();
    }
    ಶೂನ್ಯ clearReplaced() { m_replacedVref = ಸುಳ್ಳು; }
    ತಾರ್ಕಿಕ replaced() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_replacedVref; }
    ಶೂನ್ಯ varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        Life_Var_ನಮೂದು& entr = m_map[nodep];
        ಆದರೆ (entr.isNew()) {
            entr.init(ಸುಳ್ಳು);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ constp = entr.constNodep()) {
                ಆದರೆ (!varrefp->varp()->is_Sig_ಬಹಿರಂಗ() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                    m_replacedVref = ನಿಜ;
                    VL_DO_DANGLING(varrefp->delete_ವೃಕ್ಷ(), varrefp);
                    ++m_statep->m_statAssnCon;
                    ಹಿಂತಿರುಗಿ;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    ಶೂನ್ಯ complex_ನಿಯೋಜಿಸು_ಕಂಡುಹಿಡಿ(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        Life_Var_ನಮೂದು& entr = m_map[nodep];
        ಆದರೆ (entr.isNew()) entr.init(ಸುಳ್ಳು);
        entr.complex_ನಿಯೋಜಿಸು();
    }
    ಶೂನ್ಯ consumed_ಕಂಡುಹಿಡಿ(AstVarScope* nodep) {
        Life_Var_ನಮೂದು& entr = m_map[nodep];
        ಆದರೆ (entr.isNew()) entr.init(ಸುಳ್ಳು);
        entr.consumed();
    }
    ಶೂನ್ಯ life_ಗಮ್ಯ_Above() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        ಆದರೆ (m_noopt) m_aboveLifep->noopt();
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& itr : m_map) {
            AstVarScope* ಸ್ಥಿರಾಂಕ nodep = itr.ಮೊದಲ;
            m_aboveLifep->complex_ನಿಯೋಜಿಸು_ಕಂಡುಹಿಡಿ(nodep);
            ಆದರೆ (itr.ಎರಡನೇ.ever_ಗಣ()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complex_ನಿಯೋಜಿಸು_ಕಂಡುಹಿಡಿ(nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Record consumption, so we don't eliminate earlier assignments
                consumed_ಕಂಡುಹಿಡಿ(nodep);
            }
        }
    }
    ಶೂನ್ಯ dualBranch(Life_ಖಂಡ* life1p, Life_ಖಂಡ* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();  // user1p() used on entire tree
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            ಆದರೆ (itr.ಎರಡನೇ.ಗಣ_Before_Use()) itr.ಮೊದಲ->user1(1);
        }
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* ಸ್ಥಿರಾಂಕ nodep = itr.ಮೊದಲ;
            ಆದರೆ (itr.ಎರಡನೇ.ಗಣ_Before_Use() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ itab = m_map.ಕಂಡುಹಿಡಿ(nodep);
                ಆದರೆ (itab != m_map.ಕೊನೆ()) check_Remove_ನಿಯೋಜಿಸು(nodep, itab->ಎರಡನೇ);
            }
        }
        // this->lifeDump();
    }
    ಶೂನ್ಯ noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = ನಿಜ;
    }
    // DEBUG
    ಶೂನ್ಯ lifeDump() {
        UINFO(5, "  LifeMap:");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.ಎರಡನೇ.ಗಣ_Before_Use() ? "[F]  " : "     ") << itr.ಮೊದಲ);
            ಆದರೆ (itr.ಎರಡನೇ.assignp()) {  //
                UINFO(5, "       Ass: " << itr.ಎರಡನೇ.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

ವರ್ಗ LifeVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    LifeState* ಸ್ಥಿರಾಂಕ m_statep;  // Current state
    ತಾರ್ಕಿಕ m_ಒಳಗೊಂಡಿದೆ_Timing = ಸುಳ್ಳು;  // Statement contains timing control
    ತಾರ್ಕಿಕ m_sideEffect = ಸುಳ್ಳು;  // Side effects discovered in assign RHS
    ತಾರ್ಕಿಕ m_noopt = ಸುಳ್ಳು;  // Disable optimization of variables in this block
    ತಾರ್ಕಿಕ m_tracingCall = ಸುಳ್ಳು;  // Iterating into a CCall to a CFunc
    Life_ಖಂಡ* m_lifep = ಶೂನ್ಯಸೂಚಕ;  // Current active lifetime map for current scope

    // METHODS
    ಶೂನ್ಯ ಗಣ_Noopt(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* reasonp) {
        (ಶೂನ್ಯ)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = ನಿಜ;
        m_lifep->noopt();
    }

    ಶೂನ್ಯ processAssignment(Ast_ಗಂಟು_Stmt* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = ಸುಳ್ಳು;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        ಆದರೆ (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        ಆದರೆ (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simple_ನಿಯೋಜಿಸು(vscp, nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterate_And_ಮುಂದಿನ_Null(lhsp);
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        ಆದರೆ (nodep->access().isWriteOrRW()) {
            m_sideEffect = ನಿಜ;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complex_ನಿಯೋಜಿಸು(vscp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (nodep->isTimingControl() || VN_IS(nodep, ನಿಯೋಜಿಸು_Force)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            ಗಣ_Noopt("timing|force");
            ಆದರೆ (nodep->isTimingControl()) m_ಒಳಗೊಂಡಿದೆ_Timing = ನಿಜ;
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override {
        // V3Life doesn't understand time sense
        ಆದರೆ (nodep->isTimingControl()) {
            // Don't optimize
            ಗಣ_Noopt("assigndly");
            m_ಒಳಗೊಂಡಿದೆ_Timing = ನಿಜ;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterate_And_ಮುಂದಿನ_Null(nodep->condp());
        Life_ಖಂಡ* ಸ್ಥಿರಾಂಕ ifLifep = ಹೊಸ Life_ಖಂಡ{m_lifep, m_statep};
        Life_ಖಂಡ* ಸ್ಥಿರಾಂಕ elseLifep = ಹೊಸ Life_ಖಂಡ{m_lifep, m_statep};
        {
            VL_RESTORER(m_lifep);
            m_lifep = ifLifep;
            iterate_And_ಮುಂದಿನ_Null(nodep->thensp());
        }
        {
            VL_RESTORER(m_lifep);
            m_lifep = elseLifep;
            iterate_And_ಮುಂದಿನ_Null(nodep->elsesp());
        }
        UINFO(4, "   join " << nodep);
        // Find sets on both flows
        m_lifep->dualBranch(ifLifep, elseLifep);
        // For the next assignments, clear any variables that were read or written in the block
        ifLifep->life_ಗಮ್ಯ_Above();
        elseLifep->life_ಗಮ್ಯ_Above();
        VL_DO_DANGLING(ಅಳಿಸು ifLifep, ifLifep);
        VL_DO_DANGLING(ಅಳಿಸು elseLifep, elseLifep);
        UINFO(4, "   if-done " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_ಒಳಗೊಂಡಿದೆ_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = ಹೊಸ Life_ಖಂಡ{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = ನಿಜ;
            iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_ಗಮ್ಯ_Above();
            VL_DO_DANGLING(ಅಳಿಸು m_lifep, m_lifep);
        }
        ಆದರೆ (m_ಒಳಗೊಂಡಿದೆ_Timing) ಗಣ_Noopt("timing");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Jump_ಖಂಡ* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_ಒಳಗೊಂಡಿದೆ_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = ಹೊಸ Life_ಖಂಡ{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = ನಿಜ;
            iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_ಗಮ್ಯ_Above();
            VL_DO_DANGLING(ಅಳಿಸು m_lifep, m_lifep);
        }
        ಆದರೆ (m_ಒಳಗೊಂಡಿದೆ_Timing) ಗಣ_Noopt("timing");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        ಆದರೆ (nodep->funcp()->ನಮೂದು_ಬಿಂದು()) {
            ಗಣ_Noopt("ccall");
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_tracingCall = ನಿಜ;
            iterate(nodep->funcp());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        ಆದರೆ (!m_tracingCall && !nodep->ನಮೂದು_ಬಿಂದು()) ಹಿಂತಿರುಗಿ;
        m_tracingCall = ಸುಳ್ಳು;
        ಆದರೆ (nodep->recursive()) ಗಣ_Noopt("recursive");
        ಆದರೆ (nodep->noLife()) ಗಣ_Noopt("nolife");
        ಆದರೆ (nodep->dpiImportPrototype() && !nodep->dpi_ಶುದ್ಧ()) {
            m_sideEffect = ನಿಜ;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExprUser* nodep) override {
        m_sideEffect = ನಿಜ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExpr* nodep) override {
        m_sideEffect = ನಿಜ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}  // Don't want varrefs under it
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            ಗಣ_Noopt("timing");
            m_ಒಳಗೊಂಡಿದೆ_Timing = ನಿಜ;
        }
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LifeVisitor(Ast_ಗಂಟು* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = ಹೊಸ Life_ಖಂಡ{ಶೂನ್ಯಸೂಚಕ, m_statep};
            iterate(nodep);
            ಆದರೆ (m_lifep) VL_DO_CLEAR(ಅಳಿಸು m_lifep, m_lifep = ಶೂನ್ಯಸೂಚಕ);
        }
    }
    ~LifeVisitor() override {
        ಆದರೆ (m_lifep) VL_DO_CLEAR(ಅಳಿಸು m_lifep, m_lifep = ಶೂನ್ಯಸೂಚಕ);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

ವರ್ಗ Life_ತುದಿ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
ಖಾಸಗಿ:
    // STATE
    LifeState* ಸ್ಥಿರಾಂಕ m_statep;  // Current state

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        ಆದರೆ (nodep->ನಮೂದು_ಬಿಂದು()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Life_ತುದಿ_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_ತುದಿ_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Life class functions

ಶೂನ್ಯ V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_ತುದಿ_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_ವೃಕ್ಷ();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_ವೃಕ್ಷ("life", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
