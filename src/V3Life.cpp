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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Life.h"

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Structure for global state

വർഗ്ഗം LifeState final {
    // NODE STATE
    //   See below
    സ്ഥിരം VNUser1InUse m_inuser1;

    // STATE
പൊതു:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = സ്ഥിരസ്ഥിതി;
    ~LifeState() {
        V3Stats::addStatSum("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::addStatSum("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

വർഗ്ഗം LifeVarEntry final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_കണ്ണി_Stmt* m_assignp = nullptr;  // Last simple assignment
    AstConst* m_constp = nullptr;  // Known constant value
    തർക്കം m_isNew = സത്യം;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    തർക്കം m_setBeforeUse = അസത്യം;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    തർക്കം m_everSet = അസത്യം;

പൊതു:
    LifeVarEntry() = സ്ഥിരസ്ഥിതി;
    ~LifeVarEntry() = സ്ഥിരസ്ഥിതി;

    ശൂന്യം init(തർക്കം setBeforeUse) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = അസത്യം;
        m_setBeforeUse = setBeforeUse;
    }
    string ascii() സ്ഥിരം {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        എങ്കിൽ (m_isNew) os << " isNew";
        എങ്കിൽ (m_setBeforeUse) os << " setBeforeUse";
        എങ്കിൽ (m_everSet) os << " everSet";
        എങ്കിൽ (m_assignp) os << " assignp=" << Ast_കണ്ണി::കണ്ണി_Addr(m_assignp);
        എങ്കിൽ (m_constp) os << " constp=" << Ast_കണ്ണി::കണ്ണി_Addr(m_constp);
        os << "]";
        തിരികെ os.str();
    }  // LCOV_EXCL_STOP

    ശൂന്യം simpleAssign(Ast_കണ്ണി_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = nullptr;
        m_everSet = സത്യം;
        എങ്കിൽ (Ast_കണ്ണി_Assign* സ്ഥിരം assp = VN_CAST(nodep, Assign)) {
            എങ്കിൽ (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    ശൂന്യം complexAssign() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
        m_constp = nullptr;
        m_everSet = സത്യം;
    }
    ശൂന്യം consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
    }
    Ast_കണ്ണി_Stmt* assignp() സ്ഥിരം { തിരികെ m_assignp; }
    AstConst* constNodep() സ്ഥിരം { തിരികെ m_constp; }
    തർക്കം isNew() സ്ഥിരം { തിരികെ m_isNew; }
    തർക്കം setBeforeUse() സ്ഥിരം { തിരികെ m_setBeforeUse; }
    തർക്കം everSet() സ്ഥിരം { തിരികെ m_everSet; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

വർഗ്ഗം LifeBlock final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, LifeVarEntry> m_map;
    LifeBlock* സ്ഥിരം m_aboveLifep;  // Upper life, or nullptr
    LifeState* സ്ഥിരം m_statep;  // Current global state
    തർക്കം m_noopt = അസത്യം;  // Opaque operation seen; invalidates the block above at exit
    തർക്കം m_replacedVref = അസത്യം;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

പൊതു:
    LifeBlock(LifeBlock* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~LifeBlock() = സ്ഥിരസ്ഥിതി;
    // METHODS
    ശൂന്യം checkRemoveAssign(സ്ഥിരം AstVarScope* vscp, LifeVarEntry& entr) {
        സ്ഥിരം AstVar* സ്ഥിരം varp = vscp->varp();
        // We don't optimize any public sigs
        എങ്കിൽ (varp->isSigPublic()) തിരികെ;
        എങ്കിൽ (varp->isReadByDpi()) തിരികെ;
        എങ്കിൽ (varp->sensIfacep()) തിരികെ;
        // Check the var entry, and remove if appropriate
        Ast_കണ്ണി_Stmt* സ്ഥിരം oldassp = entr.assignp();
        എങ്കിൽ (!oldassp) തിരികെ;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complexAssign();
        oldassp->unlinkFrBack();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.തള്ളുക_Deletep(oldassp), oldassp);
    }
    ശൂന്യം simpleAssign(AstVarScope* nodep, Ast_കണ്ണി_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        LifeVarEntry& entr = m_map[nodep];
        എങ്കിൽ (entr.isNew()) {
            entr.init(സത്യം);
        } അല്ലെങ്കിൽ {
            checkRemoveAssign(nodep, entr);
        }
        entr.simpleAssign(assp);
        // lifeDump();
    }
    ശൂന്യം complexAssign(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        എങ്കിൽ (entr.isNew()) entr.init(അസത്യം);
        entr.complexAssign();
    }
    ശൂന്യം clearReplaced() { m_replacedVref = അസത്യം; }
    തർക്കം replaced() സ്ഥിരം { തിരികെ m_replacedVref; }
    ശൂന്യം varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        LifeVarEntry& entr = m_map[nodep];
        എങ്കിൽ (entr.isNew()) {
            entr.init(അസത്യം);
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (AstConst* സ്ഥിരം constp = entr.constNodep()) {
                എങ്കിൽ (!varrefp->varp()->isSigPublic() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_വൃക്ഷം(അസത്യം));
                    m_replacedVref = സത്യം;
                    VL_DO_DANGLING(varrefp->delete_വൃക്ഷം(), varrefp);
                    ++m_statep->m_statAssnCon;
                    തിരികെ;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    ശൂന്യം complexAssignFind(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        എങ്കിൽ (entr.isNew()) entr.init(അസത്യം);
        entr.complexAssign();
    }
    ശൂന്യം consumedFind(AstVarScope* nodep) {
        LifeVarEntry& entr = m_map[nodep];
        എങ്കിൽ (entr.isNew()) entr.init(അസത്യം);
        entr.consumed();
    }
    ശൂന്യം lifeToAbove() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        എങ്കിൽ (m_noopt) m_aboveLifep->noopt();
        വേണ്ടി (auto& itr : m_map) {
            AstVarScope* സ്ഥിരം nodep = itr.first;
            m_aboveLifep->complexAssignFind(nodep);
            എങ്കിൽ (itr.second.everSet()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complexAssignFind(nodep);
            } അല്ലെങ്കിൽ {
                // Record consumption, so we don't eliminate earlier assignments
                consumedFind(nodep);
            }
        }
    }
    ശൂന്യം dualBranch(LifeBlock* life1p, LifeBlock* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_കണ്ണി::user1_Clear_വൃക്ഷം();  // user1p() used on entire tree
        വേണ്ടി (auto& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            എങ്കിൽ (itr.second.setBeforeUse()) itr.first->user1(1);
        }
        വേണ്ടി (auto& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* സ്ഥിരം nodep = itr.first;
            എങ്കിൽ (itr.second.setBeforeUse() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                സ്ഥിരം auto itab = m_map.find(nodep);
                എങ്കിൽ (itab != m_map.end()) checkRemoveAssign(nodep, itab->second);
            }
        }
        // this->lifeDump();
    }
    ശൂന്യം noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = സത്യം;
    }
    // DEBUG
    ശൂന്യം lifeDump() {
        UINFO(5, "  LifeMap:");
        വേണ്ടി (സ്ഥിരം auto& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.second.setBeforeUse() ? "[F]  " : "     ") << itr.first);
            എങ്കിൽ (itr.second.assignp()) {  //
                UINFO(5, "       Ass: " << itr.second.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

വർഗ്ഗം LifeVisitor final : പൊതു VNVisitor {
    // STATE
    LifeState* സ്ഥിരം m_statep;  // Current state
    തർക്കം m_containsTiming = അസത്യം;  // Statement contains timing control
    തർക്കം m_sideEffect = അസത്യം;  // Side effects discovered in assign RHS
    തർക്കം m_noopt = അസത്യം;  // Disable optimization of variables in this block
    തർക്കം m_tracingCall = അസത്യം;  // Iterating into a CCall to a CFunc
    LifeBlock* m_lifep = nullptr;  // Current active lifetime map for current scope

    // METHODS
    ശൂന്യം setNoopt(സ്ഥിരം അക്ഷരം* reasonp) {
        (ശൂന്യം)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = സത്യം;
        m_lifep->noopt();
    }

    ശൂന്യം processAssignment(Ast_കണ്ണി_Stmt* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = അസത്യം;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), കണ്ണി_വ്യഞ്ജകം);
        എങ്കിൽ (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        എങ്കിൽ (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* സ്ഥിരം vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simpleAssign(vscp, nodep);
        } അല്ലെങ്കിൽ {
            iterateAndNextNull(lhsp);
        }
    }

    // VISITORS
    ശൂന്യം visit(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* സ്ഥിരം vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        എങ്കിൽ (nodep->access().isWriteOrRW()) {
            m_sideEffect = സത്യം;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complexAssign(vscp);
        } അല്ലെങ്കിൽ {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        എങ്കിൽ (nodep->isTimingControl() || VN_IS(nodep, AssignForce)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            setNoopt("timing|force");
            എങ്കിൽ (nodep->isTimingControl()) m_containsTiming = സത്യം;
            iterateChildren(nodep);
            തിരികെ;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ശൂന്യം visit(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    ശൂന്യം visit(AstAssignDly* nodep) override {
        // V3Life doesn't understand time sense
        എങ്കിൽ (nodep->isTimingControl()) {
            // Don't optimize
            setNoopt("assigndly");
            m_containsTiming = സത്യം;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterateAndNextNull(nodep->condp());
        LifeBlock* സ്ഥിരം ifLifep = പുതിയ LifeBlock{m_lifep, m_statep};
        LifeBlock* സ്ഥിരം elseLifep = പുതിയ LifeBlock{m_lifep, m_statep};
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
    ശൂന്യം visit(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = പുതിയ LifeBlock{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = സത്യം;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        എങ്കിൽ (m_containsTiming) setNoopt("timing");
    }
    ശൂന്യം visit(AstJumpBlock* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = പുതിയ LifeBlock{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = സത്യം;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        എങ്കിൽ (m_containsTiming) setNoopt("timing");
    }
    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        എങ്കിൽ (nodep->funcp()->entryPoint()) {
            setNoopt("ccall");
        } അല്ലെങ്കിൽ {
            m_tracingCall = സത്യം;
            iterate(nodep->funcp());
        }
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        എങ്കിൽ (!m_tracingCall && !nodep->entryPoint()) തിരികെ;
        m_tracingCall = അസത്യം;
        എങ്കിൽ (nodep->recursive()) setNoopt("recursive");
        എങ്കിൽ (nodep->noLife()) setNoopt("nolife");
        എങ്കിൽ (nodep->dpiImportPrototype() && !nodep->dpiPure()) {
            m_sideEffect = സത്യം;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCExprUser* nodep) override {
        m_sideEffect = സത്യം;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCExpr* nodep) override {
        m_sideEffect = സത്യം;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstVar*) override {}  // Don't want varrefs under it
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            setNoopt("timing");
            m_containsTiming = സത്യം;
        }
        iterateChildren(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    LifeVisitor(Ast_കണ്ണി* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = പുതിയ LifeBlock{nullptr, m_statep};
            iterate(nodep);
            എങ്കിൽ (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
        }
    }
    ~LifeVisitor() override {
        എങ്കിൽ (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

വർഗ്ഗം Life_മുകൾ_Visitor final : പൊതു VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
സ്വകാര്യ:
    // STATE
    LifeState* സ്ഥിരം m_statep;  // Current state

    // VISITORS
    ശൂന്യം visit(AstCFunc* nodep) override {
        എങ്കിൽ (nodep->entryPoint()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    ശൂന്യം visit(AstVar*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി_Stmt*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    Life_മുകൾ_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_മുകൾ_Visitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Life class functions

ശൂന്യം V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_മുകൾ_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_വൃക്ഷം();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_വൃക്ഷം("life", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
