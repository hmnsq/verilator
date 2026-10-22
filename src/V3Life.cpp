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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Life.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Structure for global state

वर्ग LifeState final {
    // NODE STATE
    //   See below
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // STATE
सार्वजनिक:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = स्वभाव;
    ~LifeState() {
        V3Stats::सङ्कलय_Stat_योग("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::सङ्कलय_Stat_योग("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

वर्ग Life_Var_प्रविष्टिः final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_ग्रन्थिः_Stmt* m_assignp = शून्यसूचकः;  // Last simple assignment
    AstConst* m_constp = शून्यसूचकः;  // Known constant value
    तार्किक m_isNew = सत्य;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    तार्किक m_समुच्चयः_Before_Use = असत्य;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    तार्किक m_ever_समुच्चयः = असत्य;

सार्वजनिक:
    Life_Var_प्रविष्टिः() = स्वभाव;
    ~Life_Var_प्रविष्टिः() = स्वभाव;

    शून्य init(तार्किक समुच्चयः_Before_Use) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = असत्य;
        m_समुच्चयः_Before_Use = समुच्चयः_Before_Use;
    }
    string ascii() ध्रुव {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        यदि (m_isNew) os << " isNew";
        यदि (m_समुच्चयः_Before_Use) os << " setBeforeUse";
        यदि (m_ever_समुच्चयः) os << " everSet";
        यदि (m_assignp) os << " assignp=" << Ast_ग्रन्थिः::ग्रन्थिः_Addr(m_assignp);
        यदि (m_constp) os << " constp=" << Ast_ग्रन्थिः::ग्रन्थिः_Addr(m_constp);
        os << "]";
        प्रत्यागम os.str();
    }  // LCOV_EXCL_STOP

    शून्य simple_नियोजय(Ast_ग्रन्थिः_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = शून्यसूचकः;
        m_ever_समुच्चयः = सत्य;
        यदि (Ast_ग्रन्थिः_नियोजय* ध्रुव assp = VN_CAST(nodep, नियोजय)) {
            यदि (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    शून्य complex_नियोजय() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = शून्यसूचकः;
        m_constp = शून्यसूचकः;
        m_ever_समुच्चयः = सत्य;
    }
    शून्य consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = शून्यसूचकः;
    }
    Ast_ग्रन्थिः_Stmt* assignp() ध्रुव { प्रत्यागम m_assignp; }
    AstConst* constNodep() ध्रुव { प्रत्यागम m_constp; }
    तार्किक isNew() ध्रुव { प्रत्यागम m_isNew; }
    तार्किक समुच्चयः_Before_Use() ध्रुव { प्रत्यागम m_समुच्चयः_Before_Use; }
    तार्किक ever_समुच्चयः() ध्रुव { प्रत्यागम m_ever_समुच्चयः; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

वर्ग Life_खण्डम् final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, Life_Var_प्रविष्टिः> m_map;
    Life_खण्डम्* ध्रुव m_aboveLifep;  // Upper life, or nullptr
    LifeState* ध्रुव m_statep;  // Current global state
    तार्किक m_noopt = असत्य;  // Opaque operation seen; invalidates the block above at exit
    तार्किक m_replacedVref = असत्य;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

सार्वजनिक:
    Life_खण्डम्(Life_खण्डम्* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~Life_खण्डम्() = स्वभाव;
    // METHODS
    शून्य check_Remove_नियोजय(ध्रुव AstVarScope* vscp, Life_Var_प्रविष्टिः& entr) {
        ध्रुव AstVar* ध्रुव varp = vscp->varp();
        // We don't optimize any public sigs
        यदि (varp->is_Sig_लौकिकम्()) प्रत्यागम;
        यदि (varp->isReadByDpi()) प्रत्यागम;
        यदि (varp->sensIfacep()) प्रत्यागम;
        // Check the var entry, and remove if appropriate
        Ast_ग्रन्थिः_Stmt* ध्रुव oldassp = entr.assignp();
        यदि (!oldassp) प्रत्यागम;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complex_नियोजय();
        oldassp->unlink_Fr_पृष्ठभागः();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.प्रक्षिप_Deletep(oldassp), oldassp);
    }
    शून्य simple_नियोजय(AstVarScope* nodep, Ast_ग्रन्थिः_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        Life_Var_प्रविष्टिः& entr = m_map[nodep];
        यदि (entr.isNew()) {
            entr.init(सत्य);
        } अन्यथा {
            check_Remove_नियोजय(nodep, entr);
        }
        entr.simple_नियोजय(assp);
        // lifeDump();
    }
    शून्य complex_नियोजय(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        Life_Var_प्रविष्टिः& entr = m_map[nodep];
        यदि (entr.isNew()) entr.init(असत्य);
        entr.complex_नियोजय();
    }
    शून्य clearReplaced() { m_replacedVref = असत्य; }
    तार्किक replaced() ध्रुव { प्रत्यागम m_replacedVref; }
    शून्य varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        Life_Var_प्रविष्टिः& entr = m_map[nodep];
        यदि (entr.isNew()) {
            entr.init(असत्य);
        } अन्यथा {
            यदि (AstConst* ध्रुव constp = entr.constNodep()) {
                यदि (!varrefp->varp()->is_Sig_लौकिकम्() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_वृक्षः(असत्य));
                    m_replacedVref = सत्य;
                    VL_DO_DANGLING(varrefp->delete_वृक्षः(), varrefp);
                    ++m_statep->m_statAssnCon;
                    प्रत्यागम;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    शून्य complex_नियोजय_Find(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        Life_Var_प्रविष्टिः& entr = m_map[nodep];
        यदि (entr.isNew()) entr.init(असत्य);
        entr.complex_नियोजय();
    }
    शून्य consumedFind(AstVarScope* nodep) {
        Life_Var_प्रविष्टिः& entr = m_map[nodep];
        यदि (entr.isNew()) entr.init(असत्य);
        entr.consumed();
    }
    शून्य life_गन्तव्यम्_Above() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        यदि (m_noopt) m_aboveLifep->noopt();
        प्रति (स्वचालित& itr : m_map) {
            AstVarScope* ध्रुव nodep = itr.first;
            m_aboveLifep->complex_नियोजय_Find(nodep);
            यदि (itr.second.ever_समुच्चयः()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complex_नियोजय_Find(nodep);
            } अन्यथा {
                // Record consumption, so we don't eliminate earlier assignments
                consumedFind(nodep);
            }
        }
    }
    शून्य dualBranch(Life_खण्डम्* life1p, Life_खण्डम्* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_ग्रन्थिः::user1_Clear_वृक्षः();  // user1p() used on entire tree
        प्रति (स्वचालित& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            यदि (itr.second.समुच्चयः_Before_Use()) itr.first->user1(1);
        }
        प्रति (स्वचालित& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* ध्रुव nodep = itr.first;
            यदि (itr.second.समुच्चयः_Before_Use() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                ध्रुव स्वचालित itab = m_map.find(nodep);
                यदि (itab != m_map.अन्तः_2()) check_Remove_नियोजय(nodep, itab->second);
            }
        }
        // this->lifeDump();
    }
    शून्य noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = सत्य;
    }
    // DEBUG
    शून्य lifeDump() {
        UINFO(5, "  LifeMap:");
        प्रति (ध्रुव स्वचालित& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.second.समुच्चयः_Before_Use() ? "[F]  " : "     ") << itr.first);
            यदि (itr.second.assignp()) {  //
                UINFO(5, "       Ass: " << itr.second.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

वर्ग LifeVisitor final : सार्वजनिक VNVisitor {
    // STATE
    LifeState* ध्रुव m_statep;  // Current state
    तार्किक m_अन्तर्भवति_Timing = असत्य;  // Statement contains timing control
    तार्किक m_sideEffect = असत्य;  // Side effects discovered in assign RHS
    तार्किक m_noopt = असत्य;  // Disable optimization of variables in this block
    तार्किक m_tracingCall = असत्य;  // Iterating into a CCall to a CFunc
    Life_खण्डम्* m_lifep = शून्यसूचकः;  // Current active lifetime map for current scope

    // METHODS
    शून्य समुच्चयः_Noopt(ध्रुव वर्ण* reasonp) {
        (शून्य)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = सत्य;
        m_lifep->noopt();
    }

    शून्य processAssignment(Ast_ग्रन्थिः_Stmt* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = असत्य;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), ग्रन्थिः_व्यञ्जकम्);
        यदि (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        यदि (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* ध्रुव vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simple_नियोजय(vscp, nodep);
        } अन्यथा {
            iterate_And_अग्र_Null(lhsp);
        }
    }

    // VISITORS
    शून्य दर्शय(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* ध्रुव vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        यदि (nodep->access().isWriteOrRW()) {
            m_sideEffect = सत्य;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complex_नियोजय(vscp);
        } अन्यथा {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        यदि (nodep->isTimingControl() || VN_IS(nodep, नियोजय_Force)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            समुच्चयः_Noopt("timing|force");
            यदि (nodep->isTimingControl()) m_अन्तर्भवति_Timing = सत्य;
            iterateChildren(nodep);
            प्रत्यागम;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    शून्य दर्शय(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    शून्य दर्शय(Ast_नियोजय_Dly* nodep) override {
        // V3Life doesn't understand time sense
        यदि (nodep->isTimingControl()) {
            // Don't optimize
            समुच्चयः_Noopt("assigndly");
            m_अन्तर्भवति_Timing = सत्य;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterate_And_अग्र_Null(nodep->condp());
        Life_खण्डम्* ध्रुव ifLifep = नूतन Life_खण्डम्{m_lifep, m_statep};
        Life_खण्डम्* ध्रुव elseLifep = नूतन Life_खण्डम्{m_lifep, m_statep};
        {
            VL_RESTORER(m_lifep);
            m_lifep = ifLifep;
            iterate_And_अग्र_Null(nodep->thensp());
        }
        {
            VL_RESTORER(m_lifep);
            m_lifep = elseLifep;
            iterate_And_अग्र_Null(nodep->elsesp());
        }
        UINFO(4, "   join " << nodep);
        // Find sets on both flows
        m_lifep->dualBranch(ifLifep, elseLifep);
        // For the next assignments, clear any variables that were read or written in the block
        ifLifep->life_गन्तव्यम्_Above();
        elseLifep->life_गन्तव्यम्_Above();
        VL_DO_DANGLING(विलोपय ifLifep, ifLifep);
        VL_DO_DANGLING(विलोपय elseLifep, elseLifep);
        UINFO(4, "   if-done " << nodep);
    }
    शून्य दर्शय(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_अन्तर्भवति_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = नूतन Life_खण्डम्{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = सत्य;
            iterate_And_अग्र_Null(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_गन्तव्यम्_Above();
            VL_DO_DANGLING(विलोपय m_lifep, m_lifep);
        }
        यदि (m_अन्तर्भवति_Timing) समुच्चयः_Noopt("timing");
    }
    शून्य दर्शय(Ast_Jump_खण्डम्* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_अन्तर्भवति_Timing);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = नूतन Life_खण्डम्{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = सत्य;
            iterate_And_अग्र_Null(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->life_गन्तव्यम्_Above();
            VL_DO_DANGLING(विलोपय m_lifep, m_lifep);
        }
        यदि (m_अन्तर्भवति_Timing) समुच्चयः_Noopt("timing");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        यदि (nodep->funcp()->प्रविष्टिः_Point()) {
            समुच्चयः_Noopt("ccall");
        } अन्यथा {
            m_tracingCall = सत्य;
            iterate(nodep->funcp());
        }
    }
    शून्य दर्शय(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        यदि (!m_tracingCall && !nodep->प्रविष्टिः_Point()) प्रत्यागम;
        m_tracingCall = असत्य;
        यदि (nodep->recursive()) समुच्चयः_Noopt("recursive");
        यदि (nodep->noLife()) समुच्चयः_Noopt("nolife");
        यदि (nodep->dpiImportPrototype() && !nodep->dpi_विशुद्धम्()) {
            m_sideEffect = सत्य;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstCExprUser* nodep) override {
        m_sideEffect = सत्य;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstCExpr* nodep) override {
        m_sideEffect = सत्य;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstVar*) override {}  // Don't want varrefs under it
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        यदि (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            समुच्चयः_Noopt("timing");
            m_अन्तर्भवति_Timing = सत्य;
        }
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    LifeVisitor(Ast_ग्रन्थिः* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = नूतन Life_खण्डम्{शून्यसूचकः, m_statep};
            iterate(nodep);
            यदि (m_lifep) VL_DO_CLEAR(विलोपय m_lifep, m_lifep = शून्यसूचकः);
        }
    }
    ~LifeVisitor() override {
        यदि (m_lifep) VL_DO_CLEAR(विलोपय m_lifep, m_lifep = शून्यसूचकः);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

वर्ग Life_शिखरम्_Visitor final : सार्वजनिक VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
गुप्त:
    // STATE
    LifeState* ध्रुव m_statep;  // Current state

    // VISITORS
    शून्य दर्शय(AstCFunc* nodep) override {
        यदि (nodep->प्रविष्टिः_Point()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    शून्य दर्शय(AstVar*) override {}  // Accelerate
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt*) override {}  // Accelerate
    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्*) override {}  // Accelerate
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    Life_शिखरम्_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_शिखरम्_Visitor() override = स्वभाव;
};

//######################################################################
// Life class functions

शून्य V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_शिखरम्_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_वृक्षः();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_वृक्षः("life", 0, dump_वृक्षः_Either_Level() >= 3);
}
