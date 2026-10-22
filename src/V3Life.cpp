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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Life.h"

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Structure for global state

வகுப்பு LifeState final {
    // NODE STATE
    //   See below
    மாறிலி VNUser1InUse m_inuser1;

    // STATE
பொது:
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_statAssnCon;  // Statistic tracking

    // CONSTRUCTORS
    LifeState() = இயல்பு;
    ~LifeState() {
        V3Stats::addStatSum("Optimizations, Lifetime assign deletions", m_statAssnDel);
        V3Stats::addStatSum("Optimizations, Lifetime constant prop", m_statAssnCon);
    }
};

//######################################################################
// Structure for each variable encountered

வகுப்பு LifeVarEntry final {
    // Last assignment to this varscope, nullptr if no longer relevant
    Ast_முனை_Stmt* m_assignp = nullptr;  // Last simple assignment
    AstConst* m_constp = nullptr;  // Known constant value
    தருக்க m_isNew = உண்மை;  // Is just created
    // First access was a set (and thus block above may have a set that can be deleted
    தருக்க m_setBeforeUse = பொய்;
    // Was ever assigned (and thus above block may not preserve constant propagation)
    தருக்க m_everSet = பொய்;

பொது:
    LifeVarEntry() = இயல்பு;
    ~LifeVarEntry() = இயல்பு;

    வெறுமை init(தருக்க setBeforeUse) {
        UASSERT(m_isNew, "Not a new entry");
        m_isNew = பொய்;
        m_setBeforeUse = setBeforeUse;
    }
    string ascii() மாறிலி {  // LCOV_EXCL_START
        std::ostringstream os;
        os << "[Life ";
        எனில் (m_isNew) os << " isNew";
        எனில் (m_setBeforeUse) os << " setBeforeUse";
        எனில் (m_everSet) os << " everSet";
        எனில் (m_assignp) os << " assignp=" << Ast_முனை::முனை_Addr(m_assignp);
        எனில் (m_constp) os << " constp=" << Ast_முனை::முனை_Addr(m_constp);
        os << "]";
        திரும்பு os.str();
    }  // LCOV_EXCL_STOP

    வெறுமை simpleAssign(Ast_முனை_Stmt* nodep) {  // New simple A=.... assignment
        UASSERT_OBJ(!m_isNew, nodep, "Uninitialized new entry");
        m_assignp = nodep;
        m_constp = nullptr;
        m_everSet = உண்மை;
        எனில் (Ast_முனை_Assign* மாறிலி assp = VN_CAST(nodep, Assign)) {
            எனில் (VN_IS(assp->rhsp(), Const)) {
                m_constp = VN_AS(assp->rhsp(), Const);
                UINFO(9, "assign-const " << assp->rhsp() << " = " << m_constp);
            }
        }
    }
    வெறுமை complexAssign() {  // A[x]=... or some complicated assignment
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
        m_constp = nullptr;
        m_everSet = உண்மை;
    }
    வெறுமை consumed() {  // Rvalue read of A
        UASSERT(!m_isNew, "Uninitialized new entry");
        m_assignp = nullptr;
    }
    Ast_முனை_Stmt* assignp() மாறிலி { திரும்பு m_assignp; }
    AstConst* constNodep() மாறிலி { திரும்பு m_constp; }
    தருக்க isNew() மாறிலி { திரும்பு m_isNew; }
    தருக்க setBeforeUse() மாறிலி { திரும்பு m_setBeforeUse; }
    தருக்க everSet() மாறிலி { திரும்பு m_everSet; }
};

//######################################################################
// Structure for all variables under a given meta-basic block

