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
    स्थिरांक VNUser1InUse m_inuser1;

    // STATE
सार्वजनिक:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = पूर्वनिर्धारित;
    ~LifeState() {
        V3Stats::addStatSum("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::addStatSum("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

वर्ग LifeVarEntry final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_गाठ_Stmt* m_assignp = nullptr;  // Last simple assignment
    AstConst* m_constp = nullptr;  // Known constant value
    तार्किक m_isNew = सत्य;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    तार्किक m_setBeforeUse = असत्य;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    तार्किक m_everSet = असत्य;

सार्वजनिक:
    LifeVarEntry() = पूर्वनिर्धारित;
    ~LifeVarEntry() = पूर्वनिर्धारित;

    शून्य init(तार्किक setBeforeUse) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = असत्य;
        m_setBeforeUse = setBeforeUse;
    }
    string ascii() स्थिरांक {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        जर (m_isNew) os << " isNew";
        जर (m_setBeforeUse) os << " setBeforeUse";
        जर (m_everSet) os << " everSet";
        जर (m_assignp) os << " assignp=" << Ast_गाठ::गाठ_Addr(m_assignp);
        जर (m_constp) os << " constp=" << Ast_गाठ::गाठ_Addr(m_constp);
        os << "]";
        परत os.str();
    }  // LCOV_EXCL_STOP

    शून्य simpleAssign(Ast_गाठ_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = nullptr;
        m_everSet = सत्य;
        जर (Ast_गाठ_Assign* स्थिरांक assp = VN_CAST(nodep, Assign)) {
            जर (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    शून्य complexAssign() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
        m_constp = nullptr;
        m_everSet = सत्य;
    }
    शून्य consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
    }
    Ast_गाठ_Stmt* assignp() स्थिरांक { परत m_assignp; }
    AstConst* constNodep() स्थिरांक { परत m_constp; }
    तार्किक isNew() स्थिरांक { परत m_isNew; }
    तार्किक setBeforeUse() स्थिरांक { परत m_setBeforeUse; }
    तार्किक everSet() स्थिरांक { परत m_everSet; }
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
    LifeBlock* स्थिरांक m_aboveLifep;  // Upper life, or nullptr
    LifeState* स्थिरांक m_statep;  // Current global state
    तार्किक m_noopt = असत्य;  // Opaque operation seen; invalidates the block above at exit
    तार्किक m_replacedVref = असत्य;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

सार्वजनिक:
    LifeBlock(LifeBlock* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~LifeBlock() = पूर्वनिर्धारित;
    // METHODS
    शून्य checkRemoveAssign(स्थिरांक AstVarScope* vscp, LifeVarEntry& entr) {
        स्थिरांक AstVar* स्थिरांक varp = vscp->varp();
        // We don't optimize any public sigs
        जर (varp->isSigPublic()) परत;
        जर (varp->isReadByDpi()) परत;
        जर (varp->sensIfacep()) परत;
        // Check the var entry, and remove if appropriate
        Ast_गाठ_Stmt* स्थिरांक oldassp = entr.assignp();
        जर (!oldassp) परत;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complexAssign();
        oldassp->unlinkFrBack();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.ढकला_Deletep(oldassp), oldassp);
    }
    शून्य simpleAssign(AstVarScope* nodep, Ast_गाठ_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        LifeVarEntry& entr = m_map[nodep];
        जर (entr.isNew()) {
            entr.init(सत्य);
        } अन्यथा {
            checkRemoveAssign(nodep, entr);
        }
        entr.simpleAssign(assp);
        // lifeDump();
    }
    शून्य complexAssign(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        जर (entr.isNew()) entr.init(असत्य);
        entr.complexAssign();
    }
    शून्य clearReplaced() { m_replacedVref = असत्य; }
    तार्किक replaced() स्थिरांक { परत m_replacedVref; }
    शून्य varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        LifeVarEntry& entr = m_map[nodep];
        जर (entr.isNew()) {
            entr.init(असत्य);
        } अन्यथा {
            जर (AstConst* स्थिरांक constp = entr.constNodep()) {
                जर (!varrefp->varp()->isSigPublic() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_वृक्ष(असत्य));
                    m_replacedVref = सत्य;
                    VL_DO_DANGLING(varrefp->delete_वृक्ष(), varrefp);
                    ++m_statep->m_statAssnCon;
                    परत;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    शून्य complexAssignFind(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        जर (entr.isNew()) entr.init(असत्य);
        entr.complexAssign();
    }
    शून्य consumedFind(AstVarScope* nodep) {
        LifeVarEntry& entr = m_map[nodep];
        जर (entr.isNew()) entr.init(असत्य);
        entr.consumed();
    }
    शून्य lifeToAbove() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        जर (m_noopt) m_aboveLifep->noopt();
        साठी (स्वतः& itr : m_map) {
            AstVarScope* स्थिरांक nodep = itr.first;
            m_aboveLifep->complexAssignFind(nodep);
            जर (itr.second.everSet()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complexAssignFind(nodep);
            } अन्यथा {
                // Record consumption, so we don't eliminate earlier assignments
                consumedFind(nodep);
            }
        }
    }
    शून्य dualBranch(LifeBlock* life1p, LifeBlock* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_गाठ::user1_Clear_वृक्ष();  // user1p() used on entire tree
        साठी (स्वतः& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            जर (itr.second.setBeforeUse()) itr.first->user1(1);
        }
        साठी (स्वतः& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* स्थिरांक nodep = itr.first;
            जर (itr.second.setBeforeUse() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                स्थिरांक स्वतः itab = m_map.find(nodep);
                जर (itab != m_map.end()) checkRemoveAssign(nodep, itab->second);
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
        साठी (स्थिरांक स्वतः& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.second.setBeforeUse() ? "[F]  " : "     ") << itr.first);
            जर (itr.second.assignp()) {  //
                UINFO(5, "       Ass: " << itr.second.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

वर्ग LifeVisitor final : सार्वजनिक VNVisitor {
    // STATE
    LifeState* स्थिरांक m_statep;  // Current state
    तार्किक m_containsTiming = असत्य;  // Statement contains timing control
    तार्किक m_sideEffect = असत्य;  // Side effects discovered in assign RHS
    तार्किक m_noopt = असत्य;  // Disable optimization of variables in this block
    तार्किक m_tracingCall = असत्य;  // Iterating into a CCall to a CFunc
    LifeBlock* m_lifep = nullptr;  // Current active lifetime map for current scope

    // METHODS
    शून्य setNoopt(स्थिरांक अक्षर* reasonp) {
        (शून्य)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = सत्य;
        m_lifep->noopt();
    }

    शून्य processAssignment(Ast_गाठ_Stmt* nodep, Ast_गाठ_अभिव्यक्ती* lhsp, Ast_गाठ_अभिव्यक्ती* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = असत्य;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), गाठ_अभिव्यक्ती);
        जर (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        जर (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* स्थिरांक vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simpleAssign(vscp, nodep);
        } अन्यथा {
            iterateAndNextNull(lhsp);
        }
    }

    // VISITORS
    शून्य visit(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* स्थिरांक vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        जर (nodep->access().isWriteOrRW()) {
            m_sideEffect = सत्य;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complexAssign(vscp);
        } अन्यथा {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        जर (nodep->isTimingControl() || VN_IS(nodep, AssignForce)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            setNoopt("timing|force");
            जर (nodep->isTimingControl()) m_containsTiming = सत्य;
            iterateChildren(nodep);
            परत;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    शून्य visit(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    शून्य visit(AstAssignDly* nodep) override {
        // V3Life doesn't understand time sense
        जर (nodep->isTimingControl()) {
            // Don't optimize
            setNoopt("assigndly");
            m_containsTiming = सत्य;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    शून्य visit(Ast_गाठ_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterateAndNextNull(nodep->condp());
        LifeBlock* स्थिरांक ifLifep = नवीन LifeBlock{m_lifep, m_statep};
        LifeBlock* स्थिरांक elseLifep = नवीन LifeBlock{m_lifep, m_statep};
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
            m_lifep = नवीन LifeBlock{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = सत्य;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        जर (m_containsTiming) setNoopt("timing");
    }
    शून्य visit(AstJumpBlock* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = नवीन LifeBlock{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = सत्य;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        जर (m_containsTiming) setNoopt("timing");
    }
    शून्य visit(Ast_गाठ_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        जर (nodep->funcp()->entryPoint()) {
            setNoopt("ccall");
        } अन्यथा {
            m_tracingCall = सत्य;
            iterate(nodep->funcp());
        }
    }
    शून्य visit(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        जर (!m_tracingCall && !nodep->entryPoint()) परत;
        m_tracingCall = असत्य;
        जर (nodep->recursive()) setNoopt("recursive");
        जर (nodep->noLife()) setNoopt("nolife");
        जर (nodep->dpiImportPrototype() && !nodep->dpiPure()) {
            m_sideEffect = सत्य;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstCExprUser* nodep) override {
        m_sideEffect = सत्य;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    शून्य visit(AstCExpr* nodep) override {
        m_sideEffect = सत्य;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    शून्य visit(AstVar*) override {}  // Don't want varrefs under it
    शून्य visit(Ast_गाठ* nodep) override {
        जर (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            setNoopt("timing");
            m_containsTiming = सत्य;
        }
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    LifeVisitor(Ast_गाठ* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = नवीन LifeBlock{nullptr, m_statep};
            iterate(nodep);
            जर (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
        }
    }
    ~LifeVisitor() override {
        जर (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

वर्ग Life_शीर्ष_Visitor final : सार्वजनिक VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
खाजगी:
    // STATE
    LifeState* स्थिरांक m_statep;  // Current state

    // VISITORS
    शून्य visit(AstCFunc* nodep) override {
        जर (nodep->entryPoint()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    शून्य visit(Ast_गाठ_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    शून्य visit(AstVar*) override {}  // Accelerate
    शून्य visit(Ast_गाठ_Stmt*) override {}  // Accelerate
    शून्य visit(Ast_गाठ_अभिव्यक्ती*) override {}  // Accelerate
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    Life_शीर्ष_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_शीर्ष_Visitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Life class functions

शून्य V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_शीर्ष_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_वृक्ष();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_वृक्ष("life", 0, dump_वृक्ष_Either_Level() >= 3);
}
