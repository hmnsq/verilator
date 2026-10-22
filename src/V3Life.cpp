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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Life.h"

#समावेश "V3Const.h"
#समावेश "V3Stats.h"

#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Structure for global state

श्रेणी LifeState final {
    // NODE STATE
    //   See below
    स्थिर VNUser1InUse m_inuser1;

    // STATE
खुला:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = शेष;
    ~LifeState() {
        V3Stats::add_Stat_योग("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::add_Stat_योग("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

श्रेणी LifeVarEntry final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_गाँठ_Stmt* m_assignp = nullptr;  // Last simple assignment
    AstConst* m_constp = nullptr;  // Known constant value
    bool m_isNew = true;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    bool m_setBeforeUse = false;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    bool m_everSet = false;

खुला:
    LifeVarEntry() = शेष;
    ~LifeVarEntry() = शेष;

    व्योम init(bool setBeforeUse) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = false;
        m_setBeforeUse = setBeforeUse;
    }
    string ascii() स्थिर {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        अगर (m_isNew) os << " isNew";
        अगर (m_setBeforeUse) os << " setBeforeUse";
        अगर (m_everSet) os << " everSet";
        अगर (m_assignp) os << " assignp=" << Ast_गाँठ::गाँठ_Addr(m_assignp);
        अगर (m_constp) os << " constp=" << Ast_गाँठ::गाँठ_Addr(m_constp);
        os << "]";
        वापस os.str();
    }  // LCOV_EXCL_STOP

    व्योम simpleAssign(Ast_गाँठ_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = nullptr;
        m_everSet = true;
        अगर (Ast_गाँठ_Assign* स्थिर assp = VN_CAST(nodep, Assign)) {
            अगर (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    व्योम complexAssign() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
        m_constp = nullptr;
        m_everSet = true;
    }
    व्योम consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
    }
    Ast_गाँठ_Stmt* assignp() स्थिर { वापस m_assignp; }
    AstConst* constNodep() स्थिर { वापस m_constp; }
    bool isNew() स्थिर { वापस m_isNew; }
    bool setBeforeUse() स्थिर { वापस m_setBeforeUse; }
    bool everSet() स्थिर { वापस m_everSet; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

श्रेणी LifeBlock final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, LifeVarEntry> m_map;
    LifeBlock* स्थिर m_aboveLifep;  // Upper life, or nullptr
    LifeState* स्थिर m_statep;  // Current global state
    bool m_noopt = false;  // Opaque operation seen; invalidates the block above at exit
    bool m_replacedVref = false;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

खुला:
    LifeBlock(LifeBlock* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~LifeBlock() = शेष;
    // METHODS
    व्योम checkRemoveAssign(स्थिर AstVarScope* vscp, LifeVarEntry& entr) {
        स्थिर AstVar* स्थिर varp = vscp->varp();
        // We don't optimize any public sigs
        अगर (varp->isSigPublic()) वापस;
        अगर (varp->isReadByDpi()) वापस;
        अगर (varp->sensIfacep()) वापस;
        // Check the var entry, and remove if appropriate
        Ast_गाँठ_Stmt* स्थिर oldassp = entr.assignp();
        अगर (!oldassp) वापस;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complexAssign();
        oldassp->unlinkFrBack();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.धकेलो_Deletep(oldassp), oldassp);
    }
    व्योम simpleAssign(AstVarScope* nodep, Ast_गाँठ_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        LifeVarEntry& entr = m_map[nodep];
        अगर (entr.isNew()) {
            entr.init(true);
        } अन्यथा {
            checkRemoveAssign(nodep, entr);
        }
        entr.simpleAssign(assp);
        // lifeDump();
    }
    व्योम complexAssign(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        अगर (entr.isNew()) entr.init(false);
        entr.complexAssign();
    }
    व्योम clearReplaced() { m_replacedVref = false; }
    bool replaced() स्थिर { वापस m_replacedVref; }
    व्योम varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        LifeVarEntry& entr = m_map[nodep];
        अगर (entr.isNew()) {
            entr.init(false);
        } अन्यथा {
            अगर (AstConst* स्थिर constp = entr.constNodep()) {
                अगर (!varrefp->varp()->isSigPublic() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_वृक्ष(false));
                    m_replacedVref = true;
                    VL_DO_DANGLING(varrefp->delete_वृक्ष(), varrefp);
                    ++m_statep->m_statAssnCon;
                    वापस;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    व्योम complexAssignFind(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        अगर (entr.isNew()) entr.init(false);
        entr.complexAssign();
    }
    व्योम consumedFind(AstVarScope* nodep) {
        LifeVarEntry& entr = m_map[nodep];
        अगर (entr.isNew()) entr.init(false);
        entr.consumed();
    }
    व्योम lifeToAbove() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        अगर (m_noopt) m_aboveLifep->noopt();
        क्रम (स्वतः& itr : m_map) {
            AstVarScope* स्थिर nodep = itr.first;
            m_aboveLifep->complexAssignFind(nodep);
            अगर (itr.second.everSet()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complexAssignFind(nodep);
            } अन्यथा {
                // Record consumption, so we don't eliminate earlier assignments
                consumedFind(nodep);
            }
        }
    }
    व्योम dualBranch(LifeBlock* life1p, LifeBlock* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_गाँठ::user1_Clear_वृक्ष();  // user1p() used on entire tree
        क्रम (स्वतः& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            अगर (itr.second.setBeforeUse()) itr.first->user1(1);
        }
        क्रम (स्वतः& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* स्थिर nodep = itr.first;
            अगर (itr.second.setBeforeUse() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                स्थिर स्वतः itab = m_map.find(nodep);
                अगर (itab != m_map.end()) checkRemoveAssign(nodep, itab->second);
            }
        }
        // this->lifeDump();
    }
    व्योम noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.सफा();
        m_noopt = true;
    }
    // DEBUG
    व्योम lifeDump() {
        UINFO(5, "  LifeMap:");
        क्रम (स्थिर स्वतः& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.second.setBeforeUse() ? "[F]  " : "     ") << itr.first);
            अगर (itr.second.assignp()) {  //
                UINFO(5, "       Ass: " << itr.second.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

श्रेणी LifeVisitor final : खुला VNVisitor {
    // STATE
    LifeState* स्थिर m_statep;  // Current state
    bool m_containsTiming = false;  // Statement contains timing control
    bool m_sideEffect = false;  // Side effects discovered in assign RHS
    bool m_noopt = false;  // Disable optimization of variables in this block
    bool m_tracingCall = false;  // Iterating into a CCall to a CFunc
    LifeBlock* m_lifep = nullptr;  // Current active lifetime map for current scope

    // METHODS
    व्योम setNoopt(स्थिर अक्षर* reasonp) {
        (व्योम)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = true;
        m_lifep->noopt();
    }

    व्योम processAssignment(Ast_गाँठ_Stmt* nodep, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = false;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), गाँठ_व्यंजक);
        अगर (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        अगर (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* स्थिर vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simpleAssign(vscp, nodep);
        } अन्यथा {
            iterate_And_अग्र_Null(lhsp);
        }
    }

    // VISITORS
    व्योम visit(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* स्थिर vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        अगर (nodep->access().isWriteOrRW()) {
            m_sideEffect = true;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complexAssign(vscp);
        } अन्यथा {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        अगर (nodep->isTimingControl() || VN_IS(nodep, AssignForce)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            setNoopt("timing|force");
            अगर (nodep->isTimingControl()) m_containsTiming = true;
            iterateChildren(nodep);
            वापस;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    व्योम visit(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    व्योम visit(AstAssignDly* nodep) override {
        // V3Life doesn't understand time sense
        अगर (nodep->isTimingControl()) {
            // Don't optimize
            setNoopt("assigndly");
            m_containsTiming = true;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    व्योम visit(Ast_गाँठ_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterate_And_अग्र_Null(nodep->condp());
        LifeBlock* स्थिर ifLifep = नया LifeBlock{m_lifep, m_statep};
        LifeBlock* स्थिर elseLifep = नया LifeBlock{m_lifep, m_statep};
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
        ifLifep->lifeToAbove();
        elseLifep->lifeToAbove();
        VL_DO_DANGLING(मिटाओ ifLifep, ifLifep);
        VL_DO_DANGLING(मिटाओ elseLifep, elseLifep);
        UINFO(4, "   if-done " << nodep);
    }
    व्योम visit(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = नया LifeBlock{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = true;
            iterate_And_अग्र_Null(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(मिटाओ m_lifep, m_lifep);
        }
        अगर (m_containsTiming) setNoopt("timing");
    }
    व्योम visit(AstJumpBlock* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = नया LifeBlock{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = true;
            iterate_And_अग्र_Null(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(मिटाओ m_lifep, m_lifep);
        }
        अगर (m_containsTiming) setNoopt("timing");
    }
    व्योम visit(Ast_गाँठ_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        अगर (nodep->funcp()->entryPoint()) {
            setNoopt("ccall");
        } अन्यथा {
            m_tracingCall = true;
            iterate(nodep->funcp());
        }
    }
    व्योम visit(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        अगर (!m_tracingCall && !nodep->entryPoint()) वापस;
        m_tracingCall = false;
        अगर (nodep->recursive()) setNoopt("recursive");
        अगर (nodep->noLife()) setNoopt("nolife");
        अगर (nodep->dpiImportPrototype() && !nodep->dpiPure()) {
            m_sideEffect = true;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstCExprUser* nodep) override {
        m_sideEffect = true;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    व्योम visit(AstCExpr* nodep) override {
        m_sideEffect = true;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    व्योम visit(AstVar*) override {}  // Don't want varrefs under it
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            setNoopt("timing");
            m_containsTiming = true;
        }
        iterateChildren(nodep);
    }

खुला:
    // CONSTRUCTORS
    LifeVisitor(Ast_गाँठ* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = नया LifeBlock{nullptr, m_statep};
            iterate(nodep);
            अगर (m_lifep) VL_DO_CLEAR(मिटाओ m_lifep, m_lifep = nullptr);
        }
    }
    ~LifeVisitor() override {
        अगर (m_lifep) VL_DO_CLEAR(मिटाओ m_lifep, m_lifep = nullptr);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

श्रेणी Life_शीर्ष_Visitor final : खुला VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
निजी:
    // STATE
    LifeState* स्थिर m_statep;  // Current state

    // VISITORS
    व्योम visit(AstCFunc* nodep) override {
        अगर (nodep->entryPoint()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    व्योम visit(AstVar*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ_Stmt*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ_व्यंजक*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    Life_शीर्ष_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_शीर्ष_Visitor() override = शेष;
};

//######################################################################
// Life class functions

व्योम V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_शीर्ष_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_वृक्ष();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_वृक्ष("life", 0, dump_वृक्ष_Either_Level() >= 3);
}
