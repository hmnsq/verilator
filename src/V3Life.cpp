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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Life.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Structure for global state

ਵਰਗ LifeState final {
    // NODE STATE
    //   See below
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // STATE
ਜਨਤਕ:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = ਮੂਲ;
    ~LifeState() {
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

ਵਰਗ Life_Var_ਇੰਦਰਾਜ final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_ਗੰਢ_Stmt* m_assignp = ਖਾਲੀ_ਸੂਚਕ;  // Last simple assignment
    AstConst* m_constp = ਖਾਲੀ_ਸੂਚਕ;  // Known constant value
    ਤਾਰਕਿਕ m_isNew = ਸੱਚ;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    ਤਾਰਕਿਕ m_ਸੈੱਟ_Before_Use = ਝੂਠ;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    ਤਾਰਕਿਕ m_ever_ਸੈੱਟ = ਝੂਠ;

ਜਨਤਕ:
    Life_Var_ਇੰਦਰਾਜ() = ਮੂਲ;
    ~Life_Var_ਇੰਦਰਾਜ() = ਮੂਲ;

    ਖਾਲੀ init(ਤਾਰਕਿਕ ਸੈੱਟ_Before_Use) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = ਝੂਠ;
        m_ਸੈੱਟ_Before_Use = ਸੈੱਟ_Before_Use;
    }
    string ascii() ਅਚਲ {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        ਜੇ (m_isNew) os << " isNew";
        ਜੇ (m_ਸੈੱਟ_Before_Use) os << " setBeforeUse";
        ਜੇ (m_ever_ਸੈੱਟ) os << " everSet";
        ਜੇ (m_assignp) os << " assignp=" << Ast_ਗੰਢ::ਗੰਢ_Addr(m_assignp);
        ਜੇ (m_constp) os << " constp=" << Ast_ਗੰਢ::ਗੰਢ_Addr(m_constp);
        os << "]";
        ਵਾਪਸ os.str();
    }  // LCOV_EXCL_STOP

    ਖਾਲੀ simple_ਸੌਂਪੋ(Ast_ਗੰਢ_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = ਖਾਲੀ_ਸੂਚਕ;
        m_ever_ਸੈੱਟ = ਸੱਚ;
        ਜੇ (Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = VN_CAST(nodep, ਸੌਂਪੋ)) {
            ਜੇ (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    ਖਾਲੀ complex_ਸੌਂਪੋ() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = ਖਾਲੀ_ਸੂਚਕ;
        m_constp = ਖਾਲੀ_ਸੂਚਕ;
        m_ever_ਸੈੱਟ = ਸੱਚ;
    }
    ਖਾਲੀ consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = ਖਾਲੀ_ਸੂਚਕ;
    }
    Ast_ਗੰਢ_Stmt* assignp() ਅਚਲ { ਵਾਪਸ m_assignp; }
    AstConst* constNodep() ਅਚਲ { ਵਾਪਸ m_constp; }
    ਤਾਰਕਿਕ isNew() ਅਚਲ { ਵਾਪਸ m_isNew; }
    ਤਾਰਕਿਕ ਸੈੱਟ_Before_Use() ਅਚਲ { ਵਾਪਸ m_ਸੈੱਟ_Before_Use; }
    ਤਾਰਕਿਕ ever_ਸੈੱਟ() ਅਚਲ { ਵਾਪਸ m_ever_ਸੈੱਟ; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

ਵਰਗ Life_ਖੰਡ final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, Life_Var_ਇੰਦਰਾਜ> m_map;
    Life_ਖੰਡ* ਅਚਲ m_aboveLifep;  // Upper life, or nullptr
    LifeState* ਅਚਲ m_statep;  // Current global state
    ਤਾਰਕਿਕ m_noopt = ਝੂਠ;  // Opaque operation seen; invalidates the block above at exit
    ਤਾਰਕਿਕ m_replacedVref = ਝੂਠ;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

ਜਨਤਕ:
    Life_ਖੰਡ(Life_ਖੰਡ* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~Life_ਖੰਡ() = ਮੂਲ;
    // METHODS
    ਖਾਲੀ check_Remove_ਸੌਂਪੋ(ਅਚਲ AstVarScope* vscp, Life_Var_ਇੰਦਰਾਜ& entr) {
        ਅਚਲ AstVar* ਅਚਲ varp = vscp->varp();
        // We don't optimize any public sigs
        ਜੇ (varp->is_Sig_ਜਨਤਕ()) ਵਾਪਸ;
        ਜੇ (varp->isReadByDpi()) ਵਾਪਸ;
        ਜੇ (varp->sensIfacep()) ਵਾਪਸ;
        // Check the var entry, and remove if appropriate
        Ast_ਗੰਢ_Stmt* ਅਚਲ oldassp = entr.assignp();
        ਜੇ (!oldassp) ਵਾਪਸ;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complex_ਸੌਂਪੋ();
        oldassp->unlink_Fr_ਪਿੱਠ();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.ਧੱਕੋ_Deletep(oldassp), oldassp);
    }
    ਖਾਲੀ simple_ਸੌਂਪੋ(AstVarScope* nodep, Ast_ਗੰਢ_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        Life_Var_ਇੰਦਰਾਜ& entr = m_map[nodep];
        ਜੇ (entr.isNew()) {
            entr.init(ਸੱਚ);
        } ਨਹੀਂਤਾਂ {
            check_Remove_ਸੌਂਪੋ(nodep, entr);
        }
        entr.simple_ਸੌਂਪੋ(assp);
        // lifeDump();
    }
    ਖਾਲੀ complex_ਸੌਂਪੋ(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        Life_Var_ਇੰਦਰਾਜ& entr = m_map[nodep];
        ਜੇ (entr.isNew()) entr.init(ਝੂਠ);
        entr.complex_ਸੌਂਪੋ();
    }
    ਖਾਲੀ clearReplaced() { m_replacedVref = ਝੂਠ; }
    ਤਾਰਕਿਕ replaced() ਅਚਲ { ਵਾਪਸ m_replacedVref; }
    ਖਾਲੀ varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        Life_Var_ਇੰਦਰਾਜ& entr = m_map[nodep];
        ਜੇ (entr.isNew()) {
            entr.init(ਝੂਠ);
        } ਨਹੀਂਤਾਂ {
            ਜੇ (AstConst* ਅਚਲ constp = entr.constNodep()) {
                ਜੇ (!varrefp->varp()->is_Sig_ਜਨਤਕ() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_ਰੁੱਖ(ਝੂਠ));
                    m_replacedVref = ਸੱਚ;
                    VL_DO_DANGLING(varrefp->delete_ਰੁੱਖ(), varrefp);
                    ++m_statep->m_statAssnCon;
                    ਵਾਪਸ;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    ਖਾਲੀ complex_ਸੌਂਪੋ_ਲੱਭੋ(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        Life_Var_ਇੰਦਰਾਜ& entr = m_map[nodep];
        ਜੇ (entr.isNew()) entr.init(ਝੂਠ);
        entr.complex_ਸੌਂਪੋ();
    }
    ਖਾਲੀ consumed_ਲੱਭੋ(AstVarScope* nodep) {
        Life_Var_ਇੰਦਰਾਜ& entr = m_map[nodep];
        ਜੇ (entr.isNew()) entr.init(ਝੂਠ);
        entr.consumed();
    }
    ਖਾਲੀ life_ਮੰਜ਼ਿਲ_Above() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        ਜੇ (m_noopt) m_aboveLifep->noopt();
        ਲਈ (ਸਵੈਚਾਲਿਤ& itr : m_map) {
            AstVarScope* ਅਚਲ nodep = itr.ਪਹਿਲਾ;
            m_aboveLifep->complex_ਸੌਂਪੋ_ਲੱਭੋ(nodep);
            ਜੇ (itr.ਦੂਜਾ.ever_ਸੈੱਟ()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complex_ਸੌਂਪੋ_ਲੱਭੋ(nodep);
            } ਨਹੀਂਤਾਂ {
                // Record consumption, so we don't eliminate earlier assignments
                consumed_ਲੱਭੋ(nodep);
            }
        }
    }
    ਖਾਲੀ dualBranch(Life_ਖੰਡ* life1p, Life_ਖੰਡ* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_ਗੰਢ::user1_Clear_ਰੁੱਖ();  // user1p() used on entire tree
        ਲਈ (ਸਵੈਚਾਲਿਤ& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            ਜੇ (itr.ਦੂਜਾ.ਸੈੱਟ_Before_Use()) itr.ਪਹਿਲਾ->user1(1);
        }
        ਲਈ (ਸਵੈਚਾਲਿਤ& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* ਅਚਲ nodep = itr.ਪਹਿਲਾ;
            ਜੇ (itr.ਦੂਜਾ.ਸੈੱਟ_Before_Use() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                ਅਚਲ ਸਵੈਚਾਲਿਤ itab = m_map.ਲੱਭੋ(nodep);
                ਜੇ (itab != m_map.ਸਿਰਾ()) check_Remove_ਸੌਂਪੋ(nodep, itab->ਦੂਜਾ);
            }
        }
        // this->lifeDump();
    }
    ਖਾਲੀ noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = ਸੱਚ;
    }
    // DEBUG
    ਖਾਲੀ lifeDump() {
        UINFO(5, "  LifeMap:");
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.ਦੂਜਾ.ਸੈੱਟ_Before_Use() ? "[F]  " : "     ") << itr.ਪਹਿਲਾ);
            ਜੇ (itr.ਦੂਜਾ.assignp()) {  //
                UINFO(5, "       Ass: " << itr.ਦੂਜਾ.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

ਵਰਗ LifeVisitor final : ਜਨਤਕ VNVisitor {
    // STATE
    LifeState* ਅਚਲ m_statep;  // Current state
    ਤਾਰਕਿਕ m_ਸ਼ਾਮਲ_ਹੈ_Timing = ਝੂਠ;  // Statement contains timing control
    ਤਾਰਕਿਕ m_sideEffect = ਝੂਠ;  // Side effects discovered in assign RHS
    ਤਾਰਕਿਕ m_noopt = ਝੂਠ;  // Disable optimization of variables in this block
    ਤਾਰਕਿਕ m_tracingCall = ਝੂਠ;  // Iterating into a CCall to a CFunc
    Life_ਖੰਡ* m_lifep = ਖਾਲੀ_ਸੂਚਕ;  // Current active lifetime map for current scope

    // METHODS
    ਖਾਲੀ ਸੈੱਟ_Noopt(ਅਚਲ ਅੱਖਰ* reasonp) {
        (ਖਾਲੀ)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = ਸੱਚ;
        m_lifep->noopt();
    }

    ਖਾਲੀ processAssignment(Ast_ਗੰਢ_Stmt* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = ਝੂਠ;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), ਗੰਢ_ਵਿਅੰਜਕ);
        ਜੇ (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        ਜੇ (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* ਅਚਲ vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simple_ਸੌਂਪੋ(vscp, nodep);
        } ਨਹੀਂਤਾਂ {
            iterate_And_ਅਗਲਾ_Null(lhsp);
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* ਅਚਲ vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        ਜੇ (nodep->access().isWriteOrRW()) {
            m_sideEffect = ਸੱਚ;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complex_ਸੌਂਪੋ(vscp);
        } ਨਹੀਂਤਾਂ {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਜੇ (nodep->isTimingControl() || VN_IS(nodep, ਸੌਂਪੋ_Force)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            ਸੈੱਟ_Noopt("timing|force");
            ਜੇ (nodep->isTimingControl()) m_ਸ਼ਾਮਲ_ਹੈ_Timing = ਸੱਚ;
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override {
        // V3Life doesn't understand time sense
        ਜੇ (nodep->isTimingControl()) {
            // Don't optimize
            ਸੈੱਟ_Noopt("assigndly");
            m_ਸ਼ਾਮਲ_ਹੈ_Timing = ਸੱਚ;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterate_And_ਅਗਲਾ_Null(nodep->condp());
        Life_ਖੰਡ* ਅਚਲ ifLifep = ਨਵਾਂ Life_ਖੰਡ{m_lifep, m_statep};
        Life_ਖੰਡ* ਅਚਲ elseLifep = ਨਵਾਂ Life_ਖੰਡ{m_lifep, m_statep};
        {
            VL_RESTORER(m_lifep);
            m_lifep = ifLifep;
            iterate_And_ਅਗਲਾ_Null(nodep->thensp());
        }
        {
            VL_RESTORER(m_lifep);
            m_lifep = elseLifep;
            iterate_And_ਅਗਲਾ_Null(nodep->elsesp());
        }
        UINFO(4, "   join " << nodep);
        // Find sets on both flows
        m_lifep->dualBranch(ifLifep, elseLifep);
        // For the next assignments, clear any variables that were read or written in the block
        ifLifep->life_ਮੰਜ਼ਿਲ_Above();
        elseLifep->life_ਮੰਜ਼ਿਲ_Above();
        VL_DO_DANGLING(ਮਿਟਾਓ ifLifep, ifLifep);
        VL_DO_DANGLING(ਮਿਟਾਓ elseLifep, elseLifep);
        UINFO(4, "   if-done " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_ਸ਼ਾਮਲ_ਹੈ_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = ਨਵਾਂ Life_ਖੰਡ{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = ਸੱਚ;
            iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_ਮੰਜ਼ਿਲ_Above();
            VL_DO_DANGLING(ਮਿਟਾਓ m_lifep, m_lifep);
        }
        ਜੇ (m_ਸ਼ਾਮਲ_ਹੈ_Timing) ਸੈੱਟ_Noopt("timing");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Jump_ਖੰਡ* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_ਸ਼ਾਮਲ_ਹੈ_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = ਨਵਾਂ Life_ਖੰਡ{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = ਸੱਚ;
            iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_ਮੰਜ਼ਿਲ_Above();
            VL_DO_DANGLING(ਮਿਟਾਓ m_lifep, m_lifep);
        }
        ਜੇ (m_ਸ਼ਾਮਲ_ਹੈ_Timing) ਸੈੱਟ_Noopt("timing");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        ਜੇ (nodep->funcp()->ਇੰਦਰਾਜ_ਬਿੰਦੂ()) {
            ਸੈੱਟ_Noopt("ccall");
        } ਨਹੀਂਤਾਂ {
            m_tracingCall = ਸੱਚ;
            iterate(nodep->funcp());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        ਜੇ (!m_tracingCall && !nodep->ਇੰਦਰਾਜ_ਬਿੰਦੂ()) ਵਾਪਸ;
        m_tracingCall = ਝੂਠ;
        ਜੇ (nodep->recursive()) ਸੈੱਟ_Noopt("recursive");
        ਜੇ (nodep->noLife()) ਸੈੱਟ_Noopt("nolife");
        ਜੇ (nodep->dpiImportPrototype() && !nodep->dpi_ਸ਼ੁੱਧ()) {
            m_sideEffect = ਸੱਚ;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExprUser* nodep) override {
        m_sideEffect = ਸੱਚ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExpr* nodep) override {
        m_sideEffect = ਸੱਚ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}  // Don't want varrefs under it
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            ਸੈੱਟ_Noopt("timing");
            m_ਸ਼ਾਮਲ_ਹੈ_Timing = ਸੱਚ;
        }
        iterateChildren(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    LifeVisitor(Ast_ਗੰਢ* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = ਨਵਾਂ Life_ਖੰਡ{ਖਾਲੀ_ਸੂਚਕ, m_statep};
            iterate(nodep);
            ਜੇ (m_lifep) VL_DO_CLEAR(ਮਿਟਾਓ m_lifep, m_lifep = ਖਾਲੀ_ਸੂਚਕ);
        }
    }
    ~LifeVisitor() override {
        ਜੇ (m_lifep) VL_DO_CLEAR(ਮਿਟਾਓ m_lifep, m_lifep = ਖਾਲੀ_ਸੂਚਕ);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

ਵਰਗ Life_ਸਿਖਰ_Visitor final : ਜਨਤਕ VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
ਨਿਜੀ:
    // STATE
    LifeState* ਅਚਲ m_statep;  // Current state

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        ਜੇ (nodep->ਇੰਦਰਾਜ_ਬਿੰਦੂ()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    Life_ਸਿਖਰ_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_ਸਿਖਰ_Visitor() override = ਮੂਲ;
};

//######################################################################
// Life class functions

ਖਾਲੀ V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_ਸਿਖਰ_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_ਰੁੱਖ();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_ਰੁੱਖ("life", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
