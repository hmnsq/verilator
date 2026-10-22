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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Life.h"

#ضمن "V3Const.h"
#ضمن "V3Stats.h"

#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Structure for global state

صنف LifeState final {
    // NODE STATE
    //   See below
    ثابت VNUser1InUse m_inuser1;

    // STATE
عام:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = افتراضي;
    ~LifeState() {
        V3Stats::addStatSum("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::addStatSum("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

صنف LifeVarEntry final {
    // Last assignment to this varscope, nullptr if no longer relevant
    AstNodeStmt* m_assignp = nullptr;  // Last simple assignment
    AstConst* m_constp = nullptr;  // Known constant value
    منطقي m_isNew = ܫܪܪܐ;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    منطقي m_setBeforeUse = ܕܓܠܐ;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    منطقي m_everSet = ܕܓܠܐ;

عام:
    LifeVarEntry() = افتراضي;
    ~LifeVarEntry() = افتراضي;

    فراغ init(منطقي setBeforeUse) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = ܕܓܠܐ;
        m_setBeforeUse = setBeforeUse;
    }
    string ascii() ثابت {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        ܐܢ (m_isNew) os << " isNew";
        ܐܢ (m_setBeforeUse) os << " setBeforeUse";
        ܐܢ (m_everSet) os << " everSet";
        ܐܢ (m_assignp) os << " assignp=" << AstNode::nodeAddr(m_assignp);
        ܐܢ (m_constp) os << " constp=" << AstNode::nodeAddr(m_constp);
        os << "]";
        ܦܢܐ os.str();
    }  // LCOV_EXCL_STOP

    فراغ simpleAssign(AstNodeStmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = nullptr;
        m_everSet = ܫܪܪܐ;
        ܐܢ (AstNodeAssign* ثابت assp = VN_CAST(nodep, Assign)) {
            ܐܢ (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    فراغ complexAssign() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
        m_constp = nullptr;
        m_everSet = ܫܪܪܐ;
    }
    فراغ consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
    }
    AstNodeStmt* assignp() ثابت { ܦܢܐ m_assignp; }
    AstConst* constNodep() ثابت { ܦܢܐ m_constp; }
    منطقي isNew() ثابت { ܦܢܐ m_isNew; }
    منطقي setBeforeUse() ثابت { ܦܢܐ m_setBeforeUse; }
    منطقي everSet() ثابت { ܦܢܐ m_everSet; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

صنف LifeBlock final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, LifeVarEntry> m_map;
    LifeBlock* ثابت m_aboveLifep;  // Upper life, or nullptr
    LifeState* ثابت m_statep;  // Current global state
    منطقي m_noopt = ܕܓܠܐ;  // Opaque operation seen; invalidates the block above at exit
    منطقي m_replacedVref = ܕܓܠܐ;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

عام:
    LifeBlock(LifeBlock* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~LifeBlock() = افتراضي;
    // METHODS
    فراغ checkRemoveAssign(ثابت AstVarScope* vscp, LifeVarEntry& entr) {
        ثابت AstVar* ثابت varp = vscp->varp();
        // We don't optimize any public sigs
        ܐܢ (varp->isSigPublic()) ܦܢܐ;
        ܐܢ (varp->isReadByDpi()) ܦܢܐ;
        ܐܢ (varp->sensIfacep()) ܦܢܐ;
        // Check the var entry, and remove if appropriate
        AstNodeStmt* ثابت oldassp = entr.assignp();
        ܐܢ (!oldassp) ܦܢܐ;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complexAssign();
        oldassp->unlinkFrBack();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.pushDeletep(oldassp), oldassp);
    }
    فراغ simpleAssign(AstVarScope* nodep, AstNodeStmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        LifeVarEntry& entr = m_map[nodep];
        ܐܢ (entr.isNew()) {
            entr.init(ܫܪܪܐ);
        } ܐܚܪܢܐ {
            checkRemoveAssign(nodep, entr);
        }
        entr.simpleAssign(assp);
        // lifeDump();
    }
    فراغ complexAssign(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        ܐܢ (entr.isNew()) entr.init(ܕܓܠܐ);
        entr.complexAssign();
    }
    فراغ clearReplaced() { m_replacedVref = ܕܓܠܐ; }
    منطقي replaced() ثابت { ܦܢܐ m_replacedVref; }
    فراغ varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        LifeVarEntry& entr = m_map[nodep];
        ܐܢ (entr.isNew()) {
            entr.init(ܕܓܠܐ);
        } ܐܚܪܢܐ {
            ܐܢ (AstConst* ثابت constp = entr.constNodep()) {
                ܐܢ (!varrefp->varp()->isSigPublic() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->cloneTree(ܕܓܠܐ));
                    m_replacedVref = ܫܪܪܐ;
                    VL_DO_DANGLING(varrefp->deleteTree(), varrefp);
                    ++m_statep->m_statAssnCon;
                    ܦܢܐ;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    فراغ complexAssignFind(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        ܐܢ (entr.isNew()) entr.init(ܕܓܠܐ);
        entr.complexAssign();
    }
    فراغ consumedFind(AstVarScope* nodep) {
        LifeVarEntry& entr = m_map[nodep];
        ܐܢ (entr.isNew()) entr.init(ܕܓܠܐ);
        entr.consumed();
    }
    فراغ lifeToAbove() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        ܐܢ (m_noopt) m_aboveLifep->noopt();
        لكل (auto& itr : m_map) {
            AstVarScope* ثابت nodep = itr.first;
            m_aboveLifep->complexAssignFind(nodep);
            ܐܢ (itr.second.everSet()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complexAssignFind(nodep);
            } ܐܚܪܢܐ {
                // Record consumption, so we don't eliminate earlier assignments
                consumedFind(nodep);
            }
        }
    }
    فراغ dualBranch(LifeBlock* life1p, LifeBlock* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        AstNode::user1ClearTree();  // user1p() used on entire tree
        لكل (auto& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            ܐܢ (itr.second.setBeforeUse()) itr.first->user1(1);
        }
        لكل (auto& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* ثابت nodep = itr.first;
            ܐܢ (itr.second.setBeforeUse() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                ثابت auto itab = m_map.find(nodep);
                ܐܢ (itab != m_map.end()) checkRemoveAssign(nodep, itab->second);
            }
        }
        // this->lifeDump();
    }
    فراغ noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = ܫܪܪܐ;
    }
    // DEBUG
    فراغ lifeDump() {
        UINFO(5, "  LifeMap:");
        لكل (ثابت auto& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.second.setBeforeUse() ? "[F]  " : "     ") << itr.first);
            ܐܢ (itr.second.assignp()) {  //
                UINFO(5, "       Ass: " << itr.second.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

صنف LifeVisitor final : عام VNVisitor {
    // STATE
    LifeState* ثابت m_statep;  // Current state
    منطقي m_containsTiming = ܕܓܠܐ;  // Statement contains timing control
    منطقي m_sideEffect = ܕܓܠܐ;  // Side effects discovered in assign RHS
    منطقي m_noopt = ܕܓܠܐ;  // Disable optimization of variables in this block
    منطقي m_tracingCall = ܕܓܠܐ;  // Iterating into a CCall to a CFunc
    LifeBlock* m_lifep = nullptr;  // Current active lifetime map for current scope

    // METHODS
    فراغ setNoopt(ثابت حرف* reasonp) {
        (فراغ)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = ܫܪܪܐ;
        m_lifep->noopt();
    }

    فراغ processAssignment(AstNodeStmt* nodep, AstNodeExpr* lhsp, AstNodeExpr* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = ܕܓܠܐ;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), NodeExpr);
        ܐܢ (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        ܐܢ (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* ثابت vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simpleAssign(vscp, nodep);
        } ܐܚܪܢܐ {
            iterateAndNextNull(lhsp);
        }
    }

    // VISITORS
    فراغ visit(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* ثابت vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        ܐܢ (nodep->access().isWriteOrRW()) {
            m_sideEffect = ܫܪܪܐ;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complexAssign(vscp);
        } ܐܚܪܢܐ {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    فراغ visit(AstNodeAssign* nodep) override {
        ܐܢ (nodep->isTimingControl() || VN_IS(nodep, AssignForce)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            setNoopt("timing|force");
            ܐܢ (nodep->isTimingControl()) m_containsTiming = ܫܪܪܐ;
            iterateChildren(nodep);
            ܦܢܐ;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    فراغ visit(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    فراغ visit(AstAssignDly* nodep) override {
        // V3Life doesn't understand time sense
        ܐܢ (nodep->isTimingControl()) {
            // Don't optimize
            setNoopt("assigndly");
            m_containsTiming = ܫܪܪܐ;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    فراغ visit(AstNodeIf* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterateAndNextNull(nodep->condp());
        LifeBlock* ثابت ifLifep = جديد LifeBlock{m_lifep, m_statep};
        LifeBlock* ثابت elseLifep = جديد LifeBlock{m_lifep, m_statep};
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
    فراغ visit(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = جديد LifeBlock{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = ܫܪܪܐ;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        ܐܢ (m_containsTiming) setNoopt("timing");
    }
    فراغ visit(AstJumpBlock* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = جديد LifeBlock{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = ܫܪܪܐ;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        ܐܢ (m_containsTiming) setNoopt("timing");
    }
    فراغ visit(AstNodeCCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        ܐܢ (nodep->funcp()->entryPoint()) {
            setNoopt("ccall");
        } ܐܚܪܢܐ {
            m_tracingCall = ܫܪܪܐ;
            iterate(nodep->funcp());
        }
    }
    فراغ visit(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        ܐܢ (!m_tracingCall && !nodep->entryPoint()) ܦܢܐ;
        m_tracingCall = ܕܓܠܐ;
        ܐܢ (nodep->recursive()) setNoopt("recursive");
        ܐܢ (nodep->noLife()) setNoopt("nolife");
        ܐܢ (nodep->dpiImportPrototype() && !nodep->dpiPure()) {
            m_sideEffect = ܫܪܪܐ;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstCExprUser* nodep) override {
        m_sideEffect = ܫܪܪܐ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    فراغ visit(AstCExpr* nodep) override {
        m_sideEffect = ܫܪܪܐ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    فراغ visit(AstVar*) override {}  // Don't want varrefs under it
    فراغ visit(AstNode* nodep) override {
        ܐܢ (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            setNoopt("timing");
            m_containsTiming = ܫܪܪܐ;
        }
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    LifeVisitor(AstNode* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = جديد LifeBlock{nullptr, m_statep};
            iterate(nodep);
            ܐܢ (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
        }
    }
    ~LifeVisitor() override {
        ܐܢ (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

صنف LifeTopVisitor final : عام VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
خاص:
    // STATE
    LifeState* ثابت m_statep;  // Current state

    // VISITORS
    فراغ visit(AstCFunc* nodep) override {
        ܐܢ (nodep->entryPoint()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    فراغ visit(AstNodeProcedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    فراغ visit(AstVar*) override {}  // Accelerate
    فراغ visit(AstNodeStmt*) override {}  // Accelerate
    فراغ visit(AstNodeExpr*) override {}  // Accelerate
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    LifeTopVisitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~LifeTopVisitor() override = افتراضي;
};

//######################################################################
// Life class functions

فراغ V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        LifeTopVisitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clearCacheTree();  // Removing assignments may affect isPure
    V3Global::dumpCheckGlobalTree("life", 0, dumpTreeEitherLevel() >= 3);
}
