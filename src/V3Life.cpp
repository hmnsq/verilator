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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Life.h"

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Structure for global state

ଶ୍ରେଣୀ LifeState final {
    // NODE STATE
    //   See below
    ଅଚଳ VNUser1InUse m_inuser1;

    // STATE
ସର୍ବସାଧାରଣ:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~LifeState() {
        V3Stats::addStatSum("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::addStatSum("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

ଶ୍ରେଣୀ LifeVarEntry final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_ଗଣ୍ଠି_Stmt* m_assignp = nullptr;  // Last simple assignment
    AstConst* m_constp = nullptr;  // Known constant value
    ତାର୍କିକ m_isNew = ସତ୍ୟ;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    ତାର୍କିକ m_setBeforeUse = ମିଥ୍ୟା;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    ତାର୍କିକ m_everSet = ମିଥ୍ୟା;

ସର୍ବସାଧାରଣ:
    LifeVarEntry() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~LifeVarEntry() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    ଶୂନ୍ୟ init(ତାର୍କିକ setBeforeUse) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = ମିଥ୍ୟା;
        m_setBeforeUse = setBeforeUse;
    }
    string ascii() ଅଚଳ {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        ଯଦି (m_isNew) os << " isNew";
        ଯଦି (m_setBeforeUse) os << " setBeforeUse";
        ଯଦି (m_everSet) os << " everSet";
        ଯଦି (m_assignp) os << " assignp=" << Ast_ଗଣ୍ଠି::ଗଣ୍ଠି_Addr(m_assignp);
        ଯଦି (m_constp) os << " constp=" << Ast_ଗଣ୍ଠି::ଗଣ୍ଠି_Addr(m_constp);
        os << "]";
        ଫେରନ୍ତୁ os.str();
    }  // LCOV_EXCL_STOP

    ଶୂନ୍ୟ simpleAssign(Ast_ଗଣ୍ଠି_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = nullptr;
        m_everSet = ସତ୍ୟ;
        ଯଦି (Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = VN_CAST(nodep, Assign)) {
            ଯଦି (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    ଶୂନ୍ୟ complexAssign() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
        m_constp = nullptr;
        m_everSet = ସତ୍ୟ;
    }
    ଶୂନ୍ୟ consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
    }
    Ast_ଗଣ୍ଠି_Stmt* assignp() ଅଚଳ { ଫେରନ୍ତୁ m_assignp; }
    AstConst* constNodep() ଅଚଳ { ଫେରନ୍ତୁ m_constp; }
    ତାର୍କିକ isNew() ଅଚଳ { ଫେରନ୍ତୁ m_isNew; }
    ତାର୍କିକ setBeforeUse() ଅଚଳ { ଫେରନ୍ତୁ m_setBeforeUse; }
    ତାର୍କିକ everSet() ଅଚଳ { ଫେରନ୍ତୁ m_everSet; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

ଶ୍ରେଣୀ LifeBlock final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, LifeVarEntry> m_map;
    LifeBlock* ଅଚଳ m_aboveLifep;  // Upper life, or nullptr
    LifeState* ଅଚଳ m_statep;  // Current global state
    ତାର୍କିକ m_noopt = ମିଥ୍ୟା;  // Opaque operation seen; invalidates the block above at exit
    ତାର୍କିକ m_replacedVref = ମିଥ୍ୟା;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

ସର୍ବସାଧାରଣ:
    LifeBlock(LifeBlock* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~LifeBlock() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // METHODS
    ଶୂନ୍ୟ checkRemoveAssign(ଅଚଳ AstVarScope* vscp, LifeVarEntry& entr) {
        ଅଚଳ AstVar* ଅଚଳ varp = vscp->varp();
        // We don't optimize any public sigs
        ଯଦି (varp->isSigPublic()) ଫେରନ୍ତୁ;
        ଯଦି (varp->isReadByDpi()) ଫେରନ୍ତୁ;
        ଯଦି (varp->sensIfacep()) ଫେରନ୍ତୁ;
        // Check the var entry, and remove if appropriate
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ oldassp = entr.assignp();
        ଯଦି (!oldassp) ଫେରନ୍ତୁ;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complexAssign();
        oldassp->unlinkFrBack();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.ଠେଲ_Deletep(oldassp), oldassp);
    }
    ଶୂନ୍ୟ simpleAssign(AstVarScope* nodep, Ast_ଗଣ୍ଠି_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        LifeVarEntry& entr = m_map[nodep];
        ଯଦି (entr.isNew()) {
            entr.init(ସତ୍ୟ);
        } ଅନ୍ୟଥା {
            checkRemoveAssign(nodep, entr);
        }
        entr.simpleAssign(assp);
        // lifeDump();
    }
    ଶୂନ୍ୟ complexAssign(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        ଯଦି (entr.isNew()) entr.init(ମିଥ୍ୟା);
        entr.complexAssign();
    }
    ଶୂନ୍ୟ clearReplaced() { m_replacedVref = ମିଥ୍ୟା; }
    ତାର୍କିକ replaced() ଅଚଳ { ଫେରନ୍ତୁ m_replacedVref; }
    ଶୂନ୍ୟ varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        LifeVarEntry& entr = m_map[nodep];
        ଯଦି (entr.isNew()) {
            entr.init(ମିଥ୍ୟା);
        } ଅନ୍ୟଥା {
            ଯଦି (AstConst* ଅଚଳ constp = entr.constNodep()) {
                ଯଦି (!varrefp->varp()->isSigPublic() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                    m_replacedVref = ସତ୍ୟ;
                    VL_DO_DANGLING(varrefp->delete_ବୃକ୍ଷ(), varrefp);
                    ++m_statep->m_statAssnCon;
                    ଫେରନ୍ତୁ;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    ଶୂନ୍ୟ complexAssignFind(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        ଯଦି (entr.isNew()) entr.init(ମିଥ୍ୟା);
        entr.complexAssign();
    }
    ଶୂନ୍ୟ consumedFind(AstVarScope* nodep) {
        LifeVarEntry& entr = m_map[nodep];
        ଯଦି (entr.isNew()) entr.init(ମିଥ୍ୟା);
        entr.consumed();
    }
    ଶୂନ୍ୟ lifeToAbove() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        ଯଦି (m_noopt) m_aboveLifep->noopt();
        ପାଇଁ (auto& itr : m_map) {
            AstVarScope* ଅଚଳ nodep = itr.first;
            m_aboveLifep->complexAssignFind(nodep);
            ଯଦି (itr.second.everSet()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complexAssignFind(nodep);
            } ଅନ୍ୟଥା {
                // Record consumption, so we don't eliminate earlier assignments
                consumedFind(nodep);
            }
        }
    }
    ଶୂନ୍ୟ dualBranch(LifeBlock* life1p, LifeBlock* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_ଗଣ୍ଠି::user1_Clear_ବୃକ୍ଷ();  // user1p() used on entire tree
        ପାଇଁ (auto& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            ଯଦି (itr.second.setBeforeUse()) itr.first->user1(1);
        }
        ପାଇଁ (auto& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* ଅଚଳ nodep = itr.first;
            ଯଦି (itr.second.setBeforeUse() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                ଅଚଳ auto itab = m_map.find(nodep);
                ଯଦି (itab != m_map.end()) checkRemoveAssign(nodep, itab->second);
            }
        }
        // this->lifeDump();
    }
    ଶୂନ୍ୟ noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = ସତ୍ୟ;
    }
    // DEBUG
    ଶୂନ୍ୟ lifeDump() {
        UINFO(5, "  LifeMap:");
        ପାଇଁ (ଅଚଳ auto& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.second.setBeforeUse() ? "[F]  " : "     ") << itr.first);
            ଯଦି (itr.second.assignp()) {  //
                UINFO(5, "       Ass: " << itr.second.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

ଶ୍ରେଣୀ LifeVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE
    LifeState* ଅଚଳ m_statep;  // Current state
    ତାର୍କିକ m_containsTiming = ମିଥ୍ୟା;  // Statement contains timing control
    ତାର୍କିକ m_sideEffect = ମିଥ୍ୟା;  // Side effects discovered in assign RHS
    ତାର୍କିକ m_noopt = ମିଥ୍ୟା;  // Disable optimization of variables in this block
    ତାର୍କିକ m_tracingCall = ମିଥ୍ୟା;  // Iterating into a CCall to a CFunc
    LifeBlock* m_lifep = nullptr;  // Current active lifetime map for current scope

    // METHODS
    ଶୂନ୍ୟ setNoopt(ଅଚଳ ଅକ୍ଷର* reasonp) {
        (ଶୂନ୍ୟ)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = ସତ୍ୟ;
        m_lifep->noopt();
    }

    ଶୂନ୍ୟ processAssignment(Ast_ଗଣ୍ଠି_Stmt* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = ମିଥ୍ୟା;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
        ଯଦି (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        ଯଦି (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* ଅଚଳ vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simpleAssign(vscp, nodep);
        } ଅନ୍ୟଥା {
            iterateAndNextNull(lhsp);
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* ଅଚଳ vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        ଯଦି (nodep->access().isWriteOrRW()) {
            m_sideEffect = ସତ୍ୟ;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complexAssign(vscp);
        } ଅନ୍ୟଥା {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        ଯଦି (nodep->isTimingControl() || VN_IS(nodep, AssignForce)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            setNoopt("timing|force");
            ଯଦି (nodep->isTimingControl()) m_containsTiming = ସତ୍ୟ;
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ଶୂନ୍ୟ visit(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override {
        // V3Life doesn't understand time sense
        ଯଦି (nodep->isTimingControl()) {
            // Don't optimize
            setNoopt("assigndly");
            m_containsTiming = ସତ୍ୟ;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterateAndNextNull(nodep->condp());
        LifeBlock* ଅଚଳ ifLifep = ନୂତନ LifeBlock{m_lifep, m_statep};
        LifeBlock* ଅଚଳ elseLifep = ନୂତନ LifeBlock{m_lifep, m_statep};
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
    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = ନୂତନ LifeBlock{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = ସତ୍ୟ;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        ଯଦି (m_containsTiming) setNoopt("timing");
    }
    ଶୂନ୍ୟ visit(AstJumpBlock* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = ନୂତନ LifeBlock{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = ସତ୍ୟ;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        ଯଦି (m_containsTiming) setNoopt("timing");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        ଯଦି (nodep->funcp()->entryPoint()) {
            setNoopt("ccall");
        } ଅନ୍ୟଥା {
            m_tracingCall = ସତ୍ୟ;
            iterate(nodep->funcp());
        }
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        ଯଦି (!m_tracingCall && !nodep->entryPoint()) ଫେରନ୍ତୁ;
        m_tracingCall = ମିଥ୍ୟା;
        ଯଦି (nodep->recursive()) setNoopt("recursive");
        ଯଦି (nodep->noLife()) setNoopt("nolife");
        ଯଦି (nodep->dpiImportPrototype() && !nodep->dpiPure()) {
            m_sideEffect = ସତ୍ୟ;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCExprUser* nodep) override {
        m_sideEffect = ସତ୍ୟ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCExpr* nodep) override {
        m_sideEffect = ସତ୍ୟ;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstVar*) override {}  // Don't want varrefs under it
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            setNoopt("timing");
            m_containsTiming = ସତ୍ୟ;
        }
        iterateChildren(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    LifeVisitor(Ast_ଗଣ୍ଠି* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = ନୂତନ LifeBlock{nullptr, m_statep};
            iterate(nodep);
            ଯଦି (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
        }
    }
    ~LifeVisitor() override {
        ଯଦି (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

ଶ୍ରେଣୀ Life_ଶୀର୍ଷ_Visitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
ବ୍ୟକ୍ତିଗତ:
    // STATE
    LifeState* ଅଚଳ m_statep;  // Current state

    // VISITORS
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        ଯଦି (nodep->entryPoint()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    ଶୂନ୍ୟ visit(AstVar*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    Life_ଶୀର୍ଷ_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_ଶୀର୍ଷ_Visitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Life class functions

ଶୂନ୍ୟ V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_ଶୀର୍ଷ_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_ବୃକ୍ଷ();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_ବୃକ୍ଷ("life", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
