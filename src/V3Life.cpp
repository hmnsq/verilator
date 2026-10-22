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
    ध्रुव VNUser1InUse m_inuser1;

    // STATE
सार्वजनिक:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = स्वभाव;
    ~LifeState() {
        V3Stats::addStatSum("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::addStatSum("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

वर्ग LifeVarEntry final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_ग्रन्थिः_Stmt* m_assignp = nullptr;  // Last simple assignment
    AstConst* m_constp = nullptr;  // Known constant value
    तार्किक m_isNew = सच्च;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    तार्किक m_setBeforeUse = मिच्छ;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    तार्किक m_everSet = मिच्छ;

सार्वजनिक:
    LifeVarEntry() = स्वभाव;
    ~LifeVarEntry() = स्वभाव;

    शून्य init(तार्किक setBeforeUse) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = मिच्छ;
        m_setBeforeUse = setBeforeUse;
    }
    string ascii() ध्रुव {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        जइ (m_isNew) os << " isNew";
        जइ (m_setBeforeUse) os << " setBeforeUse";
        जइ (m_everSet) os << " everSet";
        जइ (m_assignp) os << " assignp=" << Ast_ग्रन्थिः::ग्रन्थिः_Addr(m_assignp);
        जइ (m_constp) os << " constp=" << Ast_ग्रन्थिः::ग्रन्थिः_Addr(m_constp);
        os << "]";
        प्रत्यागम os.str();
    }  // LCOV_EXCL_STOP

    शून्य simpleAssign(Ast_ग्रन्थिः_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = nullptr;
        m_everSet = सच्च;
        जइ (Ast_ग्रन्थिः_Assign* ध्रुव assp = VN_CAST(nodep, Assign)) {
            जइ (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    शून्य complexAssign() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
        m_constp = nullptr;
        m_everSet = सच्च;
    }
    शून्य consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
    }
    Ast_ग्रन्थिः_Stmt* assignp() ध्रुव { प्रत्यागम m_assignp; }
    AstConst* constNodep() ध्रुव { प्रत्यागम m_constp; }
    तार्किक isNew() ध्रुव { प्रत्यागम m_isNew; }
    तार्किक setBeforeUse() ध्रुव { प्रत्यागम m_setBeforeUse; }
    तार्किक everSet() ध्रुव { प्रत्यागम m_everSet; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

वर्ग LifeBlock final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, LifeVarEntry> m_map;
    LifeBlock* ध्रुव m_aboveLifep;  // Upper life, or nullptr
    LifeState* ध्रुव m_statep;  // Current global state
    तार्किक m_noopt = मिच्छ;  // Opaque operation seen; invalidates the block above at exit
    तार्किक m_replacedVref = मिच्छ;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

सार्वजनिक:
    LifeBlock(LifeBlock* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~LifeBlock() = स्वभाव;
    // METHODS
    शून्य checkRemoveAssign(ध्रुव AstVarScope* vscp, LifeVarEntry& entr) {
        ध्रुव AstVar* ध्रुव varp = vscp->varp();
        // We don't optimize any public sigs
        जइ (varp->isSigPublic()) प्रत्यागम;
        जइ (varp->isReadByDpi()) प्रत्यागम;
        जइ (varp->sensIfacep()) प्रत्यागम;
        // Check the var entry, and remove if appropriate
        Ast_ग्रन्थिः_Stmt* ध्रुव oldassp = entr.assignp();
        जइ (!oldassp) प्रत्यागम;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complexAssign();
        oldassp->unlinkFrBack();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.प्रक्षिप_Deletep(oldassp), oldassp);
    }
    शून्य simpleAssign(AstVarScope* nodep, Ast_ग्रन्थिः_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        LifeVarEntry& entr = m_map[nodep];
        जइ (entr.isNew()) {
            entr.init(सच्च);
        } अण्णहा {
            checkRemoveAssign(nodep, entr);
        }
        entr.simpleAssign(assp);
        // lifeDump();
    }
    शून्य complexAssign(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        जइ (entr.isNew()) entr.init(मिच्छ);
        entr.complexAssign();
    }
    शून्य clearReplaced() { m_replacedVref = मिच्छ; }
    तार्किक replaced() ध्रुव { प्रत्यागम m_replacedVref; }
    शून्य varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        LifeVarEntry& entr = m_map[nodep];
        जइ (entr.isNew()) {
            entr.init(मिच्छ);
        } अण्णहा {
            जइ (AstConst* ध्रुव constp = entr.constNodep()) {
                जइ (!varrefp->varp()->isSigPublic() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_वृक्षः(मिच्छ));
                    m_replacedVref = सच्च;
                    VL_DO_DANGLING(varrefp->delete_वृक्षः(), varrefp);
                    ++m_statep->m_statAssnCon;
                    प्रत्यागम;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    शून्य complexAssignFind(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        जइ (entr.isNew()) entr.init(मिच्छ);
        entr.complexAssign();
    }
    शून्य consumedFind(AstVarScope* nodep) {
        LifeVarEntry& entr = m_map[nodep];
        जइ (entr.isNew()) entr.init(मिच्छ);
        entr.consumed();
    }
    शून्य lifeToAbove() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        जइ (m_noopt) m_aboveLifep->noopt();
        प्रति (auto& itr : m_map) {
            AstVarScope* ध्रुव nodep = itr.first;
            m_aboveLifep->complexAssignFind(nodep);
            जइ (itr.second.everSet()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complexAssignFind(nodep);
            } अण्णहा {
                // Record consumption, so we don't eliminate earlier assignments
                consumedFind(nodep);
            }
        }
    }
    शून्य dualBranch(LifeBlock* life1p, LifeBlock* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_ग्रन्थिः::user1_Clear_वृक्षः();  // user1p() used on entire tree
        प्रति (auto& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            जइ (itr.second.setBeforeUse()) itr.first->user1(1);
        }
        प्रति (auto& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* ध्रुव nodep = itr.first;
            जइ (itr.second.setBeforeUse() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                ध्रुव auto itab = m_map.find(nodep);
                जइ (itab != m_map.end()) checkRemoveAssign(nodep, itab->second);
            }
        }
        // this->lifeDump();
    }
    शून्य noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = सच्च;
    }
    // DEBUG
    शून्य lifeDump() {
        UINFO(5, "  LifeMap:");
        प्रति (ध्रुव auto& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.second.setBeforeUse() ? "[F]  " : "     ") << itr.first);
            जइ (itr.second.assignp()) {  //
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
    तार्किक m_containsTiming = मिच्छ;  // Statement contains timing control
    तार्किक m_sideEffect = मिच्छ;  // Side effects discovered in assign RHS
    तार्किक m_noopt = मिच्छ;  // Disable optimization of variables in this block
    तार्किक m_tracingCall = मिच्छ;  // Iterating into a CCall to a CFunc
    LifeBlock* m_lifep = nullptr;  // Current active lifetime map for current scope

    // METHODS
    शून्य setNoopt(ध्रुव वर्ण* reasonp) {
        (शून्य)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = सच्च;
        m_lifep->noopt();
    }

    शून्य processAssignment(Ast_ग्रन्थिः_Stmt* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = मिच्छ;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), ग्रन्थिः_व्यञ्जकम्);
        जइ (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        जइ (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* ध्रुव vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simpleAssign(vscp, nodep);
        } अण्णहा {
            iterateAndNextNull(lhsp);
        }
    }

    // VISITORS
    शून्य visit(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* ध्रुव vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        जइ (nodep->access().isWriteOrRW()) {
            m_sideEffect = सच्च;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complexAssign(vscp);
        } अण्णहा {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Assign* nodep) override {
        जइ (nodep->isTimingControl() || VN_IS(nodep, AssignForce)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            setNoopt("timing|force");
            जइ (nodep->isTimingControl()) m_containsTiming = सच्च;
            iterateChildren(nodep);
            प्रत्यागम;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    शून्य visit(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    शून्य visit(AstAssignDly* nodep) override {
        // V3Life doesn't understand time sense
        जइ (nodep->isTimingControl()) {
            // Don't optimize
            setNoopt("assigndly");
            m_containsTiming = सच्च;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    शून्य visit(Ast_ग्रन्थिः_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterateAndNextNull(nodep->condp());
        LifeBlock* ध्रुव ifLifep = नूतन LifeBlock{m_lifep, m_statep};
        LifeBlock* ध्रुव elseLifep = नूतन LifeBlock{m_lifep, m_statep};
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
    शून्य visit(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = नूतन LifeBlock{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = सच्च;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        जइ (m_containsTiming) setNoopt("timing");
    }
    शून्य visit(AstJumpBlock* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = नूतन LifeBlock{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = सच्च;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        जइ (m_containsTiming) setNoopt("timing");
    }
    शून्य visit(Ast_ग्रन्थिः_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        जइ (nodep->funcp()->entryPoint()) {
            setNoopt("ccall");
        } अण्णहा {
            m_tracingCall = सच्च;
            iterate(nodep->funcp());
        }
    }
    शून्य visit(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        जइ (!m_tracingCall && !nodep->entryPoint()) प्रत्यागम;
        m_tracingCall = मिच्छ;
        जइ (nodep->recursive()) setNoopt("recursive");
        जइ (nodep->noLife()) setNoopt("nolife");
        जइ (nodep->dpiImportPrototype() && !nodep->dpiPure()) {
            m_sideEffect = सच्च;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstCExprUser* nodep) override {
        m_sideEffect = सच्च;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    शून्य visit(AstCExpr* nodep) override {
        m_sideEffect = सच्च;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    शून्य visit(AstVar*) override {}  // Don't want varrefs under it
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        जइ (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            setNoopt("timing");
            m_containsTiming = सच्च;
        }
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    LifeVisitor(Ast_ग्रन्थिः* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = नूतन LifeBlock{nullptr, m_statep};
            iterate(nodep);
            जइ (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
        }
    }
    ~LifeVisitor() override {
        जइ (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
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
    शून्य visit(AstCFunc* nodep) override {
        जइ (nodep->entryPoint()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    शून्य visit(AstVar*) override {}  // Accelerate
    शून्य visit(Ast_ग्रन्थिः_Stmt*) override {}  // Accelerate
    शून्य visit(Ast_ग्रन्थिः_व्यञ्जकम्*) override {}  // Accelerate
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

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
