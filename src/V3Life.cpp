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
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // STATE
ಸಾರ್ವಜನಿಕ:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = ಸ್ವಭಾವ;
    ~LifeState() {
        V3Stats::addStatSum("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::addStatSum("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

ವರ್ಗ LifeVarEntry final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_ಗಂಟು_Stmt* m_assignp = nullptr;  // Last simple assignment
    AstConst* m_constp = nullptr;  // Known constant value
    ತಾರ್ಕಿಕ m_isNew = ಸತ್ಯ;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    ತಾರ್ಕಿಕ m_setBeforeUse = ಅಸತ್ಯ;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    ತಾರ್ಕಿಕ m_everSet = ಅಸತ್ಯ;

ಸಾರ್ವಜನಿಕ:
    LifeVarEntry() = ಸ್ವಭಾವ;
    ~LifeVarEntry() = ಸ್ವಭಾವ;

    ಶೂನ್ಯ init(ತಾರ್ಕಿಕ setBeforeUse) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = ಅಸತ್ಯ;
        m_setBeforeUse = setBeforeUse;
    }
    string ascii() ಧ್ರುವ {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        ಯದಿ (m_isNew) os << " isNew";
        ಯದಿ (m_setBeforeUse) os << " setBeforeUse";
        ಯದಿ (m_everSet) os << " everSet";
        ಯದಿ (m_assignp) os << " assignp=" << Ast_ಗಂಟು::ಗಂಟು_Addr(m_assignp);
        ಯದಿ (m_constp) os << " constp=" << Ast_ಗಂಟು::ಗಂಟು_Addr(m_constp);
        os << "]";
        ಪ್ರತ್ಯಾಗಮ os.str();
    }  // LCOV_EXCL_STOP

    ಶೂನ್ಯ simpleAssign(Ast_ಗಂಟು_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = nullptr;
        m_everSet = ಸತ್ಯ;
        ಯದಿ (Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = VN_CAST(nodep, Assign)) {
            ಯದಿ (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    ಶೂನ್ಯ complexAssign() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
        m_constp = nullptr;
        m_everSet = ಸತ್ಯ;
    }
    ಶೂನ್ಯ consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
    }
    Ast_ಗಂಟು_Stmt* assignp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_assignp; }
    AstConst* constNodep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_constp; }
    ತಾರ್ಕಿಕ isNew() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isNew; }
    ತಾರ್ಕಿಕ setBeforeUse() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_setBeforeUse; }
    ತಾರ್ಕಿಕ everSet() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_everSet; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

ವರ್ಗ LifeBlock final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, LifeVarEntry> m_map;
    LifeBlock* ಧ್ರುವ m_aboveLifep;  // Upper life, or nullptr
    LifeState* ಧ್ರುವ m_statep;  // Current global state
    ತಾರ್ಕಿಕ m_noopt = ಅಸತ್ಯ;  // Opaque operation seen; invalidates the block above at exit
    ತಾರ್ಕಿಕ m_replacedVref = ಅಸತ್ಯ;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