வகுப்பு LifeBlock final {
    // NODE STATE
    // Cleared each AstIf:
    //   AstVarScope::user1()   -> uint64_t.  Used in combining to detect duplicates

    // LIFE MAP
    // For each basic block, we'll make a new map of what variables that if/else is changing
    // Current active lifetime map for current scope
    std::unordered_map<AstVarScope*, LifeVarEntry> m_map;
    LifeBlock* மாறிலி m_aboveLifep;  // Upper life, or nullptr
    LifeState* மாறிலி m_statep;  // Current global state
    தருக்க m_noopt = பொய்;  // Opaque operation seen; invalidates the block above at exit
    தருக்க m_replacedVref = பொய்;  // Replaced a variable reference since last clearing
    VNDeleter m_deleter;  // Used to delay deletion of nodes

பொது:
    LifeBlock(LifeBlock* aboveLifep, LifeState* statep)
        : m_aboveLifep{aboveLifep}  // Null if top
        , m_statep{statep} {}
    ~LifeBlock() = இயல்பு;
    // METHODS
    வெறுமை checkRemoveAssign(மாறிலி AstVarScope* vscp, LifeVarEntry& entr) {
        மாறிலி AstVar* மாறிலி varp = vscp->varp();
        // We don't optimize any public sigs
        எனில் (varp->isSigPublic()) திரும்பு;
        எனில் (varp->isReadByDpi()) திரும்பு;
        எனில் (varp->sensIfacep()) திரும்பு;
        // Check the var entry, and remove if appropriate
        Ast_முனை_Stmt* மாறிலி oldassp = entr.assignp();
        எனில் (!oldassp) திரும்பு;
        UINFO(7, "       PREV: " << oldassp);
        // Redundant assignment, in same level block
        // Don't delete it now as it will confuse iteration since it maybe WAY
        // above our current iteration point.
        UINFOTREE(7, oldassp, "", "REMOVE/SAMEBLK");
        entr.complexAssign();
        oldassp->unlinkFrBack();
        ++m_statep->m_statAssnDel;
        VL_DO_DANGLING(m_deleter.தள்ளு_Deletep(oldassp), oldassp);
    }
    வெறுமை simpleAssign(AstVarScope* nodep, Ast_முனை_Stmt* assp) {
        // Do we have a old assignment we can nuke?
        UINFO(4, "     ASSIGNof: " << nodep);
        UINFO(7, "       new: " << assp);
        LifeVarEntry& entr = m_map[nodep];
        எனில் (entr.isNew()) {
            entr.init(உண்மை);
        } இல்லையெனில் {
            checkRemoveAssign(nodep, entr);
        }
        entr.simpleAssign(assp);
        // lifeDump();
    }
    வெறுமை complexAssign(AstVarScope* nodep) {
        UINFO(4, "     clearof: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        எனில் (entr.isNew()) entr.init(பொய்);
        entr.complexAssign();
    }
    வெறுமை clearReplaced() { m_replacedVref = பொய்; }
    தருக்க replaced() மாறிலி { திரும்பு m_replacedVref; }
    வெறுமை varUsageReplace(AstVarScope* nodep, AstVarRef* varrefp) {
        // Variable rvalue.  If it references a constant, we can replace it
        LifeVarEntry& entr = m_map[nodep];
        எனில் (entr.isNew()) {
            entr.init(பொய்);
        } இல்லையெனில் {
            எனில் (AstConst* மாறிலி constp = entr.constNodep()) {
                எனில் (!varrefp->varp()->isSigPublic() && !varrefp->varp()->isWrittenByDpi()
                    && !varrefp->varp()->isVirtIface()) {
                    // Aha, variable is constant; substitute in.
                    // We'll later constant propagate
                    UINFO(4, "     replaceconst: " << varrefp);
                    UINFO(9, "     replaceval: " << constp);
                    varrefp->replaceWith(constp->clone_மரம்(பொய்));
                    m_replacedVref = உண்மை;
                    VL_DO_DANGLING(varrefp->delete_மரம்(), varrefp);
                    ++m_statep->m_statAssnCon;
                    திரும்பு;  // **DONE, no longer a var reference**
                }
            }
            UINFO(4, "     usage: " << nodep);
        }
        entr.consumed();
    }
    வெறுமை complexAssignFind(AstVarScope* nodep) {
        UINFO(4, "     casfind: " << nodep);
        LifeVarEntry& entr = m_map[nodep];
        எனில் (entr.isNew()) entr.init(பொய்);
        entr.complexAssign();
    }
    வெறுமை consumedFind(AstVarScope* nodep) {
        LifeVarEntry& entr = m_map[nodep];
        எனில் (entr.isNew()) entr.init(பொய்);
        entr.consumed();
    }
    வெறுமை lifeToAbove() {
        // Any varrefs under a if/else branch affect statements outside and after the if/else
        UASSERT(m_aboveLifep, "Pushing life when already at the top level");
        எனில் (m_noopt) m_aboveLifep->noopt();
        க்கு (auto& itr : m_map) {
            AstVarScope* மாறிலி nodep = itr.first;
            m_aboveLifep->complexAssignFind(nodep);
            எனில் (itr.second.everSet()) {
                // Record there may be an assignment, so we don't constant propagate across the if.
                complexAssignFind(nodep);
            } இல்லையெனில் {
                // Record consumption, so we don't eliminate earlier assignments
                consumedFind(nodep);
            }
        }
    }
    வெறுமை dualBranch(LifeBlock* life1p, LifeBlock* life2p) {
        // Find any common sets on both branches of IF and propagate upwards
        // life1p->lifeDump();
        // life2p->lifeDump();
        Ast_முனை::user1_Clear_மரம்();  // user1p() used on entire tree
        க்கு (auto& itr : life1p->m_map) {
            // When the if branch sets a var before it's used, mark that variable
            எனில் (itr.second.setBeforeUse()) itr.first->user1(1);
        }
        க்கு (auto& itr : life2p->m_map) {
            // When the else branch sets a var before it's used
            AstVarScope* மாறிலி nodep = itr.first;
            எனில் (itr.second.setBeforeUse() && nodep->user1()) {
                // Both branches set the var, we can remove the assignment before the IF.
                UINFO(4, "DUALBRANCH " << nodep);
                மாறிலி auto itab = m_map.find(nodep);
                எனில் (itab != m_map.end()) checkRemoveAssign(nodep, itab->second);
            }
        }
        // this->lifeDump();
    }
    வெறுமை noopt() {
        // The block above is invalidated at exit, see lifeToAbove()
        m_map.clear();
        m_noopt = உண்மை;
    }
    // DEBUG
    வெறுமை lifeDump() {
        UINFO(5, "  LifeMap:");
        க்கு (மாறிலி auto& itr : m_map) {
            UINFO(5,
                  "     Ent:  " << (itr.second.setBeforeUse() ? "[F]  " : "     ") << itr.first);
            எனில் (itr.second.assignp()) {  //
                UINFO(5, "       Ass: " << itr.second.assignp());
            }
        }
    }
};

//######################################################################
// Life state, as a visitor of each AstNode

வகுப்பு LifeVisitor final : பொது VNVisitor {
    // STATE
    LifeState* மாறிலி m_statep;  // Current state
    தருக்க m_containsTiming = பொய்;  // Statement contains timing control
    தருக்க m_sideEffect = பொய்;  // Side effects discovered in assign RHS
    தருக்க m_noopt = பொய்;  // Disable optimization of variables in this block
    தருக்க m_tracingCall = பொய்;  // Iterating into a CCall to a CFunc
    LifeBlock* m_lifep = nullptr;  // Current active lifetime map for current scope

    // METHODS
    வெறுமை setNoopt(மாறிலி எழுத்து* reasonp) {
        (வெறுமை)reasonp;
        // UINFO(9, "setNoopt " << reasonp);
        m_noopt = உண்மை;
        m_lifep->noopt();
    }

    வெறுமை processAssignment(Ast_முனை_Stmt* nodep, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp) {
        // Collect any used variables first, as lhs may also be on rhs
        // Similar code in V3Dead
        VL_RESTORER(m_sideEffect);
        m_sideEffect = பொய்;
        m_lifep->clearReplaced();
        rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp), முனை_கோவை);
        எனில் (m_lifep->replaced()) {
            // We changed something, try to constant propagate, but don't delete the
            // assignment as we still need nodep to remain.
            V3Const::constifyEdit(rhsp);
            VL_DANGLING(rhsp);  // rhsp may change
        }
        // Has to be direct assignment without any EXTRACTing.
        எனில் (VN_IS(lhsp, VarRef) && !m_sideEffect && !m_noopt) {
            AstVarScope* மாறிலி vscp = VN_AS(lhsp, VarRef)->varScopep();
            UASSERT_OBJ(vscp, nodep, "Scope lost on variable");
            m_lifep->simpleAssign(vscp, nodep);
        } இல்லையெனில் {
            iterateAndNextNull(lhsp);
        }
    }

    // VISITORS
    வெறுமை visit(AstVarRef* nodep) override {
        // Consumption/generation of a variable,
        // it's used so can't elim assignment before this use.
        UASSERT_OBJ(nodep->varScopep(), nodep, "nullptr");
        //
        AstVarScope* மாறிலி vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");
        எனில் (nodep->access().isWriteOrRW()) {
            m_sideEffect = உண்மை;  // $sscanf etc may have RHS vars that are lvalues
            m_lifep->complexAssign(vscp);
        } இல்லையெனில் {
            VL_DO_DANGLING(m_lifep->varUsageReplace(vscp, nodep), nodep);
        }
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        எனில் (nodep->isTimingControl() || VN_IS(nodep, AssignForce)) {
            // V3Life doesn't understand time sense nor force assigns - don't optimize
            setNoopt("timing|force");
            எனில் (nodep->isTimingControl()) m_containsTiming = உண்மை;
            iterateChildren(nodep);
            திரும்பு;
        }
        processAssignment(nodep, nodep->lhsp(), nodep->rhsp());
    }
    வெறுமை visit(AstSFormat* nodep) override {
        // AstSFormat is just a special form of assignment
        processAssignment(nodep, nodep->lhsp(), nodep->fmtp());
    }
    வெறுமை visit(AstAssignDly* nodep) override {
        // V3Life doesn't understand time sense
        எனில் (nodep->isTimingControl()) {
            // Don't optimize
            setNoopt("assigndly");
            m_containsTiming = உண்மை;
        }
        // Don't treat as normal assign
        iterateChildren(nodep);
    }

    //---- Track control flow changes
    வெறுமை visit(Ast_முனை_If* nodep) override {
        UINFO(4, "   IF " << nodep);
        // Condition is part of PREVIOUS block
        iterateAndNextNull(nodep->condp());
        LifeBlock* மாறிலி ifLifep = புதிய LifeBlock{m_lifep, m_statep};
        LifeBlock* மாறிலி elseLifep = புதிய LifeBlock{m_lifep, m_statep};
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
    வெறுமை visit(AstLoop* nodep) override {
        // Similar problem to AstJumpBlock, don't optimize loop bodies - most are unrolled
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = புதிய LifeBlock{m_lifep, m_statep};
            // Disable optimization in the body, but don't flag the block:
            // a pure body must not invalidate tracking above
            m_noopt = உண்மை;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinloop");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        எனில் (m_containsTiming) setNoopt("timing");
    }
    வெறுமை visit(AstJumpBlock* nodep) override {
        // As with Loop's we can't predict if a JumpGo will kill us or not
        // It's worse though as an IF(..., JUMPGO) may change the control flow.
        // Just don't optimize blocks with labels; they're rare - so far.
        VL_RESTORER(m_containsTiming);
        {
            VL_RESTORER(m_noopt);
            VL_RESTORER(m_lifep);
            m_lifep = புதிய LifeBlock{m_lifep, m_statep};
            // Structural only, as for AstLoop above
            m_noopt = உண்மை;
            iterateAndNextNull(nodep->stmtsp());
            UINFO(4, "   joinjump");
            // For the next assignments, clear any variables that were read or written in the block
            m_lifep->lifeToAbove();
            VL_DO_DANGLING(delete m_lifep, m_lifep);
        }
        எனில் (m_containsTiming) setNoopt("timing");
    }
    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        // UINFO(4, "  CCALL " << nodep);
        iterateChildren(nodep);
        // Enter the function and trace it
        // else is non-inline or public function we optimize separately
        எனில் (nodep->funcp()->entryPoint()) {
            setNoopt("ccall");
        } இல்லையெனில் {
            m_tracingCall = உண்மை;
            iterate(nodep->funcp());
        }
    }
    வெறுமை visit(AstCFunc* nodep) override {
        // UINFO(4, "  CFUNC " << nodep);
        எனில் (!m_tracingCall && !nodep->entryPoint()) திரும்பு;
        m_tracingCall = பொய்;
        எனில் (nodep->recursive()) setNoopt("recursive");
        எனில் (nodep->noLife()) setNoopt("nolife");
        எனில் (nodep->dpiImportPrototype() && !nodep->dpiPure()) {
            m_sideEffect = உண்மை;  // If appears on assign RHS, don't ever delete the assignment
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCExprUser* nodep) override {
        m_sideEffect = உண்மை;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCExpr* nodep) override {
        m_sideEffect = உண்மை;  // If appears on assign RHS, don't ever delete the assignment
        iterateChildren(nodep);
    }

    வெறுமை visit(AstVar*) override {}  // Don't want varrefs under it
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (nodep->isTimingControl()) {
            // V3Life doesn't understand time sense - don't optimize
            setNoopt("timing");
            m_containsTiming = உண்மை;
        }
        iterateChildren(nodep);
    }

