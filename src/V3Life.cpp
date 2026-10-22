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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Life.h"

#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Structure for global state

বর্গ LifeState final {
    // NODE STATE
    //   See below
    ধ্রুব VNUser1InUse m_inuser1;

    // STATE
সার্বজনিক:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = স্বভাব;
    ~LifeState() {
        V3Stats::addStatSum("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::addStatSum("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

বর্গ LifeVarEntry final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_গ্রন্থি_Stmt* m_assignp = nullptr;  // Last simple assignment
    AstConst* m_constp = nullptr;  // Known constant value
    তার্কিক m_isNew = সত্য;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    তার্কিক m_setBeforeUse = অসত্য;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    তার্কিক m_everSet = অসত্য;

সার্বজনিক:
    LifeVarEntry() = স্বভাব;
    ~LifeVarEntry() = স্বভাব;

    শূন্য init(তার্কিক setBeforeUse) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = অসত্য;
        m_setBeforeUse = setBeforeUse;
    }
    string ascii() ধ্রুব {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        যদি (m_isNew) os << " isNew";
        যদি (m_setBeforeUse) os << " setBeforeUse";
        যদি (m_everSet) os << " everSet";
        যদি (m_assignp) os << " assignp=" << Ast_গ্রন্থি::গ্রন্থি_Addr(m_assignp);
        যদি (m_constp) os << " constp=" << Ast_গ্রন্থি::গ্রন্থি_Addr(m_constp);
        os << "]";
        প্রত্যাগম os.str();
    }  // LCOV_EXCL_STOP

    শূন্য simpleAssign(Ast_গ্রন্থি_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = nullptr;
        m_everSet = সত্য;
        যদি (Ast_গ্রন্থি_Assign* ধ্রুব assp = VN_CAST(nodep, Assign)) {
            যদি (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    শূন্য complexAssign() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
        m_constp = nullptr;
        m_everSet = সত্য;
    }
    শূন্য consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
    }
    Ast_গ্রন্থি_Stmt* assignp() ধ্রুব { প্রত্যাগম m_assignp; }
    AstConst* constNodep() ধ্রুব { প্রত্যাগম m_constp; }
    তার্কিক isNew() ধ্রুব { প্রত্যাগম m_isNew; }
    তার্কিক setBeforeUse() ধ্রুব { প্রত্যাগম m_setBeforeUse; }
    তার্কিক everSet() ধ্রুব { প্রত্যাগম m_everSet; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

বর্গ LifeBlock final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, LifeVarEntry> m_map;
    LifeBlock* ধ্রুব m_aboveLifep;  // Upper life, or nullptr
    LifeState* ধ্রুব m_statep;  // Current global state
    তার্কিক m_noopt = অসত্য;  // Opaque operation seen; invalidates the block above at exit
    তার্কিক m_replacedVref = অসত্য;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

সার্বজনিক:
    LifeBlock(LifeBlock* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~LifeBlock() = স্বভাব;
    // METHODS
    শূন্য checkRemoveAssign(ধ্রুব AstVarScope* vscp, LifeVarEntry& entr) {
        ধ্রুব AstVar* ধ্রুব varp = vscp->varp();
        // We don't optimize any public sigs
        যদি (varp->isSigPublic()) প্রত্যাগম;
        যদি (varp->isReadByDpi()) প্রত্যাগম;
        যদি (varp->sensIfacep()) প্রত্যাগম;
        // Check the var entry, and remove if appropriate
        Ast_গ্রন্থি_Stmt* ধ্রুব oldassp = entr.assignp();
        যদি (!oldassp) প্রত্যাগম;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complexAssign();
        oldassp->unlinkFrBack();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.ঠেলো_Deletep(oldassp), oldassp);
    }
    শূন্য simpleAssign(AstVarScope* nodep, Ast_গ্রন্থি_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        LifeVarEntry& entr = m_map[nodep];
        যদি (entr.isNew()) {
            entr.init(সত্য);
        } অন্যথা {
            checkRemoveAssign(nodep, entr);
        }
        entr.simpleAssign(assp);
        // lifeDump();
    }
    শূন্য complexAssign(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        যদি (entr.isNew()) entr.init(অসত্য);
        entr.complexAssign();
    }
    শূন্য clearReplaced() { m_replacedVref = অসত্য; }
    তার্কিক replaced() ধ্রুব { প্রত্যাগম m_replacedVref; }
    শূন্য varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        LifeVarEntry& entr = m_map[nodep];
        যদি (entr.isNew()) {
            entr.init(অসত্য);
        } অন্যথা {
            যদি (AstConst* ধ্রুব constp = entr.constNodep()) {
                যদি (!varrefp->varp()->isSigPublic() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_বৃক্ষ(অসত্য));
                    m_replacedVref = সত্য;
                    VL_DO_DANGLING(varrefp->delete_বৃক্ষ(), varrefp);
                    ++m_statep->m_statAssnCon;
                    প্রত্যাগম;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    শূন্য complexAssignFind(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        যদি (entr.isNew()) entr.init(অসত্য);
        entr.complexAssign();
    }
    শূন্য consumedFind(AstVarScope* nodep) {
        LifeVarEntry& entr = m_map[nodep];
        যদি (entr.isNew()) entr.init(অসত্য);
        entr.consumed();
    }
    শূন্য lifeToAbove() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        যদি (m_noopt) m_aboveLifep->noopt();
        প্রতি (auto& itr : m_map) {
            AstVarScope* ধ্রুব nodep = itr.first;
            m_aboveLifep->complexAssignFind(nodep);
            যদি (itr.second.everSet()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complexAssignFind(nodep);
            } অন্যথা {
                // Record consumption, so we don't eliminate earlier assignments
                consumedFind(nodep);
            }
        }
    }
    শূন্য dualBranch(LifeBlock* life1p, LifeBlock* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_গ্রন্থি::user1_Clear_বৃক্ষ();  // user1p() used on entire tree
        প্রতি (auto& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            যদি (itr.second.setBeforeUse()) itr.first->user1(1);
        }
        প্রতি (auto& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* ধ্রুব nodep = itr.first;
            যদি (itr.second.setBeforeUse() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                ধ্রুব auto itab = m_map.find(nodep);
                যদি (itab != m_map.end()) checkRemoveAssign(nodep, itab->second);
            }
        }
        // this->lifeDump();
    }
    শূন্য noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = সত্য;
    }
    // DEBUG
    শূন্য lifeDump() {
        UINFO(5, "  LifeMap:");
        প্রতি (ধ্রুব auto& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.second.setBeforeUse() ? "[F]  " : "     ") << itr.first);
            যদি (itr.second.assignp()) {  //
                UINFO(5, "       Ass: " << itr.second.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

বর্গ LifeVisitor final : সার্বজনিক VNVisitor {
    // STATE
    LifeState* ধ্রুব m_statep;  // Current state
    তার্কিক m_containsTiming = অসত্য;  // Statement contains timing control
    তার্কিক m_sideEffect = অসত্য;  // Side effects discovered in assign RHS
    তার্কিক m_noopt = অসত্য;  // Disable optimization of variables in this block
    তার্কিক m_tracingCall = অসত্য;  // Iterating into a CCall to a CFunc
    LifeBlock* m_lifep = nullptr;  // Current active lifetime map for current scope

    // METHODS
    শূন্য setNoopt(ধ্রুব বর্ণ* reasonp) {
        (শূন্য)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = সত্য;
        m_lifep->noopt();
    }

    শূন্য processAssignment(Ast_গ্রন্থি_Stmt* nodep, Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = অসত্য;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), গ্রন্থি_রাশিমালা);
        যদি (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        যদি (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* ধ্রুব vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simpleAssign(vscp, nodep);
        } অন্যথা {
            iterateAndNextNull(lhsp);
        }
    }

    // VISITORS
    শূন্য visit(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* ধ্রুব vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        যদি (nodep->access().isWriteOrRW()) {
            m_sideEffect = সত্য;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complexAssign(vscp);
        } অন্যথা {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        যদি (nodep->isTimingControl() || VN_IS(nodep, AssignForce)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            setNoopt("timing|force");
            যদি (nodep->isTimingControl()) m_containsTiming = সত্য;
            iterateChildren(nodep);
            প্রত্যাগম;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    শূন্য visit(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    শূন্য visit(AstAssignDly* nodep) override {
        // V3Life doesn't understand time sense
        যদি (nodep->isTimingControl()) {
            // Don't optimize
            setNoopt("assigndly");
            m_containsTiming = সত্য;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    শূন্য visit(Ast_গ্রন্থি_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterateAndNextNull(nodep->condp());
        LifeBlock* ধ্রুব ifLifep = নূতন LifeBlock{m_lifep, m_statep};
        LifeBlock* ধ্রুব elseLifep = নূতন LifeBlock{m_lifep, m_statep};
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
    শূন্য visit(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = নূতন LifeBlock{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = সত্য;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        যদি (m_containsTiming) setNoopt("timing");
    }
    শূন্য visit(AstJumpBlock* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = নূতন LifeBlock{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = সত্য;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        যদি (m_containsTiming) setNoopt("timing");
    }
    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        যদি (nodep->funcp()->entryPoint()) {
            setNoopt("ccall");
        } অন্যথা {
            m_tracingCall = সত্য;
            iterate(nodep->funcp());
        }
    }
    শূন্য visit(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        যদি (!m_tracingCall && !nodep->entryPoint()) প্রত্যাগম;
        m_tracingCall = অসত্য;
        যদি (nodep->recursive()) setNoopt("recursive");
        যদি (nodep->noLife()) setNoopt("nolife");
        যদি (nodep->dpiImportPrototype() && !nodep->dpiPure()) {
            m_sideEffect = সত্য;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstCExprUser* nodep) override {
        m_sideEffect = সত্য;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    শূন্য visit(AstCExpr* nodep) override {
        m_sideEffect = সত্য;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    শূন্য visit(AstVar*) override {}  // Don't want varrefs under it
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            setNoopt("timing");
            m_containsTiming = সত্য;
        }
        iterateChildren(nodep);
    }

সার্বজনিক:
    // CONSTRUCTORS
    LifeVisitor(Ast_গ্রন্থি* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = নূতন LifeBlock{nullptr, m_statep};
            iterate(nodep);
            যদি (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
        }
    }
    ~LifeVisitor() override {
        যদি (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

বর্গ Life_শীর্ষ_Visitor final : সার্বজনিক VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
গুপ্ত:
    // STATE
    LifeState* ধ্রুব m_statep;  // Current state

    // VISITORS
    শূন্য visit(AstCFunc* nodep) override {
        যদি (nodep->entryPoint()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    শূন্য visit(AstVar*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি_Stmt*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি_রাশিমালা*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    Life_শীর্ষ_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_শীর্ষ_Visitor() override = স্বভাব;
};

//######################################################################
// Life class functions

শূন্য V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_শীর্ষ_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_বৃক্ষ();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_বৃক্ষ("life", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