ಸಾರ್ವಜನಿಕ:
    LifeBlock(LifeBlock* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~LifeBlock() = ಸ್ವಭಾವ;
    // METHODS
    ಶೂನ್ಯ checkRemoveAssign(ಧ್ರುವ AstVarScope* vscp, LifeVarEntry& entr) {
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = vscp->varp();
        // We don't optimize any public sigs
        ಯದಿ (varp->isSigPublic()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (varp->isReadByDpi()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (varp->sensIfacep()) ಪ್ರತ್ಯಾಗಮ;
        // Check the var entry, and remove if appropriate
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ oldassp = entr.assignp();
        ಯದಿ (!oldassp) ಪ್ರತ್ಯಾಗಮ;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complexAssign();
        oldassp->unlinkFrBack();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.ತಳ್ಳು_Deletep(oldassp), oldassp);
    }
    ಶೂನ್ಯ simpleAssign(AstVarScope* nodep, Ast_ಗಂಟು_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        LifeVarEntry& entr = m_map[nodep];
        ಯದಿ (entr.isNew()) {
            entr.init(ಸತ್ಯ);
        } ಅನ್ಯಥಾ {
            checkRemoveAssign(nodep, entr);
        }
        entr.simpleAssign(assp);
        // lifeDump();
    }
    ಶೂನ್ಯ complexAssign(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        ಯದಿ (entr.isNew()) entr.init(ಅಸತ್ಯ);
        entr.complexAssign();
    }
    ಶೂನ್ಯ clearReplaced() { m_replacedVref = ಅಸತ್ಯ; }
    ತಾರ್ಕಿಕ replaced() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_replacedVref; }
    ಶೂನ್ಯ varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        LifeVarEntry& entr = m_map[nodep];
        ಯದಿ (entr.isNew()) {
            entr.init(ಅಸತ್ಯ);
        } ಅನ್ಯಥಾ {
            ಯದಿ (AstConst* ಧ್ರುವ constp = entr.constNodep()) {
                ಯದಿ (!varrefp->varp()->isSigPublic() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                    m_replacedVref = ಸತ್ಯ;
                    VL_DO_DANGLING(varrefp->delete_ವೃಕ್ಷ(), varrefp);
                    ++m_statep->m_statAssnCon;
                    ಪ್ರತ್ಯಾಗಮ;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    ಶೂನ್ಯ complexAssignFind(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        ಯದಿ (entr.isNew()) entr.init(ಅಸತ್ಯ);
        entr.complexAssign();
    }
    ಶೂನ್ಯ consumedFind(AstVarScope* nodep) {
        LifeVarEntry& entr = m_map[nodep];
        ಯದಿ (entr.isNew()) entr.init(ಅಸತ್ಯ);
        entr.consumed();
    }
    ಶೂನ್ಯ lifeToAbove() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        ಯದಿ (m_noopt) m_aboveLifep->noopt();
        ಪ್ರತಿ (auto& itr : m_map) {
            AstVarScope* ಧ್ರುವ nodep = itr.first;
            m_aboveLifep->complexAssignFind(nodep);
            ಯದಿ (itr.second.everSet()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complexAssignFind(nodep);
            } ಅನ್ಯಥಾ {
                // Record consumption, so we don't eliminate earlier assignments
                consumedFind(nodep);
            }
        }
    }
    ಶೂನ್ಯ dualBranch(LifeBlock* life1p, LifeBlock* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();  // user1p() used on entire tree
        ಪ್ರತಿ (auto& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            ಯದಿ (itr.second.setBeforeUse()) itr.first->user1(1);
        }
        ಪ್ರತಿ (auto& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* ಧ್ರುವ nodep = itr.first;
            ಯದಿ (itr.second.setBeforeUse() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                ಧ್ರುವ auto itab = m_map.find(nodep);
                ಯದಿ (itab != m_map.end()) checkRemoveAssign(nodep, itab->second);
            }
        }
        // this->lifeDump();
    }
    ಶೂನ್ಯ noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = ಸತ್ಯ;
    }
    // DEBUG
    ಶೂನ್ಯ lifeDump() {
        UINFO(5, "  LifeMap:");
        ಪ್ರತಿ (ಧ್ರುವ auto& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.second.setBeforeUse() ? "[F]  " : "     ") << itr.first);
            ಯದಿ (itr.second.assignp()) {  //
                UINFO(5, "       Ass: " << itr.second.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

ವರ್ಗ LifeVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    LifeState* ಧ್ರುವ m_statep;  // Current state
    ತಾರ್ಕಿಕ m_containsTiming = ಅಸತ್ಯ;  // Statement contains timing control
    ತಾರ್ಕಿಕ m_sideEffect = ಅಸತ್ಯ;  // Side effects discovered in assign RHS
    ತಾರ್ಕಿಕ m_noopt = ಅಸತ್ಯ;  // Disable optimization of variables in this block
    ತಾರ್ಕಿಕ m_tracingCall = ಅಸತ್ಯ;  // Iterating into a CCall to a CFunc
    LifeBlock* m_lifep = nullptr;  // Current active lifetime map for current scope

    // METHODS
    ಶೂನ್ಯ setNoopt(ಧ್ರುವ ವರ್ಣ* reasonp) {
        (ಶೂನ್ಯ)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = ಸತ್ಯ;
        m_lifep->noopt();
    }

    ಶೂನ್ಯ processAssignment(Ast_ಗಂಟು_Stmt* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = ಅಸತ್ಯ;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        ಯದಿ (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        ಯದಿ (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* ಧ್ರುವ vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simpleAssign(vscp, nodep);
        } ಅನ್ಯಥಾ {
            iterateAndNextNull(lhsp);
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* ಧ್ರುವ vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        ಯದಿ (nodep->access().isWriteOrRW()) {
            m_sideEffect = ಸತ್ಯ;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complexAssign(vscp);
        } ಅನ್ಯಥಾ {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        ಯದಿ (nodep->isTimingControl() || VN_IS(nodep, AssignForce)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            setNoopt("timing|force");
            ಯದಿ (nodep->isTimingControl()) m_containsTiming = ಸತ್ಯ;
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ಶೂನ್ಯ visit(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    ಶೂನ್ಯ visit(AstAssignDly* nodep) override {
        // V3Life doesn't understand time sense
        ಯದಿ (nodep->isTimingControl()) {
            // Don't optimize
            setNoopt("assigndly");
            m_containsTiming = ಸತ್ಯ;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterateAndNextNull(nodep->condp());
        LifeBlock* ಧ್ರುವ ifLifep = ನೂತನ LifeBlock{m_lifep, m_statep};
        LifeBlock* ಧ್ರುವ elseLifep = ನೂತನ LifeBlock{m_lifep, m_statep};
        {
            VL_RESTORER(m_lifep);
            m_lifep = ifLifep;
            iterateAndNextNull(nodep->thensp());
        }
        {
            VL_RESTORER(m_lifep);
            m_lifep = elseLifep;
            iterateAndNextNull(nodep->elsesp());
        }
        UINFO(4, "   join " << nodep);
        // Find sets on both flows
        m_lifep->dualBranch(ifLifep, elseLifep);
        // For the next assignments, clear any variables that were read or written in the block
        ifLifep->lifeToAbove();
        elseLifep->lifeToAbove();
        VL_DO_DANGLING(delete ifLifep, ifLifep);
        VL_DO_DANGLING(delete elseLifep, elseLifep);
        UINFO(4, "   if-done " << nodep);
    }
    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = ನೂತನ LifeBlock{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = ಸತ್ಯ;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        ಯದಿ (m_containsTiming) setNoopt("timing");
    }
    ಶೂನ್ಯ visit(AstJumpBlock* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = ನೂತನ LifeBlock{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = ಸತ್ಯ;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        ಯದಿ (m_containsTiming) setNoopt("timing");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        ಯದಿ (nodep->funcp()->entryPoint()) {
            setNoopt("ccall");
        } ಅನ್ಯಥಾ {
            m_tracingCall = ಸತ್ಯ;
            iterate(nodep->funcp());
        }
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        ಯದಿ (!m_tracingCall && !nodep->entryPoint()) ಪ್ರತ್ಯಾಗಮ;
        m_tracingCall = ಅಸತ್ಯ;
        ಯದಿ (nodep->recursive()) setNoopt("recursive");
        ಯದಿ (nodep->noLife()) setNoopt("nolife");
        ಯದಿ (nodep->dpiImportPrototype() && !nodep->dpiPure()) {
            m_sideEffect = ಸತ್ಯ;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCExprUser* nodep) override {
        m_sideEffect = ಸತ್ಯ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCExpr* nodep) override {
        m_sideEffect = ಸತ್ಯ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstVar*) override {}  // Don't want varrefs under it
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            setNoopt("timing");
            m_containsTiming = ಸತ್ಯ;
        }
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LifeVisitor(Ast_ಗಂಟು* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = ನೂತನ LifeBlock{nullptr, m_statep};
            iterate(nodep);
            ಯದಿ (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
        }
    }
    ~LifeVisitor() override {
        ಯದಿ (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

ವರ್ಗ Life_ತುದಿ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
ಗುಪ್ತ:
    // STATE
    LifeState* ಧ್ರುವ m_statep;  // Current state

    // VISITORS
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        ಯದಿ (nodep->entryPoint()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    ಶೂನ್ಯ visit(AstVar*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Life_ತುದಿ_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_ತುದಿ_Visitor() override = ಸ್ವಭಾವ;
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