பொது:
    // CONSTRUCTORS
    LifeVisitor(Ast_முனை* nodep, LifeState* statep)
        : m_statep{statep} {
        UINFO(4, "  LifeVisitor on " << nodep);
        {
            m_lifep = புதிய LifeBlock{nullptr, m_statep};
            iterate(nodep);
            எனில் (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
        }
    }
    ~LifeVisitor() override {
        எனில் (m_lifep) VL_DO_CLEAR(delete m_lifep, m_lifep = nullptr);
    }
    VL_UNCOPYABLE(LifeVisitor);
};

//######################################################################

வகுப்பு Life_உச்சி_Visitor final : பொது VNVisitor {
    // Visit all top nodes searching for functions that are entry points we want to start
    // finding code within.
தனிப்பட்ட:
    // STATE
    LifeState* மாறிலி m_statep;  // Current state

    // VISITORS
    வெறுமை visit(AstCFunc* nodep) override {
        எனில் (nodep->entryPoint()) {
            // Usage model 1: Simulate all C code, doing lifetime analysis
            LifeVisitor{nodep, m_statep};
        }
    }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override {
        // Usage model 2: Cleanup basic blocks
        LifeVisitor{nodep, m_statep};
    }
    வெறுமை visit(AstVar*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை_Stmt*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை_கோவை*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    Life_உச்சி_Visitor(AstNetlist* nodep, LifeState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~Life_உச்சி_Visitor() override = இயல்பு;
};

//######################################################################
// Life class functions

வெறுமை V3Life::lifeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        LifeState state;
        Life_உச்சி_Visitor{nodep, &state};
    }  // Destruct before checking
    VIsCached::clear_Cache_மரம்();  // Removing assignments may affect isPure
    V3Global::dump_Check_Global_மரம்("life", 0, dump_மரம்_Either_Level() >= 3);
}
