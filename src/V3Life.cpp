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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Life.h"

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Structure for global state

વર્ગ LifeState final {
    // NODE STATE
    //   See below
    અચલ VNUser1InUse m_inuser1;

    // STATE
જાહેર:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = મૂળભૂત;
    ~LifeState() {
        V3Stats::addStatSum("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::addStatSum("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

વર્ગ LifeVarEntry final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_ગાંઠ_Stmt* m_assignp = nullptr;  // Last simple assignment
    AstConst* m_constp = nullptr;  // Known constant value
    તાર્કિક m_isNew = સાચું;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    તાર્કિક m_setBeforeUse = ખોટું;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    તાર્કિક m_everSet = ખોટું;

જાહેર:
    LifeVarEntry() = મૂળભૂત;
    ~LifeVarEntry() = મૂળભૂત;

    શૂન્ય init(તાર્કિક setBeforeUse) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = ખોટું;
        m_setBeforeUse = setBeforeUse;
    }
    string ascii() અચલ {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        જો (m_isNew) os << " isNew";
        જો (m_setBeforeUse) os << " setBeforeUse";
        જો (m_everSet) os << " everSet";
        જો (m_assignp) os << " assignp=" << Ast_ગાંઠ::ગાંઠ_Addr(m_assignp);
        જો (m_constp) os << " constp=" << Ast_ગાંઠ::ગાંઠ_Addr(m_constp);
        os << "]";
        પાછા os.str();
    }  // LCOV_EXCL_STOP

    શૂન્ય simpleAssign(Ast_ગાંઠ_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = nullptr;
        m_everSet = સાચું;
        જો (Ast_ગાંઠ_Assign* અચલ assp = VN_CAST(nodep, Assign)) {
            જો (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    શૂન્ય complexAssign() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
        m_constp = nullptr;
        m_everSet = સાચું;
    }
    શૂન્ય consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
    }
    Ast_ગાંઠ_Stmt* assignp() અચલ { પાછા m_assignp; }
    AstConst* constNodep() અચલ { પાછા m_constp; }
    તાર્કિક isNew() અચલ { પાછા m_isNew; }
    તાર્કિક setBeforeUse() અચલ { પાછા m_setBeforeUse; }
    તાર્કિક everSet() અચલ { પાછા m_everSet; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

વર્ગ LifeBlock final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, LifeVarEntry> m_map;
    LifeBlock* અચલ m_aboveLifep;  // Upper life, or nullptr
    LifeState* અચલ m_statep;  // Current global state
    તાર્કિક m_noopt = ખોટું;  // Opaque operation seen; invalidates the block above at exit
    તાર્કિક m_replacedVref = ખોટું;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

જાહેર:
    LifeBlock(LifeBlock* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~LifeBlock() = મૂળભૂત;
    // METHODS
    શૂન્ય checkRemoveAssign(અચલ AstVarScope* vscp, LifeVarEntry& entr) {
        અચલ AstVar* અચલ varp = vscp->varp();
        // We don't optimize any public sigs
        જો (varp->isSigPublic()) પાછા;
        જો (varp->isReadByDpi()) પાછા;
        જો (varp->sensIfacep()) પાછા;
        // Check the var entry, and remove if appropriate
        Ast_ગાંઠ_Stmt* અચલ oldassp = entr.assignp();
        જો (!oldassp) પાછા;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complexAssign();
        oldassp->unlinkFrBack();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.ધકેલો_Deletep(oldassp), oldassp);
    }
    શૂન્ય simpleAssign(AstVarScope* nodep, Ast_ગાંઠ_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        LifeVarEntry& entr = m_map[nodep];
        જો (entr.isNew()) {
            entr.init(સાચું);
        } નહીંતર {
            checkRemoveAssign(nodep, entr);
        }
        entr.simpleAssign(assp);
        // lifeDump();
    }
    શૂન્ય complexAssign(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        જો (entr.isNew()) entr.init(ખોટું);
        entr.complexAssign();
    }
    શૂન્ય clearReplaced() { m_replacedVref = ખોટું; }
    તાર્કિક replaced() અચલ { પાછા m_replacedVref; }
    શૂન્ય varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        LifeVarEntry& entr = m_map[nodep];
        જો (entr.isNew()) {
            entr.init(ખોટું);
        } નહીંતર {
            જો (AstConst* અચલ constp = entr.constNodep()) {
                જો (!varrefp->varp()->isSigPublic() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_વૃક્ષ(ખોટું));
                    m_replacedVref = સાચું;
                    VL_DO_DANGLING(varrefp->delete_વૃક્ષ(), varrefp);
                    ++m_statep->m_statAssnCon;
                    પાછા;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    શૂન્ય complexAssignFind(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        જો (entr.isNew()) entr.init(ખોટું);
        entr.complexAssign();
    }
    શૂન્ય consumedFind(AstVarScope* nodep) {
        LifeVarEntry& entr = m_map[nodep];
        જો (entr.isNew()) entr.init(ખોટું);
        entr.consumed();
    }
    શૂન્ય lifeToAbove() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        જો (m_noopt) m_aboveLifep->noopt();
        માટે (auto& itr : m_map) {
            AstVarScope* અચલ nodep = itr.first;
            m_aboveLifep->complexAssignFind(nodep);
            જો (itr.second.everSet()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complexAssignFind(nodep);
            } નહીંતર {
                // Record consumption, so we don't eliminate earlier assignments
                consumedFind(nodep);
            }
        }
    }
    શૂન્ય dualBranch(LifeBlock* life1p, LifeBlock* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_ગાંઠ::user1_Clear_વૃક્ષ();  // user1p() used on entire tree
        માટે (auto& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            જો (itr.second.setBeforeUse()) itr.first->user1(1);
        }
        માટે (auto& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* અચલ nodep = itr.first;
            જો (itr.second.setBeforeUse() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                અચલ auto itab = m_map.find(nodep);
                જો (itab != m_map.end()) checkRemoveAssign(nodep, itab->second);
            }
        }
        // this->lifeDump();
    }
    શૂન્ય noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = સાચું;
    }
    // DEBUG
    શૂન્ય lifeDump() {
        UINFO(5, "  LifeMap:");
        માટે (અચલ auto& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.second.setBeforeUse() ? "[F]  " : "     ") << itr.first);
            જો (itr.second.assignp()) {  //
                UINFO(5, "       Ass: " << itr.second.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

વર્ગ LifeVisitor final : જાહેર VNVisitor {
    // STATE
    LifeState* અચલ m_statep;  // Current state
    તાર્કિક m_containsTiming = ખોટું;  // Statement contains timing control
    તાર્કિક m_sideEffect = ખોટું;  // Side effects discovered in assign RHS
    તાર્કિક m_noopt = ખોટું;  // Disable optimization of variables in this block
    તાર્કિક m_tracingCall = ખોટું;  // Iterating into a CCall to a CFunc
    LifeBlock* m_lifep = nullptr;  // Current active lifetime map for current scope

    // METHODS
    શૂન્ય setNoopt(અચલ અક્ષર* reasonp) {
        (શૂન્ય)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = સાચું;
        m_lifep->noopt();
    }

    શૂન્ય processAssignment(Ast_ગાંઠ_Stmt* nodep, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = ખોટું;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), ગાંઠ_પદાવલિ);
        જો (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        જો (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* અચલ vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simpleAssign(vscp, nodep);
        } નહીંતર {
            iterateAndNextNull(lhsp);
        }
    }

    // VISITORS
    શૂન્ય visit(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* અચલ vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        જો (nodep->access().isWriteOrRW()) {
            m_sideEffect = સાચું;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complexAssign(vscp);
        } નહીંતર {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        જો (nodep->isTimingControl() || VN_IS(nodep, AssignForce)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            setNoopt("timing|force");
            જો (nodep->isTimingControl()) m_containsTiming = સાચું;
            iterateChildren(nodep);
            પાછા;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    શૂન્ય visit(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    શૂન્ય visit(AstAssignDly* nodep) override {
        // V3Life doesn't understand time sense
        જો (nodep->isTimingControl()) {
            // Don't optimize
            setNoopt("assigndly");
            m_containsTiming = સાચું;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterateAndNextNull(nodep->condp());
        LifeBlock* અચલ ifLifep = નવું LifeBlock{m_lifep, m_statep};
        LifeBlock* અચલ elseLifep = નવું LifeBlock{m_lifep, m_statep};
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
    શૂન્ય visit(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = નવું LifeBlock{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = સાચું;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        જો (m_containsTiming) setNoopt("timing");
    }
    શૂન્ય visit(AstJumpBlock* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = નવું LifeBlock{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = સાચું;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        જો (m_containsTiming) setNoopt("timing");
    }
    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        જો (nodep->funcp()->entryPoint()) {
            setNoopt("ccall");
        } નહીંતર {
            m_tracingCall = સાચું;
            iterate(nodep->funcp());
        }
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        જો (!m_tracingCall && !nodep->entryPoint()) પાછા;
        m_tracingCall = ખોટું;
        જો (nodep->recursive()) setNoopt("recursive");
        જો (nodep->noLife()) setNoopt("nolife");
        જો (nodep->dpiImportPrototype() && !nodep->dpiPure()) {
            m_sideEffect = સાચું;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCExprUser* nodep) override {
        m_sideEffect = સાચું;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCExpr* nodep) override {
        m_sideEffect = સાચું;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstVar*) override {}  // Don't want varrefs under it
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            setNoopt("timing");
            m_containsTiming = સાચું;
        }
        iterateChildren(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    LifeVisitor(Ast_ગાંઠ* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = નવું LifeBlock{nullptr, m_statep};
            iterate(nodep);
            જો (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
        }
    }
    ~LifeVisitor() override {
        જો (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

વર્ગ Life_ટોચ_Visitor final : જાહેર VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
ખાનગી:
    // STATE
    LifeState* અચલ m_statep;  // Current state

    // VISITORS
    શૂન્ય visit(AstCFunc* nodep) override {
        જો (nodep->entryPoint()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    શૂન્ય visit(AstVar*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ_Stmt*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    Life_ટોચ_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_ટોચ_Visitor() override = મૂળભૂત;
};

//######################################################################
// Life class functions

શૂન્ય V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_ટોચ_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_વૃક્ષ();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_વૃક્ષ("life", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
