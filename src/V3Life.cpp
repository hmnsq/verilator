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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Life.h"

#شامل "V3Const.h"
#شامل "V3Stats.h"

#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Structure for global state

جماعت LifeState final {
    // NODE STATE
    //   See below
    ثابت VNUser1_اندر_Use m_inuser1;

    // STATE
عوامی:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = طےشدہ;
    ~LifeState() {
        V3Stats::جوڑو_Stat_مجموع("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::جوڑو_Stat_مجموع("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

جماعت Life_Var_اندراج final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_گرہ_Stmt* m_assignp = خالی_اشاریہ;  // Last simple assignment
    AstConst* m_constp = خالی_اشاریہ;  // Known constant value
    منطقی m_isNew = سچ;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    منطقی m_سیٹ_Before_Use = جھوٹ;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    منطقی m_ever_سیٹ = جھوٹ;

عوامی:
    Life_Var_اندراج() = طےشدہ;
    ~Life_Var_اندراج() = طےشدہ;

    خالی init(منطقی سیٹ_Before_Use) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = جھوٹ;
        m_سیٹ_Before_Use = سیٹ_Before_Use;
    }
    string ascii() ثابت {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        اگر (m_isNew) os << " isNew";
        اگر (m_سیٹ_Before_Use) os << " setBeforeUse";
        اگر (m_ever_سیٹ) os << " everSet";
        اگر (m_assignp) os << " assignp=" << Ast_گرہ::گرہ_Addr(m_assignp);
        اگر (m_constp) os << " constp=" << Ast_گرہ::گرہ_Addr(m_constp);
        os << "]";
        واپس os.str();
    }  // LCOV_EXCL_STOP

    خالی simple_تفویض(Ast_گرہ_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = خالی_اشاریہ;
        m_ever_سیٹ = سچ;
        اگر (Ast_گرہ_تفویض* ثابت assp = VN_CAST(nodep, تفویض)) {
            اگر (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    خالی complex_تفویض() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = خالی_اشاریہ;
        m_constp = خالی_اشاریہ;
        m_ever_سیٹ = سچ;
    }
    خالی consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = خالی_اشاریہ;
    }
    Ast_گرہ_Stmt* assignp() ثابت { واپس m_assignp; }
    AstConst* constNodep() ثابت { واپس m_constp; }
    منطقی isNew() ثابت { واپس m_isNew; }
    منطقی سیٹ_Before_Use() ثابت { واپس m_سیٹ_Before_Use; }
    منطقی ever_سیٹ() ثابت { واپس m_ever_سیٹ; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

جماعت Life_بلاک final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, Life_Var_اندراج> m_map;
    Life_بلاک* ثابت m_aboveLifep;  // Upper life, or nullptr
    LifeState* ثابت m_statep;  // Current global state
    منطقی m_noopt = جھوٹ;  // Opaque operation seen; invalidates the block above at exit
    منطقی m_replacedVref = جھوٹ;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

عوامی:
    Life_بلاک(Life_بلاک* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~Life_بلاک() = طےشدہ;
    // METHODS
    خالی check_Remove_تفویض(ثابت AstVarScope* vscp, Life_Var_اندراج& entr) {
        ثابت AstVar* ثابت varp = vscp->varp();
        // We don't optimize any public sigs
        اگر (varp->is_Sig_عوامی()) واپس;
        اگر (varp->isReadByDpi()) واپس;
        اگر (varp->sensIfacep()) واپس;
        // Check the var entry, and remove if appropriate
        Ast_گرہ_Stmt* ثابت oldassp = entr.assignp();
        اگر (!oldassp) واپس;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complex_تفویض();
        oldassp->unlink_Fr_عقب();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.ڈالو_Deletep(oldassp), oldassp);
    }
    خالی simple_تفویض(AstVarScope* nodep, Ast_گرہ_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        Life_Var_اندراج& entr = m_map[nodep];
        اگر (entr.isNew()) {
            entr.init(سچ);
        } ورنہ {
            check_Remove_تفویض(nodep, entr);
        }
        entr.simple_تفویض(assp);
        // lifeDump();
    }
    خالی complex_تفویض(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        Life_Var_اندراج& entr = m_map[nodep];
        اگر (entr.isNew()) entr.init(جھوٹ);
        entr.complex_تفویض();
    }
    خالی clearReplaced() { m_replacedVref = جھوٹ; }
    منطقی replaced() ثابت { واپس m_replacedVref; }
    خالی varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        Life_Var_اندراج& entr = m_map[nodep];
        اگر (entr.isNew()) {
            entr.init(جھوٹ);
        } ورنہ {
            اگر (AstConst* ثابت constp = entr.constNodep()) {
                اگر (!varrefp->varp()->is_Sig_عوامی() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_درخت(جھوٹ));
                    m_replacedVref = سچ;
                    VL_DO_DANGLING(varrefp->delete_درخت(), varrefp);
                    ++m_statep->m_statAssnCon;
                    واپس;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    خالی complex_تفویض_ڈھونڈو(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        Life_Var_اندراج& entr = m_map[nodep];
        اگر (entr.isNew()) entr.init(جھوٹ);
        entr.complex_تفویض();
    }
    خالی consumed_ڈھونڈو(AstVarScope* nodep) {
        Life_Var_اندراج& entr = m_map[nodep];
        اگر (entr.isNew()) entr.init(جھوٹ);
        entr.consumed();
    }
    خالی life_مقصد_Above() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        اگر (m_noopt) m_aboveLifep->noopt();
        برائے (خودکار& itr : m_map) {
            AstVarScope* ثابت nodep = itr.پہلا;
            m_aboveLifep->complex_تفویض_ڈھونڈو(nodep);
            اگر (itr.دوسرا.ever_سیٹ()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complex_تفویض_ڈھونڈو(nodep);
            } ورنہ {
                // Record consumption, so we don't eliminate earlier assignments
                consumed_ڈھونڈو(nodep);
            }
        }
    }
    خالی dualBranch(Life_بلاک* life1p, Life_بلاک* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_گرہ::user1_Clear_درخت();  // user1p() used on entire tree
        برائے (خودکار& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            اگر (itr.دوسرا.سیٹ_Before_Use()) itr.پہلا->user1(1);
        }
        برائے (خودکار& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* ثابت nodep = itr.پہلا;
            اگر (itr.دوسرا.سیٹ_Before_Use() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                ثابت خودکار itab = m_map.ڈھونڈو(nodep);
                اگر (itab != m_map.انتہا()) check_Remove_تفویض(nodep, itab->دوسرا);
            }
        }
        // this->lifeDump();
    }
    خالی noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = سچ;
    }
    // DEBUG
    خالی lifeDump() {
        UINFO(5, "  LifeMap:");
        برائے (ثابت خودکار& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.دوسرا.سیٹ_Before_Use() ? "[F]  " : "     ") << itr.پہلا);
            اگر (itr.دوسرا.assignp()) {  //
                UINFO(5, "       Ass: " << itr.دوسرا.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

جماعت LifeVisitor final : عوامی VNVisitor {
    // STATE
    LifeState* ثابت m_statep;  // Current state
    منطقی m_شامل_ہے_Timing = جھوٹ;  // Statement contains timing control
    منطقی m_sideEffect = جھوٹ;  // Side effects discovered in assign RHS
    منطقی m_noopt = جھوٹ;  // Disable optimization of variables in this block
    منطقی m_tracingCall = جھوٹ;  // Iterating into a CCall to a CFunc
    Life_بلاک* m_lifep = خالی_اشاریہ;  // Current active lifetime map for current scope

    // METHODS
    خالی سیٹ_Noopt(ثابت حرف* reasonp) {
        (خالی)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = سچ;
        m_lifep->noopt();
    }

    خالی processAssignment(Ast_گرہ_Stmt* nodep, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = جھوٹ;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), گرہ_اظہاریہ);
        اگر (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        اگر (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* ثابت vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simple_تفویض(vscp, nodep);
        } ورنہ {
            iterate_And_اگلا_Null(lhsp);
        }
    }

    // VISITORS
    خالی دورہ(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* ثابت vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        اگر (nodep->access().isWriteOrRW()) {
            m_sideEffect = سچ;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complex_تفویض(vscp);
        } ورنہ {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        اگر (nodep->isTimingControl() || VN_IS(nodep, تفویض_Force)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            سیٹ_Noopt("timing|force");
            اگر (nodep->isTimingControl()) m_شامل_ہے_Timing = سچ;
            iterateChildren(nodep);
            واپس;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    خالی دورہ(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    خالی دورہ(Ast_تفویض_Dly* nodep) override {
        // V3Life doesn't understand time sense
        اگر (nodep->isTimingControl()) {
            // Don't optimize
            سیٹ_Noopt("assigndly");
            m_شامل_ہے_Timing = سچ;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    خالی دورہ(Ast_گرہ_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterate_And_اگلا_Null(nodep->condp());
        Life_بلاک* ثابت ifLifep = نیا Life_بلاک{m_lifep, m_statep};
        Life_بلاک* ثابت elseLifep = نیا Life_بلاک{m_lifep, m_statep};
        {
            VL_RESTORER(m_lifep);
            m_lifep = ifLifep;
            iterate_And_اگلا_Null(nodep->thensp());
        }
        {
            VL_RESTORER(m_lifep);
            m_lifep = elseLifep;
            iterate_And_اگلا_Null(nodep->elsesp());
        }
        UINFO(4, "   join " << nodep);
        // Find sets on both flows
        m_lifep->dualBranch(ifLifep, elseLifep);
        // For the next assignments, clear any variables that were read or written in the block
        ifLifep->life_مقصد_Above();
        elseLifep->life_مقصد_Above();
        VL_DO_DANGLING(حذف ifLifep, ifLifep);
        VL_DO_DANGLING(حذف elseLifep, elseLifep);
        UINFO(4, "   if-done " << nodep);
    }
    خالی دورہ(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_شامل_ہے_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = نیا Life_بلاک{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = سچ;
            iterate_And_اگلا_Null(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_مقصد_Above();
            VL_DO_DANGLING(حذف m_lifep, m_lifep);
        }
        اگر (m_شامل_ہے_Timing) سیٹ_Noopt("timing");
    }
    خالی دورہ(Ast_Jump_بلاک* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_شامل_ہے_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = نیا Life_بلاک{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = سچ;
            iterate_And_اگلا_Null(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_مقصد_Above();
            VL_DO_DANGLING(حذف m_lifep, m_lifep);
        }
        اگر (m_شامل_ہے_Timing) سیٹ_Noopt("timing");
    }
    خالی دورہ(Ast_گرہ_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        اگر (nodep->funcp()->اندراج_نقطہ()) {
            سیٹ_Noopt("ccall");
        } ورنہ {
            m_tracingCall = سچ;
            iterate(nodep->funcp());
        }
    }
    خالی دورہ(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        اگر (!m_tracingCall && !nodep->اندراج_نقطہ()) واپس;
        m_tracingCall = جھوٹ;
        اگر (nodep->recursive()) سیٹ_Noopt("recursive");
        اگر (nodep->noLife()) سیٹ_Noopt("nolife");
        اگر (nodep->dpiImportPrototype() && !nodep->dpi_خالص()) {
            m_sideEffect = سچ;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstCExprUser* nodep) override {
        m_sideEffect = سچ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    خالی دورہ(AstCExpr* nodep) override {
        m_sideEffect = سچ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    خالی دورہ(AstVar*) override {}  // Don't want varrefs under it
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            سیٹ_Noopt("timing");
            m_شامل_ہے_Timing = سچ;
        }
        iterateChildren(nodep);
    }

عوامی:
    // CONSTRUCTORS
    LifeVisitor(Ast_گرہ* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = نیا Life_بلاک{خالی_اشاریہ, m_statep};
            iterate(nodep);
            اگر (m_lifep) VL_DO_CLEAR(حذف m_lifep, m_lifep = خالی_اشاریہ);
        }
    }
    ~LifeVisitor() override {
        اگر (m_lifep) VL_DO_CLEAR(حذف m_lifep, m_lifep = خالی_اشاریہ);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

جماعت Life_چوٹی_Visitor final : عوامی VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
نجی:
    // STATE
    LifeState* ثابت m_statep;  // Current state

    // VISITORS
    خالی دورہ(AstCFunc* nodep) override {
        اگر (nodep->اندراج_نقطہ()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    خالی دورہ(Ast_گرہ_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    خالی دورہ(AstVar*) override {}  // Accelerate
    خالی دورہ(Ast_گرہ_Stmt*) override {}  // Accelerate
    خالی دورہ(Ast_گرہ_اظہاریہ*) override {}  // Accelerate
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    Life_چوٹی_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_چوٹی_Visitor() override = طےشدہ;
};

//######################################################################
// Life class functions

خالی V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_چوٹی_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_درخت();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_درخت("life", 0, dump_درخت_Either_Level() >= 3);
}
