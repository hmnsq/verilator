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
    ثابت VNUser1_اندريون_Use m_inuser1;

    // STATE
عام:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = افتراضي;
    ~LifeState() {
        V3Stats::جوڙيو_Stat_جوڙ("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::جوڙيو_Stat_جوڙ("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

صنف Life_Var_داخلا final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_ڳنڍ_Stmt* m_assignp = خالي_اشارو;  // Last simple assignment
    AstConst* m_constp = خالي_اشارو;  // Known constant value
    منطقي m_isNew = سچ;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    منطقي m_سيٽ_Before_Use = خطأ;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    منطقي m_ever_سيٽ = خطأ;

عام:
    Life_Var_داخلا() = افتراضي;
    ~Life_Var_داخلا() = افتراضي;

    فراغ init(منطقي سيٽ_Before_Use) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = خطأ;
        m_سيٽ_Before_Use = سيٽ_Before_Use;
    }
    string ascii() ثابت {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        إذا (m_isNew) os << " isNew";
        إذا (m_سيٽ_Before_Use) os << " setBeforeUse";
        إذا (m_ever_سيٽ) os << " everSet";
        إذا (m_assignp) os << " assignp=" << Ast_ڳنڍ::ڳنڍ_Addr(m_assignp);
        إذا (m_constp) os << " constp=" << Ast_ڳنڍ::ڳنڍ_Addr(m_constp);
        os << "]";
        أرجع os.str();
    }  // LCOV_EXCL_STOP

    فراغ simple_سونپيو(Ast_ڳنڍ_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = خالي_اشارو;
        m_ever_سيٽ = سچ;
        إذا (Ast_ڳنڍ_سونپيو* ثابت assp = VN_CAST(nodep, سونپيو)) {
            إذا (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    فراغ complex_سونپيو() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = خالي_اشارو;
        m_constp = خالي_اشارو;
        m_ever_سيٽ = سچ;
    }
    فراغ consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = خالي_اشارو;
    }
    Ast_ڳنڍ_Stmt* assignp() ثابت { أرجع m_assignp; }
    AstConst* constNodep() ثابت { أرجع m_constp; }
    منطقي isNew() ثابت { أرجع m_isNew; }
    منطقي سيٽ_Before_Use() ثابت { أرجع m_سيٽ_Before_Use; }
    منطقي ever_سيٽ() ثابت { أرجع m_ever_سيٽ; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

صنف Life_بلاڪ final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, Life_Var_داخلا> m_map;
    Life_بلاڪ* ثابت m_aboveLifep;  // Upper life, or nullptr
    LifeState* ثابت m_statep;  // Current global state
    منطقي m_noopt = خطأ;  // Opaque operation seen; invalidates the block above at exit
    منطقي m_replacedVref = خطأ;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

عام:
    Life_بلاڪ(Life_بلاڪ* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~Life_بلاڪ() = افتراضي;
    // METHODS
    فراغ check_Remove_سونپيو(ثابت AstVarScope* vscp, Life_Var_داخلا& entr) {
        ثابت AstVar* ثابت varp = vscp->varp();
        // We don't optimize any public sigs
        إذا (varp->is_Sig_عوامي()) أرجع;
        إذا (varp->isReadByDpi()) أرجع;
        إذا (varp->sensIfacep()) أرجع;
        // Check the var entry, and remove if appropriate
        Ast_ڳنڍ_Stmt* ثابت oldassp = entr.assignp();
        إذا (!oldassp) أرجع;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complex_سونپيو();
        oldassp->unlink_Fr_پٺ();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.ڌڪو_Deletep(oldassp), oldassp);
    }
    فراغ simple_سونپيو(AstVarScope* nodep, Ast_ڳنڍ_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        Life_Var_داخلا& entr = m_map[nodep];
        إذا (entr.isNew()) {
            entr.init(سچ);
        } وإلا {
            check_Remove_سونپيو(nodep, entr);
        }
        entr.simple_سونپيو(assp);
        // lifeDump();
    }
    فراغ complex_سونپيو(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        Life_Var_داخلا& entr = m_map[nodep];
        إذا (entr.isNew()) entr.init(خطأ);
        entr.complex_سونپيو();
    }
    فراغ clearReplaced() { m_replacedVref = خطأ; }
    منطقي replaced() ثابت { أرجع m_replacedVref; }
    فراغ varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        Life_Var_داخلا& entr = m_map[nodep];
        إذا (entr.isNew()) {
            entr.init(خطأ);
        } وإلا {
            إذا (AstConst* ثابت constp = entr.constNodep()) {
                إذا (!varrefp->varp()->is_Sig_عوامي() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_وڻ(خطأ));
                    m_replacedVref = سچ;
                    VL_DO_DANGLING(varrefp->delete_وڻ(), varrefp);
                    ++m_statep->m_statAssnCon;
                    أرجع;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    فراغ complex_سونپيو_ڳوليو(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        Life_Var_داخلا& entr = m_map[nodep];
        إذا (entr.isNew()) entr.init(خطأ);
        entr.complex_سونپيو();
    }
    فراغ consumed_ڳوليو(AstVarScope* nodep) {
        Life_Var_داخلا& entr = m_map[nodep];
        إذا (entr.isNew()) entr.init(خطأ);
        entr.consumed();
    }
    فراغ life_منزل_Above() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        إذا (m_noopt) m_aboveLifep->noopt();
        لكل (خودڪار& itr : m_map) {
            AstVarScope* ثابت nodep = itr.پهريون;
            m_aboveLifep->complex_سونپيو_ڳوليو(nodep);
            إذا (itr.ٻيو.ever_سيٽ()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complex_سونپيو_ڳوليو(nodep);
            } وإلا {
                // Record consumption, so we don't eliminate earlier assignments
                consumed_ڳوليو(nodep);
            }
        }
    }
    فراغ dualBranch(Life_بلاڪ* life1p, Life_بلاڪ* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_ڳنڍ::user1_Clear_وڻ();  // user1p() used on entire tree
        لكل (خودڪار& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            إذا (itr.ٻيو.سيٽ_Before_Use()) itr.پهريون->user1(1);
        }
        لكل (خودڪار& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* ثابت nodep = itr.پهريون;
            إذا (itr.ٻيو.سيٽ_Before_Use() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                ثابت خودڪار itab = m_map.ڳوليو(nodep);
                إذا (itab != m_map.پڇاڙي()) check_Remove_سونپيو(nodep, itab->ٻيو);
            }
        }
        // this->lifeDump();
    }
    فراغ noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = سچ;
    }
    // DEBUG
    فراغ lifeDump() {
        UINFO(5, "  LifeMap:");
        لكل (ثابت خودڪار& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.ٻيو.سيٽ_Before_Use() ? "[F]  " : "     ") << itr.پهريون);
            إذا (itr.ٻيو.assignp()) {  //
                UINFO(5, "       Ass: " << itr.ٻيو.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

صنف LifeVisitor final : عام VNVisitor {
    // STATE
    LifeState* ثابت m_statep;  // Current state
    منطقي m_شامل_آهي_Timing = خطأ;  // Statement contains timing control
    منطقي m_sideEffect = خطأ;  // Side effects discovered in assign RHS
    منطقي m_noopt = خطأ;  // Disable optimization of variables in this block
    منطقي m_tracingCall = خطأ;  // Iterating into a CCall to a CFunc
    Life_بلاڪ* m_lifep = خالي_اشارو;  // Current active lifetime map for current scope

    // METHODS
    فراغ سيٽ_Noopt(ثابت حرف* reasonp) {
        (فراغ)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = سچ;
        m_lifep->noopt();
    }

    فراغ processAssignment(Ast_ڳنڍ_Stmt* nodep, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = خطأ;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), ڳنڍ_اظهار);
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
            m_lifep->simple_سونپيو(vscp, nodep);
        } وإلا {
            iterate_And_ايندڙ_Null(lhsp);
        }
    }

    // VISITORS
    فراغ گهمو(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* ثابت vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        إذا (nodep->access().isWriteOrRW()) {
            m_sideEffect = سچ;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complex_سونپيو(vscp);
        } وإلا {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        إذا (nodep->isTimingControl() || VN_IS(nodep, سونپيو_Force)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            سيٽ_Noopt("timing|force");
            إذا (nodep->isTimingControl()) m_شامل_آهي_Timing = سچ;
            iterateChildren(nodep);
            أرجع;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    فراغ گهمو(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    فراغ گهمو(Ast_سونپيو_Dly* nodep) override {
        // V3Life doesn't understand time sense
        إذا (nodep->isTimingControl()) {
            // Don't optimize
            سيٽ_Noopt("assigndly");
            m_شامل_آهي_Timing = سچ;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    فراغ گهمو(Ast_ڳنڍ_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterate_And_ايندڙ_Null(nodep->condp());
        Life_بلاڪ* ثابت ifLifep = جديد Life_بلاڪ{m_lifep, m_statep};
        Life_بلاڪ* ثابت elseLifep = جديد Life_بلاڪ{m_lifep, m_statep};
        {
            VL_RESTORER(m_lifep);
            m_lifep = ifLifep;
            iterate_And_ايندڙ_Null(nodep->thensp());
        }
        {
            VL_RESTORER(m_lifep);
            m_lifep = elseLifep;
            iterate_And_ايندڙ_Null(nodep->elsesp());
        }
        UINFO(4, "   join " << nodep);
        // Find sets on both flows
        m_lifep->dualBranch(ifLifep, elseLifep);
        // For the next assignments, clear any variables that were read or written in the block
        ifLifep->life_منزل_Above();
        elseLifep->life_منزل_Above();
        VL_DO_DANGLING(ميٽيو ifLifep, ifLifep);
        VL_DO_DANGLING(ميٽيو elseLifep, elseLifep);
        UINFO(4, "   if-done " << nodep);
    }
    فراغ گهمو(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_شامل_آهي_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = جديد Life_بلاڪ{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = سچ;
            iterate_And_ايندڙ_Null(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_منزل_Above();
            VL_DO_DANGLING(ميٽيو m_lifep, m_lifep);
        }
        إذا (m_شامل_آهي_Timing) سيٽ_Noopt("timing");
    }
    فراغ گهمو(Ast_Jump_بلاڪ* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_شامل_آهي_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = جديد Life_بلاڪ{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = سچ;
            iterate_And_ايندڙ_Null(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_منزل_Above();
            VL_DO_DANGLING(ميٽيو m_lifep, m_lifep);
        }
        إذا (m_شامل_آهي_Timing) سيٽ_Noopt("timing");
    }
    فراغ گهمو(Ast_ڳنڍ_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        إذا (nodep->funcp()->داخلا_نقطو()) {
            سيٽ_Noopt("ccall");
        } وإلا {
            m_tracingCall = سچ;
            iterate(nodep->funcp());
        }
    }
    فراغ گهمو(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        إذا (!m_tracingCall && !nodep->داخلا_نقطو()) أرجع;
        m_tracingCall = خطأ;
        إذا (nodep->recursive()) سيٽ_Noopt("recursive");
        إذا (nodep->noLife()) سيٽ_Noopt("nolife");
        إذا (nodep->dpiImportPrototype() && !nodep->dpi_خالص()) {
            m_sideEffect = سچ;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(AstCExprUser* nodep) override {
        m_sideEffect = سچ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    فراغ گهمو(AstCExpr* nodep) override {
        m_sideEffect = سچ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    فراغ گهمو(AstVar*) override {}  // Don't want varrefs under it
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            سيٽ_Noopt("timing");
            m_شامل_آهي_Timing = سچ;
        }
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    LifeVisitor(Ast_ڳنڍ* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = جديد Life_بلاڪ{خالي_اشارو, m_statep};
            iterate(nodep);
            إذا (m_lifep) VL_DO_CLEAR(ميٽيو m_lifep, m_lifep = خالي_اشارو);
        }
    }
    ~LifeVisitor() override {
        إذا (m_lifep) VL_DO_CLEAR(ميٽيو m_lifep, m_lifep = خالي_اشارو);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

صنف Life_چوٽي_Visitor final : عام VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
خاص:
    // STATE
    LifeState* ثابت m_statep;  // Current state

    // VISITORS
    فراغ گهمو(AstCFunc* nodep) override {
        إذا (nodep->داخلا_نقطو()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    فراغ گهمو(Ast_ڳنڍ_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    فراغ گهمو(AstVar*) override {}  // Accelerate
    فراغ گهمو(Ast_ڳنڍ_Stmt*) override {}  // Accelerate
    فراغ گهمو(Ast_ڳنڍ_اظهار*) override {}  // Accelerate
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    Life_چوٽي_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_چوٽي_Visitor() override = افتراضي;
};

//######################################################################
// Life class functions

فراغ V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_چوٽي_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_وڻ();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_وڻ("life", 0, dump_وڻ_Either_Level() >= 3);
}
