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

رده LifeState final {
    // NODE STATE
    //   See below
    ثابت VNUser1InUse m_inuser1;

    // STATE
عمومی:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = پیشفرض;
    ~LifeState() {
        V3Stats::addStatSum("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::addStatSum("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

رده LifeVarEntry final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_گره_Stmt* m_assignp = nullptr;  // Last simple assignment
    AstConst* m_constp = nullptr;  // Known constant value
    منطقی m_isNew = درست;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    منطقی m_setBeforeUse = نادرست;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    منطقی m_everSet = نادرست;

عمومی:
    LifeVarEntry() = پیشفرض;
    ~LifeVarEntry() = پیشفرض;

    تهی init(منطقی setBeforeUse) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = نادرست;
        m_setBeforeUse = setBeforeUse;
    }
    string ascii() ثابت {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        اگر (m_isNew) os << " isNew";
        اگر (m_setBeforeUse) os << " setBeforeUse";
        اگر (m_everSet) os << " everSet";
        اگر (m_assignp) os << " assignp=" << Ast_گره::گره_Addr(m_assignp);
        اگر (m_constp) os << " constp=" << Ast_گره::گره_Addr(m_constp);
        os << "]";
        بازگشت os.str();
    }  // LCOV_EXCL_STOP

    تهی simpleAssign(Ast_گره_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = nullptr;
        m_everSet = درست;
        اگر (Ast_گره_Assign* ثابت assp = VN_CAST(nodep, Assign)) {
            اگر (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    تهی complexAssign() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
        m_constp = nullptr;
        m_everSet = درست;
    }
    تهی consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
    }
    Ast_گره_Stmt* assignp() ثابت { بازگشت m_assignp; }
    AstConst* constNodep() ثابت { بازگشت m_constp; }
    منطقی isNew() ثابت { بازگشت m_isNew; }
    منطقی setBeforeUse() ثابت { بازگشت m_setBeforeUse; }
    منطقی everSet() ثابت { بازگشت m_everSet; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

رده LifeBlock final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, LifeVarEntry> m_map;
    LifeBlock* ثابت m_aboveLifep;  // Upper life, or nullptr
    LifeState* ثابت m_statep;  // Current global state
    منطقی m_noopt = نادرست;  // Opaque operation seen; invalidates the block above at exit
    منطقی m_replacedVref = نادرست;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

عمومی:
    LifeBlock(LifeBlock* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~LifeBlock() = پیشفرض;
    // METHODS
    تهی checkRemoveAssign(ثابت AstVarScope* vscp, LifeVarEntry& entr) {
        ثابت AstVar* ثابت varp = vscp->varp();
        // We don't optimize any public sigs
        اگر (varp->isSigPublic()) بازگشت;
        اگر (varp->isReadByDpi()) بازگشت;
        اگر (varp->sensIfacep()) بازگشت;
        // Check the var entry, and remove if appropriate
        Ast_گره_Stmt* ثابت oldassp = entr.assignp();
        اگر (!oldassp) بازگشت;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complexAssign();
        oldassp->unlinkFrBack();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.افزودن_Deletep(oldassp), oldassp);
    }
    تهی simpleAssign(AstVarScope* nodep, Ast_گره_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        LifeVarEntry& entr = m_map[nodep];
        اگر (entr.isNew()) {
            entr.init(درست);
        } وگرنه {
            checkRemoveAssign(nodep, entr);
        }
        entr.simpleAssign(assp);
        // lifeDump();
    }
    تهی complexAssign(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        اگر (entr.isNew()) entr.init(نادرست);
        entr.complexAssign();
    }
    تهی clearReplaced() { m_replacedVref = نادرست; }
    منطقی replaced() ثابت { بازگشت m_replacedVref; }
    تهی varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        LifeVarEntry& entr = m_map[nodep];
        اگر (entr.isNew()) {
            entr.init(نادرست);
        } وگرنه {
            اگر (AstConst* ثابت constp = entr.constNodep()) {
                اگر (!varrefp->varp()->isSigPublic() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_درخت(نادرست));
                    m_replacedVref = درست;
                    VL_DO_DANGLING(varrefp->delete_درخت(), varrefp);
                    ++m_statep->m_statAssnCon;
                    بازگشت;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    تهی complexAssignFind(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        اگر (entr.isNew()) entr.init(نادرست);
        entr.complexAssign();
    }
    تهی consumedFind(AstVarScope* nodep) {
        LifeVarEntry& entr = m_map[nodep];
        اگر (entr.isNew()) entr.init(نادرست);
        entr.consumed();
    }
    تهی lifeToAbove() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        اگر (m_noopt) m_aboveLifep->noopt();
        برای (auto& itr : m_map) {
            AstVarScope* ثابت nodep = itr.first;
            m_aboveLifep->complexAssignFind(nodep);
            اگر (itr.second.everSet()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complexAssignFind(nodep);
            } وگرنه {
                // Record consumption, so we don't eliminate earlier assignments
                consumedFind(nodep);
            }
        }
    }
    تهی dualBranch(LifeBlock* life1p, LifeBlock* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_گره::user1_Clear_درخت();  // user1p() used on entire tree
        برای (auto& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            اگر (itr.second.setBeforeUse()) itr.first->user1(1);
        }
        برای (auto& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* ثابت nodep = itr.first;
            اگر (itr.second.setBeforeUse() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                ثابت auto itab = m_map.find(nodep);
                اگر (itab != m_map.end()) checkRemoveAssign(nodep, itab->second);
            }
        }
        // this->lifeDump();
    }
    تهی noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = درست;
    }
    // DEBUG
    تهی lifeDump() {
        UINFO(5, "  LifeMap:");
        برای (ثابت auto& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.second.setBeforeUse() ? "[F]  " : "     ") << itr.first);
            اگر (itr.second.assignp()) {  //
                UINFO(5, "       Ass: " << itr.second.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

رده LifeVisitor final : عمومی VNVisitor {
    // STATE
    LifeState* ثابت m_statep;  // Current state
    منطقی m_containsTiming = نادرست;  // Statement contains timing control
    منطقی m_sideEffect = نادرست;  // Side effects discovered in assign RHS
    منطقی m_noopt = نادرست;  // Disable optimization of variables in this block
    منطقی m_tracingCall = نادرست;  // Iterating into a CCall to a CFunc
    LifeBlock* m_lifep = nullptr;  // Current active lifetime map for current scope

    // METHODS
    تهی setNoopt(ثابت نویسه* reasonp) {
        (تهی)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = درست;
        m_lifep->noopt();
    }

    تهی processAssignment(Ast_گره_Stmt* nodep, Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = نادرست;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), گره_عبارت);
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
            m_lifep->simpleAssign(vscp, nodep);
        } وگرنه {
            iterateAndNextNull(lhsp);
        }
    }

    // VISITORS
    تهی visit(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* ثابت vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        اگر (nodep->access().isWriteOrRW()) {
            m_sideEffect = درست;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complexAssign(vscp);
        } وگرنه {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    تهی visit(Ast_گره_Assign* nodep) override {
        اگر (nodep->isTimingControl() || VN_IS(nodep, AssignForce)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            setNoopt("timing|force");
            اگر (nodep->isTimingControl()) m_containsTiming = درست;
            iterateChildren(nodep);
            بازگشت;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    تهی visit(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    تهی visit(AstAssignDly* nodep) override {
        // V3Life doesn't understand time sense
        اگر (nodep->isTimingControl()) {
            // Don't optimize
            setNoopt("assigndly");
            m_containsTiming = درست;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    تهی visit(Ast_گره_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterateAndNextNull(nodep->condp());
        LifeBlock* ثابت ifLifep = جدید LifeBlock{m_lifep, m_statep};
        LifeBlock* ثابت elseLifep = جدید LifeBlock{m_lifep, m_statep};
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
    تهی visit(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = جدید LifeBlock{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = درست;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        اگر (m_containsTiming) setNoopt("timing");
    }
    تهی visit(AstJumpBlock* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = جدید LifeBlock{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = درست;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        اگر (m_containsTiming) setNoopt("timing");
    }
    تهی visit(Ast_گره_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        اگر (nodep->funcp()->entryPoint()) {
            setNoopt("ccall");
        } وگرنه {
            m_tracingCall = درست;
            iterate(nodep->funcp());
        }
    }
    تهی visit(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        اگر (!m_tracingCall && !nodep->entryPoint()) بازگشت;
        m_tracingCall = نادرست;
        اگر (nodep->recursive()) setNoopt("recursive");
        اگر (nodep->noLife()) setNoopt("nolife");
        اگر (nodep->dpiImportPrototype() && !nodep->dpiPure()) {
            m_sideEffect = درست;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    تهی visit(AstCExprUser* nodep) override {
        m_sideEffect = درست;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    تهی visit(AstCExpr* nodep) override {
        m_sideEffect = درست;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    تهی visit(AstVar*) override {}  // Don't want varrefs under it
    تهی visit(Ast_گره* nodep) override {
        اگر (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            setNoopt("timing");
            m_containsTiming = درست;
        }
        iterateChildren(nodep);
    }

عمومی:
    // CONSTRUCTORS
    LifeVisitor(Ast_گره* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = جدید LifeBlock{nullptr, m_statep};
            iterate(nodep);
            اگر (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
        }
    }
    ~LifeVisitor() override {
        اگر (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

رده Life_بالاترین_Visitor final : عمومی VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
خصوصی:
    // STATE
    LifeState* ثابت m_statep;  // Current state

    // VISITORS
    تهی visit(AstCFunc* nodep) override {
        اگر (nodep->entryPoint()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    تهی visit(Ast_گره_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    تهی visit(AstVar*) override {}  // Accelerate
    تهی visit(Ast_گره_Stmt*) override {}  // Accelerate
    تهی visit(Ast_گره_عبارت*) override {}  // Accelerate
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    Life_بالاترین_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_بالاترین_Visitor() override = پیشفرض;
};

//######################################################################
// Life class functions

تهی V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_بالاترین_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_درخت();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_درخت("life", 0, dump_درخت_Either_Level() >= 3);
}
