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
    Ast_گرہ_Stmt* m_assignp = nullptr;  // Last simple assignment
    AstConst* m_constp = nullptr;  // Known constant value
    منطقي m_isNew = true;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    منطقي m_setBeforeUse = خطأ;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    منطقي m_everSet = خطأ;

عام:
    LifeVarEntry() = افتراضي;
    ~LifeVarEntry() = افتراضي;

    فراغ init(منطقي setBeforeUse) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = خطأ;
        m_setBeforeUse = setBeforeUse;
    }
    string ascii() ثابت {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        إذا (m_isNew) os << " isNew";
        إذا (m_setBeforeUse) os << " setBeforeUse";
        إذا (m_everSet) os << " everSet";
        إذا (m_assignp) os << " assignp=" << Ast_گرہ::گرہ_Addr(m_assignp);
        إذا (m_constp) os << " constp=" << Ast_گرہ::گرہ_Addr(m_constp);
        os << "]";
        أرجع os.str();
    }  // LCOV_EXCL_STOP

    فراغ simpleAssign(Ast_گرہ_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = nullptr;
        m_everSet = true;
        إذا (Ast_گرہ_Assign* ثابت assp = VN_CAST(nodep, Assign)) {
            إذا (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    فراغ complexAssign() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
        m_constp = nullptr;
        m_everSet = true;
    }
    فراغ consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
    }
    Ast_گرہ_Stmt* assignp() ثابت { أرجع m_assignp; }
    AstConst* constNodep() ثابت { أرجع m_constp; }
    منطقي isNew() ثابت { أرجع m_isNew; }
    منطقي setBeforeUse() ثابت { أرجع m_setBeforeUse; }
    منطقي everSet() ثابت { أرجع m_everSet; }
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
    منطقي m_noopt = خطأ;  // Opaque operation seen; invalidates the block above at exit
    منطقي m_replacedVref = خطأ;  // Replaced a variable reference since last clearing
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
        إذا (varp->isSigPublic()) أرجع;
        إذا (varp->isReadByDpi()) أرجع;
        إذا (varp->sensIfacep()) أرجع;
        // Check the var entry, and remove if appropriate
        Ast_گرہ_Stmt* ثابت oldassp = entr.assignp();
        إذا (!oldassp) أرجع;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complexAssign();
        oldassp->unlinkFrBack();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.ڈالو_Deletep(oldassp), oldassp);
    }
    فراغ simpleAssign(AstVarScope* nodep, Ast_گرہ_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        LifeVarEntry& entr = m_map[nodep];
        إذا (entr.isNew()) {
            entr.init(true);
        } وإلا {
            checkRemoveAssign(nodep, entr);
        }
        entr.simpleAssign(assp);
        // lifeDump();
    }
    فراغ complexAssign(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        إذا (entr.isNew()) entr.init(خطأ);
        entr.complexAssign();
    }
    فراغ clearReplaced() { m_replacedVref = خطأ; }
    منطقي replaced() ثابت { أرجع m_replacedVref; }
    فراغ varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        LifeVarEntry& entr = m_map[nodep];
        إذا (entr.isNew()) {
            entr.init(خطأ);
        } وإلا {
            إذا (AstConst* ثابت constp = entr.constNodep()) {
                إذا (!varrefp->varp()->isSigPublic() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_درخت(خطأ));
                    m_replacedVref = true;
                    VL_DO_DANGLING(varrefp->delete_درخت(), varrefp);
                    ++m_statep->m_statAssnCon;
                    أرجع;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    فراغ complexAssignFind(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        إذا (entr.isNew()) entr.init(خطأ);
        entr.complexAssign();
    }
    فراغ consumedFind(AstVarScope* nodep) {
        LifeVarEntry& entr = m_map[nodep];
        إذا (entr.isNew()) entr.init(خطأ);
        entr.consumed();
    }
    فراغ lifeToAbove() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        إذا (m_noopt) m_aboveLifep->noopt();
        لكل (auto& itr : m_map) {
            AstVarScope* ثابت nodep = itr.first;
            m_aboveLifep->complexAssignFind(nodep);
            إذا (itr.second.everSet()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complexAssignFind(nodep);
            } وإلا {
                // Record consumption, so we don't eliminate earlier assignments
                consumedFind(nodep);
            }
        }
    }
    فراغ dualBranch(LifeBlock* life1p, LifeBlock* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_گرہ::user1_Clear_درخت();  // user1p() used on entire tree
        لكل (auto& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            إذا (itr.second.setBeforeUse()) itr.first->user1(1);
        }
        لكل (auto& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* ثابت nodep = itr.first;
            إذا (itr.second.setBeforeUse() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                ثابت auto itab = m_map.find(nodep);
                إذا (itab != m_map.end()) checkRemoveAssign(nodep, itab->second);
            }
        }
        // this->lifeDump();
    }
    فراغ noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = true;
    }
    // DEBUG
    فراغ lifeDump() {
        UINFO(5, "  LifeMap:");
        لكل (ثابت auto& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.second.setBeforeUse() ? "[F]  " : "     ") << itr.first);
            إذا (itr.second.assignp()) {  //
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
    منطقي m_containsTiming = خطأ;  // Statement contains timing control
    منطقي m_sideEffect = خطأ;  // Side effects discovered in assign RHS
    منطقي m_noopt = خطأ;  // Disable optimization of variables in this block
    منطقي m_tracingCall = خطأ;  // Iterating into a CCall to a CFunc
    LifeBlock* m_lifep = nullptr;  // Current active lifetime map for current scope

    // METHODS
    فراغ setNoopt(ثابت حرف* reasonp) {
        (فراغ)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = true;
        m_lifep->noopt();
    }

    فراغ processAssignment(Ast_گرہ_Stmt* nodep, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = خطأ;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), گرہ_اظہاریہ);
        إذا (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        إذا (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* ثابت vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simpleAssign(vscp, nodep);
        } وإلا {
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
        إذا (nodep->access().isWriteOrRW()) {
            m_sideEffect = true;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complexAssign(vscp);
        } وإلا {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    فراغ visit(Ast_گرہ_Assign* nodep) override {
        إذا (nodep->isTimingControl() || VN_IS(nodep, AssignForce)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            setNoopt("timing|force");
            إذا (nodep->isTimingControl()) m_containsTiming = true;
            iterateChildren(nodep);
            أرجع;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    فراغ visit(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    فراغ visit(AstAssignDly* nodep) override {
        // V3Life doesn't understand time sense
        إذا (nodep->isTimingControl()) {
            // Don't optimize
            setNoopt("assigndly");
            m_containsTiming = true;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    فراغ visit(Ast_گرہ_If* nodep) override {
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
            m_noopt = true;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        إذا (m_containsTiming) setNoopt("timing");
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
            m_noopt = true;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        إذا (m_containsTiming) setNoopt("timing");
    }
    فراغ visit(Ast_گرہ_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        إذا (nodep->funcp()->entryPoint()) {
            setNoopt("ccall");
        } وإلا {
            m_tracingCall = true;
            iterate(nodep->funcp());
        }
    }
    فراغ visit(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        إذا (!m_tracingCall && !nodep->entryPoint()) أرجع;
        m_tracingCall = خطأ;
        إذا (nodep->recursive()) setNoopt("recursive");
        إذا (nodep->noLife()) setNoopt("nolife");
        إذا (nodep->dpiImportPrototype() && !nodep->dpiPure()) {
            m_sideEffect = true;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstCExprUser* nodep) override {
        m_sideEffect = true;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    فراغ visit(AstCExpr* nodep) override {
        m_sideEffect = true;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    فراغ visit(AstVar*) override {}  // Don't want varrefs under it
    فراغ visit(Ast_گرہ* nodep) override {
        إذا (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            setNoopt("timing");
            m_containsTiming = true;
        }
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    LifeVisitor(Ast_گرہ* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = جديد LifeBlock{nullptr, m_statep};
            iterate(nodep);
            إذا (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
        }
    }
    ~LifeVisitor() override {
        إذا (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

صنف Life_چوٹی_Visitor final : عام VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
خاص:
    // STATE
    LifeState* ثابت m_statep;  // Current state

    // VISITORS
    فراغ visit(AstCFunc* nodep) override {
        إذا (nodep->entryPoint()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    فراغ visit(Ast_گرہ_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    فراغ visit(AstVar*) override {}  // Accelerate
    فراغ visit(Ast_گرہ_Stmt*) override {}  // Accelerate
    فراغ visit(Ast_گرہ_اظہاریہ*) override {}  // Accelerate
    فراغ visit(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    Life_چوٹی_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_چوٹی_Visitor() override = افتراضي;
};

//######################################################################
// Life class functions

فراغ V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_چوٹی_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_درخت();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_درخت("life", 0, dump_درخت_Either_Level() >= 3);
}
