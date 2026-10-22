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
    ثابت VNUser1_دننه_Use m_inuser1;

    // STATE
عام:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = افتراضي;
    ~LifeState() {
        V3Stats::جمع_کړه_Stat_مجموعه("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::جمع_کړه_Stat_مجموعه("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

صنف Life_Var_ثبت final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_غوټه_Stmt* m_assignp = تش_اشاره;  // Last simple assignment
    AstConst* m_constp = تش_اشاره;  // Known constant value
    منطقي m_isNew = ریښتیا;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    منطقي m_سېټ_Before_Use = خطأ;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    منطقي m_ever_سېټ = خطأ;

عام:
    Life_Var_ثبت() = افتراضي;
    ~Life_Var_ثبت() = افتراضي;

    فراغ init(منطقي سېټ_Before_Use) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = خطأ;
        m_سېټ_Before_Use = سېټ_Before_Use;
    }
    string ascii() ثابت {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        إذا (m_isNew) os << " isNew";
        إذا (m_سېټ_Before_Use) os << " setBeforeUse";
        إذا (m_ever_سېټ) os << " everSet";
        إذا (m_assignp) os << " assignp=" << Ast_غوټه::غوټه_Addr(m_assignp);
        إذا (m_constp) os << " constp=" << Ast_غوټه::غوټه_Addr(m_constp);
        os << "]";
        أرجع os.str();
    }  // LCOV_EXCL_STOP

    فراغ simple_ټاکل(Ast_غوټه_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = تش_اشاره;
        m_ever_سېټ = ریښتیا;
        إذا (Ast_غوټه_ټاکل* ثابت assp = VN_CAST(nodep, ټاکل)) {
            إذا (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    فراغ complex_ټاکل() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = تش_اشاره;
        m_constp = تش_اشاره;
        m_ever_سېټ = ریښتیا;
    }
    فراغ consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = تش_اشاره;
    }
    Ast_غوټه_Stmt* assignp() ثابت { أرجع m_assignp; }
    AstConst* constNodep() ثابت { أرجع m_constp; }
    منطقي isNew() ثابت { أرجع m_isNew; }
    منطقي سېټ_Before_Use() ثابت { أرجع m_سېټ_Before_Use; }
    منطقي ever_سېټ() ثابت { أرجع m_ever_سېټ; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

صنف Life_بلاک final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, Life_Var_ثبت> m_map;
    Life_بلاک* ثابت m_aboveLifep;  // Upper life, or nullptr
    LifeState* ثابت m_statep;  // Current global state
    منطقي m_noopt = خطأ;  // Opaque operation seen; invalidates the block above at exit
    منطقي m_replacedVref = خطأ;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

عام:
    Life_بلاک(Life_بلاک* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~Life_بلاک() = افتراضي;
    // METHODS
    فراغ check_Remove_ټاکل(ثابت AstVarScope* vscp, Life_Var_ثبت& entr) {
        ثابت AstVar* ثابت varp = vscp->varp();
        // We don't optimize any public sigs
        إذا (varp->is_Sig_عامه()) أرجع;
        إذا (varp->isReadByDpi()) أرجع;
        إذا (varp->sensIfacep()) أرجع;
        // Check the var entry, and remove if appropriate
        Ast_غوټه_Stmt* ثابت oldassp = entr.assignp();
        إذا (!oldassp) أرجع;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complex_ټاکل();
        oldassp->unlink_Fr_شا();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.ټېل_Deletep(oldassp), oldassp);
    }
    فراغ simple_ټاکل(AstVarScope* nodep, Ast_غوټه_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        Life_Var_ثبت& entr = m_map[nodep];
        إذا (entr.isNew()) {
            entr.init(ریښتیا);
        } وإلا {
            check_Remove_ټاکل(nodep, entr);
        }
        entr.simple_ټاکل(assp);
        // lifeDump();
    }
    فراغ complex_ټاکل(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        Life_Var_ثبت& entr = m_map[nodep];
        إذا (entr.isNew()) entr.init(خطأ);
        entr.complex_ټاکل();
    }
    فراغ clearReplaced() { m_replacedVref = خطأ; }
    منطقي replaced() ثابت { أرجع m_replacedVref; }
    فراغ varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        Life_Var_ثبت& entr = m_map[nodep];
        إذا (entr.isNew()) {
            entr.init(خطأ);
        } وإلا {
            إذا (AstConst* ثابت constp = entr.constNodep()) {
                إذا (!varrefp->varp()->is_Sig_عامه() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_ونه(خطأ));
                    m_replacedVref = ریښتیا;
                    VL_DO_DANGLING(varrefp->delete_ونه(), varrefp);
                    ++m_statep->m_statAssnCon;
                    أرجع;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    فراغ complex_ټاکل_پیدا_کړه(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        Life_Var_ثبت& entr = m_map[nodep];
        إذا (entr.isNew()) entr.init(خطأ);
        entr.complex_ټاکل();
    }
    فراغ consumed_پیدا_کړه(AstVarScope* nodep) {
        Life_Var_ثبت& entr = m_map[nodep];
        إذا (entr.isNew()) entr.init(خطأ);
        entr.consumed();
    }
    فراغ life_منزل_Above() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        إذا (m_noopt) m_aboveLifep->noopt();
        لكل (خپلکاری& itr : m_map) {
            AstVarScope* ثابت nodep = itr.لومړی;
            m_aboveLifep->complex_ټاکل_پیدا_کړه(nodep);
            إذا (itr.دوهم.ever_سېټ()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complex_ټاکل_پیدا_کړه(nodep);
            } وإلا {
                // Record consumption, so we don't eliminate earlier assignments
                consumed_پیدا_کړه(nodep);
            }
        }
    }
    فراغ dualBranch(Life_بلاک* life1p, Life_بلاک* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_غوټه::user1_Clear_ونه();  // user1p() used on entire tree
        لكل (خپلکاری& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            إذا (itr.دوهم.سېټ_Before_Use()) itr.لومړی->user1(1);
        }
        لكل (خپلکاری& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* ثابت nodep = itr.لومړی;
            إذا (itr.دوهم.سېټ_Before_Use() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                ثابت خپلکاری itab = m_map.پیدا_کړه(nodep);
                إذا (itab != m_map.پای_ټکی()) check_Remove_ټاکل(nodep, itab->دوهم);
            }
        }
        // this->lifeDump();
    }
    فراغ noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = ریښتیا;
    }
    // DEBUG
    فراغ lifeDump() {
        UINFO(5, "  LifeMap:");
        لكل (ثابت خپلکاری& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.دوهم.سېټ_Before_Use() ? "[F]  " : "     ") << itr.لومړی);
            إذا (itr.دوهم.assignp()) {  //
                UINFO(5, "       Ass: " << itr.دوهم.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

صنف LifeVisitor final : عام VNVisitor {
    // STATE
    LifeState* ثابت m_statep;  // Current state
    منطقي m_لري_Timing = خطأ;  // Statement contains timing control
    منطقي m_sideEffect = خطأ;  // Side effects discovered in assign RHS
    منطقي m_noopt = خطأ;  // Disable optimization of variables in this block
    منطقي m_tracingCall = خطأ;  // Iterating into a CCall to a CFunc
    Life_بلاک* m_lifep = تش_اشاره;  // Current active lifetime map for current scope

    // METHODS
    فراغ سېټ_Noopt(ثابت حرف* reasonp) {
        (فراغ)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = ریښتیا;
        m_lifep->noopt();
    }

    فراغ processAssignment(Ast_غوټه_Stmt* nodep, Ast_غوټه_بیان* lhsp, Ast_غوټه_بیان* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = خطأ;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), غوټه_بیان);
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
            m_lifep->simple_ټاکل(vscp, nodep);
        } وإلا {
            iterate_And_بل_Null(lhsp);
        }
    }

    // VISITORS
    فراغ لیدنه(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* ثابت vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        إذا (nodep->access().isWriteOrRW()) {
            m_sideEffect = ریښتیا;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complex_ټاکل(vscp);
        } وإلا {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        إذا (nodep->isTimingControl() || VN_IS(nodep, ټاکل_Force)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            سېټ_Noopt("timing|force");
            إذا (nodep->isTimingControl()) m_لري_Timing = ریښتیا;
            iterateChildren(nodep);
            أرجع;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    فراغ لیدنه(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    فراغ لیدنه(Ast_ټاکل_Dly* nodep) override {
        // V3Life doesn't understand time sense
        إذا (nodep->isTimingControl()) {
            // Don't optimize
            سېټ_Noopt("assigndly");
            m_لري_Timing = ریښتیا;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    فراغ لیدنه(Ast_غوټه_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterate_And_بل_Null(nodep->condp());
        Life_بلاک* ثابت ifLifep = جديد Life_بلاک{m_lifep, m_statep};
        Life_بلاک* ثابت elseLifep = جديد Life_بلاک{m_lifep, m_statep};
        {
            VL_RESTORER(m_lifep);
            m_lifep = ifLifep;
            iterate_And_بل_Null(nodep->thensp());
        }
        {
            VL_RESTORER(m_lifep);
            m_lifep = elseLifep;
            iterate_And_بل_Null(nodep->elsesp());
        }
        UINFO(4, "   join " << nodep);
        // Find sets on both flows
        m_lifep->dualBranch(ifLifep, elseLifep);
        // For the next assignments, clear any variables that were read or written in the block
        ifLifep->life_منزل_Above();
        elseLifep->life_منزل_Above();
        VL_DO_DANGLING(ړنګول ifLifep, ifLifep);
        VL_DO_DANGLING(ړنګول elseLifep, elseLifep);
        UINFO(4, "   if-done " << nodep);
    }
    فراغ لیدنه(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_لري_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = جديد Life_بلاک{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = ریښتیا;
            iterate_And_بل_Null(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_منزل_Above();
            VL_DO_DANGLING(ړنګول m_lifep, m_lifep);
        }
        إذا (m_لري_Timing) سېټ_Noopt("timing");
    }
    فراغ لیدنه(Ast_Jump_بلاک* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_لري_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = جديد Life_بلاک{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = ریښتیا;
            iterate_And_بل_Null(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_منزل_Above();
            VL_DO_DANGLING(ړنګول m_lifep, m_lifep);
        }
        إذا (m_لري_Timing) سېټ_Noopt("timing");
    }
    فراغ لیدنه(Ast_غوټه_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        إذا (nodep->funcp()->ثبت_ټکی()) {
            سېټ_Noopt("ccall");
        } وإلا {
            m_tracingCall = ریښتیا;
            iterate(nodep->funcp());
        }
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        إذا (!m_tracingCall && !nodep->ثبت_ټکی()) أرجع;
        m_tracingCall = خطأ;
        إذا (nodep->recursive()) سېټ_Noopt("recursive");
        إذا (nodep->noLife()) سېټ_Noopt("nolife");
        إذا (nodep->dpiImportPrototype() && !nodep->dpi_خالص()) {
            m_sideEffect = ریښتیا;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCExprUser* nodep) override {
        m_sideEffect = ریښتیا;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCExpr* nodep) override {
        m_sideEffect = ریښتیا;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstVar*) override {}  // Don't want varrefs under it
    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            سېټ_Noopt("timing");
            m_لري_Timing = ریښتیا;
        }
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    LifeVisitor(Ast_غوټه* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = جديد Life_بلاک{تش_اشاره, m_statep};
            iterate(nodep);
            إذا (m_lifep) VL_DO_CLEAR(ړنګول m_lifep, m_lifep = تش_اشاره);
        }
    }
    ~LifeVisitor() override {
        إذا (m_lifep) VL_DO_CLEAR(ړنګول m_lifep, m_lifep = تش_اشاره);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

صنف Life_سر_Visitor final : عام VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
خاص:
    // STATE
    LifeState* ثابت m_statep;  // Current state

    // VISITORS
    فراغ لیدنه(AstCFunc* nodep) override {
        إذا (nodep->ثبت_ټکی()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    فراغ لیدنه(AstVar*) override {}  // Accelerate
    فراغ لیدنه(Ast_غوټه_Stmt*) override {}  // Accelerate
    فراغ لیدنه(Ast_غوټه_بیان*) override {}  // Accelerate
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    Life_سر_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_سر_Visitor() override = افتراضي;
};

//######################################################################
// Life class functions

فراغ V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_سر_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_ونه();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_ونه("life", 0, dump_ونه_Either_Level() >= 3);
}
