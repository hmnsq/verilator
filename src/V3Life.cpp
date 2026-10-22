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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Life.h"

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Structure for global state

తరగతి LifeState final {
    // NODE STATE
    //   See below
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // STATE
బహిరంగ:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = అప్రమేయ;
    ~LifeState() {
        V3Stats::కూడు_Stat_కూడిక("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::కూడు_Stat_కూడిక("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

తరగతి Life_Var_నమోదు final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_కణుపు_Stmt* m_assignp = శూన్యసూచిక;  // Last simple assignment
    AstConst* m_constp = శూన్యసూచిక;  // Known constant value
    తార్కిక m_isNew = నిజం;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    తార్కిక m_సమితి_Before_Use = అబద్ధం;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    తార్కిక m_ever_సమితి = అబద్ధం;

బహిరంగ:
    Life_Var_నమోదు() = అప్రమేయ;
    ~Life_Var_నమోదు() = అప్రమేయ;

    శూన్య init(తార్కిక సమితి_Before_Use) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = అబద్ధం;
        m_సమితి_Before_Use = సమితి_Before_Use;
    }
    string ascii() స్థిరరాశి {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        ఒకవేళ (m_isNew) os << " isNew";
        ఒకవేళ (m_సమితి_Before_Use) os << " setBeforeUse";
        ఒకవేళ (m_ever_సమితి) os << " everSet";
        ఒకవేళ (m_assignp) os << " assignp=" << Ast_కణుపు::కణుపు_Addr(m_assignp);
        ఒకవేళ (m_constp) os << " constp=" << Ast_కణుపు::కణుపు_Addr(m_constp);
        os << "]";
        తిరిగి os.str();
    }  // LCOV_EXCL_STOP

    శూన్య simple_కేటాయించు(Ast_కణుపు_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = శూన్యసూచిక;
        m_ever_సమితి = నిజం;
        ఒకవేళ (Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = VN_CAST(nodep, కేటాయించు)) {
            ఒకవేళ (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    శూన్య complex_కేటాయించు() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = శూన్యసూచిక;
        m_constp = శూన్యసూచిక;
        m_ever_సమితి = నిజం;
    }
    శూన్య consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = శూన్యసూచిక;
    }
    Ast_కణుపు_Stmt* assignp() స్థిరరాశి { తిరిగి m_assignp; }
    AstConst* constNodep() స్థిరరాశి { తిరిగి m_constp; }
    తార్కిక isNew() స్థిరరాశి { తిరిగి m_isNew; }
    తార్కిక సమితి_Before_Use() స్థిరరాశి { తిరిగి m_సమితి_Before_Use; }
    తార్కిక ever_సమితి() స్థిరరాశి { తిరిగి m_ever_సమితి; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

తరగతి Life_ఖండం final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, Life_Var_నమోదు> m_map;
    Life_ఖండం* స్థిరరాశి m_aboveLifep;  // Upper life, or nullptr
    LifeState* స్థిరరాశి m_statep;  // Current global state
    తార్కిక m_noopt = అబద్ధం;  // Opaque operation seen; invalidates the block above at exit
    తార్కిక m_replacedVref = అబద్ధం;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

బహిరంగ:
    Life_ఖండం(Life_ఖండం* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~Life_ఖండం() = అప్రమేయ;
    // METHODS
    శూన్య check_Remove_కేటాయించు(స్థిరరాశి AstVarScope* vscp, Life_Var_నమోదు& entr) {
        స్థిరరాశి AstVar* స్థిరరాశి varp = vscp->varp();
        // We don't optimize any public sigs
        ఒకవేళ (varp->is_Sig_ప్రజా()) తిరిగి;
        ఒకవేళ (varp->isReadByDpi()) తిరిగి;
        ఒకవేళ (varp->sensIfacep()) తిరిగి;
        // Check the var entry, and remove if appropriate
        Ast_కణుపు_Stmt* స్థిరరాశి oldassp = entr.assignp();
        ఒకవేళ (!oldassp) తిరిగి;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complex_కేటాయించు();
        oldassp->unlink_Fr_వెనుక();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.నెట్టు_Deletep(oldassp), oldassp);
    }
    శూన్య simple_కేటాయించు(AstVarScope* nodep, Ast_కణుపు_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        Life_Var_నమోదు& entr = m_map[nodep];
        ఒకవేళ (entr.isNew()) {
            entr.init(నిజం);
        } లేకపోతే {
            check_Remove_కేటాయించు(nodep, entr);
        }
        entr.simple_కేటాయించు(assp);
        // lifeDump();
    }
    శూన్య complex_కేటాయించు(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        Life_Var_నమోదు& entr = m_map[nodep];
        ఒకవేళ (entr.isNew()) entr.init(అబద్ధం);
        entr.complex_కేటాయించు();
    }
    శూన్య clearReplaced() { m_replacedVref = అబద్ధం; }
    తార్కిక replaced() స్థిరరాశి { తిరిగి m_replacedVref; }
    శూన్య varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        Life_Var_నమోదు& entr = m_map[nodep];
        ఒకవేళ (entr.isNew()) {
            entr.init(అబద్ధం);
        } లేకపోతే {
            ఒకవేళ (AstConst* స్థిరరాశి constp = entr.constNodep()) {
                ఒకవేళ (!varrefp->varp()->is_Sig_ప్రజా() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_వృక్షం(అబద్ధం));
                    m_replacedVref = నిజం;
                    VL_DO_DANGLING(varrefp->delete_వృక్షం(), varrefp);
                    ++m_statep->m_statAssnCon;
                    తిరిగి;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    శూన్య complex_కేటాయించు_కనుగొను(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        Life_Var_నమోదు& entr = m_map[nodep];
        ఒకవేళ (entr.isNew()) entr.init(అబద్ధం);
        entr.complex_కేటాయించు();
    }
    శూన్య consumed_కనుగొను(AstVarScope* nodep) {
        Life_Var_నమోదు& entr = m_map[nodep];
        ఒకవేళ (entr.isNew()) entr.init(అబద్ధం);
        entr.consumed();
    }
    శూన్య life_గమ్యం_Above() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        ఒకవేళ (m_noopt) m_aboveLifep->noopt();
        కొరకు (స్వయంచాలక& itr : m_map) {
            AstVarScope* స్థిరరాశి nodep = itr.మొదటి;
            m_aboveLifep->complex_కేటాయించు_కనుగొను(nodep);
            ఒకవేళ (itr.రెండవ.ever_సమితి()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complex_కేటాయించు_కనుగొను(nodep);
            } లేకపోతే {
                // Record consumption, so we don't eliminate earlier assignments
                consumed_కనుగొను(nodep);
            }
        }
    }
    శూన్య dualBranch(Life_ఖండం* life1p, Life_ఖండం* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_కణుపు::user1_Clear_వృక్షం();  // user1p() used on entire tree
        కొరకు (స్వయంచాలక& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            ఒకవేళ (itr.రెండవ.సమితి_Before_Use()) itr.మొదటి->user1(1);
        }
        కొరకు (స్వయంచాలక& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* స్థిరరాశి nodep = itr.మొదటి;
            ఒకవేళ (itr.రెండవ.సమితి_Before_Use() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                స్థిరరాశి స్వయంచాలక itab = m_map.కనుగొను(nodep);
                ఒకవేళ (itab != m_map.చివర()) check_Remove_కేటాయించు(nodep, itab->రెండవ);
            }
        }
        // this->lifeDump();
    }
    శూన్య noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = నిజం;
    }
    // DEBUG
    శూన్య lifeDump() {
        UINFO(5, "  LifeMap:");
        కొరకు (స్థిరరాశి స్వయంచాలక& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.రెండవ.సమితి_Before_Use() ? "[F]  " : "     ") << itr.మొదటి);
            ఒకవేళ (itr.రెండవ.assignp()) {  //
                UINFO(5, "       Ass: " << itr.రెండవ.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

తరగతి LifeVisitor final : బహిరంగ VNVisitor {
    // STATE
    LifeState* స్థిరరాశి m_statep;  // Current state
    తార్కిక m_కలిగియుంది_Timing = అబద్ధం;  // Statement contains timing control
    తార్కిక m_sideEffect = అబద్ధం;  // Side effects discovered in assign RHS
    తార్కిక m_noopt = అబద్ధం;  // Disable optimization of variables in this block
    తార్కిక m_tracingCall = అబద్ధం;  // Iterating into a CCall to a CFunc
    Life_ఖండం* m_lifep = శూన్యసూచిక;  // Current active lifetime map for current scope

    // METHODS
    శూన్య సమితి_Noopt(స్థిరరాశి అక్షరం* reasonp) {
        (శూన్య)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = నిజం;
        m_lifep->noopt();
    }

    శూన్య processAssignment(Ast_కణుపు_Stmt* nodep, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = అబద్ధం;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), కణుపు_సమాసం);
        ఒకవేళ (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        ఒకవేళ (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* స్థిరరాశి vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simple_కేటాయించు(vscp, nodep);
        } లేకపోతే {
            iterate_And_తదుపరి_Null(lhsp);
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* స్థిరరాశి vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        ఒకవేళ (nodep->access().isWriteOrRW()) {
            m_sideEffect = నిజం;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complex_కేటాయించు(vscp);
        } లేకపోతే {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ఒకవేళ (nodep->isTimingControl() || VN_IS(nodep, కేటాయించు_Force)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            సమితి_Noopt("timing|force");
            ఒకవేళ (nodep->isTimingControl()) m_కలిగియుంది_Timing = నిజం;
            iterateChildren(nodep);
            తిరిగి;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    శూన్య సందర్శించు(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override {
        // V3Life doesn't understand time sense
        ఒకవేళ (nodep->isTimingControl()) {
            // Don't optimize
            సమితి_Noopt("assigndly");
            m_కలిగియుంది_Timing = నిజం;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterate_And_తదుపరి_Null(nodep->condp());
        Life_ఖండం* స్థిరరాశి ifLifep = కొత్త Life_ఖండం{m_lifep, m_statep};
        Life_ఖండం* స్థిరరాశి elseLifep = కొత్త Life_ఖండం{m_lifep, m_statep};
        {
            VL_RESTORER(m_lifep);
            m_lifep = ifLifep;
            iterate_And_తదుపరి_Null(nodep->thensp());
        }
        {
            VL_RESTORER(m_lifep);
            m_lifep = elseLifep;
            iterate_And_తదుపరి_Null(nodep->elsesp());
        }
        UINFO(4, "   join " << nodep);
        // Find sets on both flows
        m_lifep->dualBranch(ifLifep, elseLifep);
        // For the next assignments, clear any variables that were read or written in the block
        ifLifep->life_గమ్యం_Above();
        elseLifep->life_గమ్యం_Above();
        VL_DO_DANGLING(తొలగించు ifLifep, ifLifep);
        VL_DO_DANGLING(తొలగించు elseLifep, elseLifep);
        UINFO(4, "   if-done " << nodep);
    }
    శూన్య సందర్శించు(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_కలిగియుంది_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = కొత్త Life_ఖండం{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = నిజం;
            iterate_And_తదుపరి_Null(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_గమ్యం_Above();
            VL_DO_DANGLING(తొలగించు m_lifep, m_lifep);
        }
        ఒకవేళ (m_కలిగియుంది_Timing) సమితి_Noopt("timing");
    }
    శూన్య సందర్శించు(Ast_Jump_ఖండం* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_కలిగియుంది_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = కొత్త Life_ఖండం{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = నిజం;
            iterate_And_తదుపరి_Null(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_గమ్యం_Above();
            VL_DO_DANGLING(తొలగించు m_lifep, m_lifep);
        }
        ఒకవేళ (m_కలిగియుంది_Timing) సమితి_Noopt("timing");
    }
    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        ఒకవేళ (nodep->funcp()->నమోదు_బిందువు()) {
            సమితి_Noopt("ccall");
        } లేకపోతే {
            m_tracingCall = నిజం;
            iterate(nodep->funcp());
        }
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        ఒకవేళ (!m_tracingCall && !nodep->నమోదు_బిందువు()) తిరిగి;
        m_tracingCall = అబద్ధం;
        ఒకవేళ (nodep->recursive()) సమితి_Noopt("recursive");
        ఒకవేళ (nodep->noLife()) సమితి_Noopt("nolife");
        ఒకవేళ (nodep->dpiImportPrototype() && !nodep->dpi_శుద్ధ()) {
            m_sideEffect = నిజం;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCExprUser* nodep) override {
        m_sideEffect = నిజం;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCExpr* nodep) override {
        m_sideEffect = నిజం;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstVar*) override {}  // Don't want varrefs under it
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            సమితి_Noopt("timing");
            m_కలిగియుంది_Timing = నిజం;
        }
        iterateChildren(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    LifeVisitor(Ast_కణుపు* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = కొత్త Life_ఖండం{శూన్యసూచిక, m_statep};
            iterate(nodep);
            ఒకవేళ (m_lifep) VL_DO_CLEAR(తొలగించు m_lifep, m_lifep = శూన్యసూచిక);
        }
    }
    ~LifeVisitor() override {
        ఒకవేళ (m_lifep) VL_DO_CLEAR(తొలగించు m_lifep, m_lifep = శూన్యసూచిక);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

తరగతి Life_శిఖరం_Visitor final : బహిరంగ VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
అంతరంగ:
    // STATE
    LifeState* స్థిరరాశి m_statep;  // Current state

    // VISITORS
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        ఒకవేళ (nodep->నమోదు_బిందువు()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    శూన్య సందర్శించు(AstVar*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు_Stmt*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు_సమాసం*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    Life_శిఖరం_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_శిఖరం_Visitor() override = అప్రమేయ;
};

//######################################################################
// Life class functions

శూన్య V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_శిఖరం_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_వృక్షం();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_వృక్షం("life", 0, dump_వృక్షం_Either_Level() >= 3);
}
