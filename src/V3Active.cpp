// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break always into sensitivity active domains
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
// V3Active's Transformations:
//
// Note this can be called multiple times.
//   Create a IACTIVE(initial), SACTIVE(combo)
//      ALWAYS: Remove any-edges from sense list
//          If no POS/NEG in senselist, Fold into SACTIVE(combo)
//          Else fold into SACTIVE(sequent).
//          OPTIMIZE: When support async clocks, fold into that active if possible
//      INITIAL: Move into IACTIVE
//      WIRE: Move into SACTIVE(combo)
//
//*************************************************************************

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Active.h"

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Graph.h"

#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

வகுப்பு LatchDetectGraphVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(LatchDetectGraphVertex, V3GraphVertex)
பொது:
    எண்ணிக்கை VertexType : uint8_t { VT_BLOCK, VT_BRANCH, VT_OUTPUT };

தனிப்பட்ட:
    மாறிலி string m_name;  // Only used for .dot file generation
    மாறிலி VertexType m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() மாறிலி VL_MT_SAFE {  //   "
        தேர்வு (m_type) {
        நிலை VT_BLOCK: திரும்பு "(||)";  // basic block node
        நிலை VT_BRANCH: திரும்பு "(&&)";  // if/else branch mode
        நிலை VT_OUTPUT: திரும்பு "(out)";  // var assignment
        இயல்பு: திரும்பு "??";  // unknown
        }
    }

பொது:
    LatchDetectGraphVertex(V3Graph* graphp, மாறிலி string& name, VertexType type = VT_BLOCK)
        : V3GraphVertex{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name + " " + typestr(); }
    string dotColor() மாறிலி override { திரும்பு user() ? "green" : "black"; }
    மெய்நிகர் முழு type() மாறிலி { திரும்பு m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

வகுப்பு LatchDetectGraph final : பொது V3Graph {
பாதுகாக்கப்பட்ட:
    LatchDetectGraphVertex* m_curVertexp = nullptr;  // Current latch detection graph vertex
    std::vector<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    நிலையான LatchDetectGraphVertex* castVertexp(வெறுமை* vertexp) {
        திரும்பு reinterpret_cast<LatchDetectGraphVertex*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    தருக்க latchCheckInternal(LatchDetectGraphVertex* vertexp) {
        தருக்க result = பொய்;
        தேர்வு (vertexp->type()) {
        நிலை LatchDetectGraphVertex::VT_OUTPUT:  // Base case
            result = vertexp->user();
            நிறுத்து;
        நிலை LatchDetectGraphVertex::VT_BLOCK:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            க்கு (V3GraphEdge& edge : vertexp->outEdges()) {
                எனில் (latchCheckInternal(castVertexp(edge.உச்சி()))) {
                    result = உண்மை;
                    நிறுத்து;
                }
            }
            நிறுத்து;
        நிலை LatchDetectGraphVertex::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            LatchDetectGraphVertex* மாறிலி ifp = castVertexp(vertexp->outEdges().frontp()->உச்சி());
            LatchDetectGraphVertex* மாறிலி elsp = castVertexp(vertexp->outEdges().backp()->உச்சி());
            result = latchCheckInternal(ifp) && latchCheckInternal(elsp);
            நிறுத்து;
        }
        vertexp->user(result);
        திரும்பு result;
    }

பொது:
    LatchDetectGraph() { clear(); }
    ~LatchDetectGraph() override { clear(); }
    // ACCESSORS
    LatchDetectGraphVertex* currentp() { திரும்பு m_curVertexp; }
    வெறுமை currentp(LatchDetectGraphVertex* vertex) { m_curVertexp = vertex; }
    // METHODS
    வெறுமை begin() {
        // Start a new if/else tracking graph
        m_curVertexp = புதிய LatchDetectGraphVertex{இது, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    வெறுமை clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3Graph::clear();
        m_curVertexp = nullptr;
    }
    // Add a new control path and connect it to its parent
    LatchDetectGraphVertex* addPathVertex(LatchDetectGraphVertex* parent, மாறிலி string& name,
                                          தருக்க branch = பொய்) {
        m_curVertexp = புதிய LatchDetectGraphVertex{இது, name,
                                                  branch ? LatchDetectGraphVertex::VT_BRANCH
                                                         : LatchDetectGraphVertex::VT_BLOCK};
        புதிய V3GraphEdge{இது, parent, m_curVertexp, 1};
        திரும்பு m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    LatchDetectGraphVertex* addOutputVertex(AstVarRef* nodep) {
        LatchDetectGraphVertex* மாறிலி outVertexp
            = புதிய LatchDetectGraphVertex{இது, nodep->name(), LatchDetectGraphVertex::VT_OUTPUT};
        nodep->varp()->user1p(outVertexp);
        m_outputs.தள்ளு_back(nodep);
        திரும்பு outVertexp;
    }
    // Connect an output assignment to its parent control block
    வெறுமை addAssignment(AstVarRef* nodep) {
        LatchDetectGraphVertex* outVertexp;
        எனில் (!nodep->varp()->user1p()) {  // Not seen this output before
            outVertexp = addOutputVertex(nodep);
        } இல்லையெனில் {
            outVertexp = castVertexp(nodep->varp()->user1p());
        }
        புதிய V3GraphEdge{இது, m_curVertexp, outVertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    வெறுமை latchCheck(Ast_முனை* nodep, தருக்க latch_expected) {
        தருக்க latch_detected = பொய்;
        க்கு (மாறிலி AstVarRef* மாறிலி vrp : m_outputs) {
            LatchDetectGraphVertex* மாறிலி vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(உண்மை);  // Identify the output vertex we are checking paths _to_
            எனில் (!latchCheckInternal(castVertexp(vertices().frontp()))) latch_detected = உண்மை;
            எனில் (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warnMore()
                        << "... Suggest use of always_latch for intentional latches");
                எனில் (dumpGraphLevel() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(பொய்);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        எனில் (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

வகுப்பு ActiveNamer final : பொது VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope to add statement to
    AstActive* m_sActivep = nullptr;  // For current scope, the Static active we're building
    AstActive* m_iActivep = nullptr;  // For current scope, the Initial active we're building
    AstActive* m_fActivep = nullptr;  // For current scope, the Final active we're building
    AstActive* m_cActivep = nullptr;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_மரம்>, AstActive*> m_activeMap;

    // METHODS
    வெறுமை addActive(AstActive* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->addBlocksp(nodep);
    }

    // VISITORS
    வெறுமை visit(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = nullptr;
        m_iActivep = nullptr;
        m_fActivep = nullptr;
        m_cActivep = nullptr;
        m_activeMap.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    வெறுமை visit(Ast_Sen_மரம்* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    வெறுமை visit(Ast_முனை_Stmt*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    template <typename T_SenItemKind>
    AstActive*& getSpecialActive();

பொது:
    // METHODS
    AstScope* scopep() { திரும்பு m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    AstActive* makeActive(File_வரி* மாறிலி fl, Ast_Sen_மரம்* மாறிலி senTreep) {
        AstActive* மாறிலி activep = புதிய AstActive{fl, "", senTreep};
        activep->sen_மரம்_Storep(activep->sentreep());
        addActive(activep);
        திரும்பு activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    template <typename T_SenItemKind>
    AstActive* makeSpecialActive(File_வரி* மாறிலி fl) {
        Ast_Sen_மரம்* மாறிலி senTreep = புதிய Ast_Sen_மரம்{fl, புதிய AstSenItem{fl, T_SenItemKind{}}};
        திரும்பு makeActive(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    template <typename T_SenItemKind>
    AstActive* getSpecialActive(File_வரி* fl) {
        AstActive*& cachep = getSpecialActive<T_SenItemKind>();
        எனில் (!cachep) cachep = makeSpecialActive<T_SenItemKind>(fl);
        திரும்பு cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    AstActive* getActive(File_வரி* fl, Ast_Sen_மரம்* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        auto it = m_activeMap.find(*sentreep);
        // If found matching AstActive, return it
        எனில் (it != m_activeMap.end()) திரும்பு it->second;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_மரம்* மாறிலி newsenp = sentreep->clone_மரம்(பொய்);
        AstActive* மாறிலி activep = புதிய AstActive{fl, "sequent", newsenp};
        activep->sen_மரம்_Storep(activep->sentreep());
        addActive(activep);
        m_activeMap.emplace(*newsenp, activep);
        திரும்பு activep;
    }

    // CONSTRUCTORS
    ActiveNamer() = இயல்பு;
    ~ActiveNamer() override = இயல்பு;
    வெறுமை முதன்மை(AstScope* nodep) { iterate(nodep); }
};

template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Static>() {
    திரும்பு m_sActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Initial>() {
    திரும்பு m_iActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Final>() {
    திரும்பு m_fActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Combo>() {
    திரும்பு m_cActivep;
}

//######################################################################
// Latch checking visitor

வகுப்பு ActiveLatchCheckVisitor final : பொது VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    மாறிலி VNUser1InUse m_inuser1;
    // STATE
    LatchDetectGraph m_graph;  // Graph used to detect latches in combo always
    // VISITORS
    வெறுமை visit(AstVarRef* nodep) override {
        மாறிலி AstVar* மாறிலி varp = nodep->varp();
        எனில் (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->isUsedLoopIdx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_graph.addAssignment(nodep);
        }
    }
    வெறுமை visit(Ast_முனை_If* nodep) override {
        எனில் (!nodep->isBoundsCheck()) {
            LatchDetectGraphVertex* மாறிலி parentp = m_graph.currentp();
            LatchDetectGraphVertex* மாறிலி branchp = m_graph.addPathVertex(parentp, "BRANCH", உண்மை);
            m_graph.addPathVertex(branchp, "IF");
            iterateAndNextConstNull(nodep->thensp());
            m_graph.addPathVertex(branchp, "ELSE");
            iterateAndNextConstNull(nodep->elsesp());
            m_graph.currentp(parentp);
        } இல்லையெனில் {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    // CONSTRUCTORS
    ActiveLatchCheckVisitor(Ast_முனை* nodep, தருக்க expectLatch) {
        m_graph.begin();
        iterateConst(nodep);
        m_graph.latchCheck(nodep, expectLatch);
    }
    ~ActiveLatchCheckVisitor() override = இயல்பு;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

வகுப்பு ActiveDlyVisitor final : பொது VNVisitor {
பொது:
    எண்ணிக்கை CheckType : uint8_t { CT_COMB, CT_FINAL };

தனிப்பட்ட:
    // MEMBERS
    மாறிலி CheckType m_check;  // Process type we are checking

    // VISITORS
    வெறுமை visit(AstAssignDly* nodep) override {
        // Issue appropriate warning
        எனில் (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } இல்லையெனில் {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warnMore()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(புதிய AstAssign{
            nodep->fileline(),  //
            nodep->lhsp()->unlinkFrBack(),  //
            nodep->rhsp()->unlinkFrBack(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlinkFrBack() : nullptr});
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }

    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    ActiveDlyVisitor(Ast_முனை* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ActiveDlyVisitor() override = இயல்பு;
};

//######################################################################
// Active class functions

வகுப்பு ActiveVisitor final : பொது VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ActiveNamer m_namer;  // Tracking of active names
    தருக்க m_clockedProcess = பொய்;  // Whether current process is a clocked process
    தருக்க m_allChanged = பொய்;  // Whether all SenItem in the SenTree are ET_CHANGED
    தருக்க m_walkingBody = பொய்;  // Walking body of a process
    தருக்க m_canBeComb = பொய்;  // Whether current clocked process can be turned into a comb process

    // METHODS
    template <typename T>
    வெறுமை moveUnderSpecial(Ast_முனை* nodep) {
        AstActive* மாறிலி wantactivep = m_namer.getSpecialActive<T>(nodep->fileline());
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);
    }

    வெறுமை visitAlways(Ast_முனை* nodep, Ast_Sen_மரம்* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        எனில் (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            திரும்பு;
        }

        {
            மாறிலி VNUser1InUse user1InUse;

            // Walk sensitivity list
            m_clockedProcess = பொய்;
            m_allChanged = உண்மை;
            எனில் (oldsentreep) {
                oldsentreep->unlinkFrBack();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            எனில் (m_allChanged) {
                மாறிலி VNUser2InUse user2InUse;
                m_walkingBody = உண்மை;
                m_canBeComb = உண்மை;
                iterateChildrenConst(nodep);
                m_walkingBody = பொய்;
                எனில் (m_canBeComb) m_clockedProcess = பொய்;
            }
        }

        AstActive* மாறிலி wantactivep
            = !m_clockedProcess ? m_namer.getSpecialActive<AstSenItem::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.getActive(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.getSpecialActive<AstSenItem::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);

        // Warn and convert any delayed assignments
        எனில் (!m_clockedProcess) ActiveDlyVisitor{nodep, ActiveDlyVisitor::CT_COMB};

        // Delete sensitivity list
        எனில் (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_மரம்(), oldsentreep);

        // check combinational processes for latches
        எனில் (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            மாறிலி ActiveLatchCheckVisitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    வெறுமை visit_Sen_பொருட்கள்(Ast_முனை* nodep) {
        எனில் (v3Global.opt.timing().isSetTrue()) {
            nodep->foreach([இது](AstSenItem* senItemp) { visit(senItemp); });
        }
    }

    நிலையான வெறுமை markEventEdges(Ast_Sen_மரம்* sentreep) {
        க்கு (AstSenItem* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), SenItem)) {
            எனில் (!senip->sensp()) தொடர்;
            எனில் (மாறிலி Ast_முனை_DType* மாறிலி dtypep = senip->sensp()->dtypep()) {
                எனில் (மாறிலி AstBasicDType* மாறிலி basicp = dtypep->basicp()) {
                    எனில் (basicp->isEvent()) senip->edgeType(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    வெறுமை visit(AstScope* nodep) override {
        m_namer.முதன்மை(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    வெறுமை visit(AstActive* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    வெறுமை visit(AstInitialStatic* nodep) override { moveUnderSpecial<AstSenItem::Static>(nodep); }
    வெறுமை visit(AstInitial* nodep) override {
        visit_Sen_பொருட்கள்(nodep);
        moveUnderSpecial<AstSenItem::Initial>(nodep);
    }
    வெறுமை visit(AstFinal* nodep) override {
        மாறிலி ActiveDlyVisitor dlyvisitor{nodep, ActiveDlyVisitor::CT_FINAL};
        moveUnderSpecial<AstSenItem::Final>(nodep);
    }
    வெறுமை visit(AstCoverToggle* nodep) override { moveUnderSpecial<AstSenItem::Combo>(nodep); }
    வெறுமை visit(AstAlways* nodep) override {
        எனில் (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            திரும்பு;
        }
        எனில் (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            moveUnderSpecial<AstSenItem::Combo>(nodep);
            திரும்பு;
        }
        visit_Sen_பொருட்கள்(nodep);
        visitAlways(nodep, nodep->sentreep(), nodep->keyword());
    }
    வெறுமை visit(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* மாறிலி activep = m_namer.makeSpecialActive<AstSenItem::Combo>(nodep->fileline());
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    வெறுமை visit(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_மரம்* மாறிலி sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* மாறிலி activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    வெறுமை visit(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_மரம்* மாறிலி sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* மாறிலி activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    வெறுமை visit(AstCFunc* nodep) override { visit_Sen_பொருட்கள்(nodep); }
    வெறுமை visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        எனில் (!nodep->sensp()) திரும்பு;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = உண்மை;
        எனில் (nodep->edgeType() != VEdgeType::ET_CHANGED) m_allChanged = பொய்;

        எனில் (மாறிலி Ast_முனை_DType* மாறிலி dtypep = nodep->sensp()->dtypep()) {
            எனில் (மாறிலி AstBasicDType* மாறிலி basicp = dtypep->basicp()) {
                எனில் (basicp->isEvent()) nodep->edgeType(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](மாறிலி AstVarRef* refp) { refp->varScopep()->user1(உண்மை); });
    }

    வெறுமை visit(AstVarRef* nodep) override {
        AstVarScope* மாறிலி vscp = nodep->varScopep();
        எனில் (nodep->access().isWriteOnly()) {
            vscp->user2(உண்மை);
        } இல்லையெனில் {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            மாறிலி AstVar* மாறிலி varp = vscp->varp();
            எனில் (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = பொய்;
            }
        }
    }
    வெறுமை visit(AstAssignDly* nodep) override {
        m_canBeComb = பொய்;
        எனில் (nodep->isTimingControl()) m_clockedProcess = உண்மை;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstFireEvent* nodep) override {
        m_canBeComb = பொய்;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstAssignForce* nodep) override {
        m_canBeComb = பொய்;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstRelease* nodep) override {
        m_canBeComb = பொய்;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstFork* nodep) override {
        எனில் (nodep->isTimingControl()) {
            m_canBeComb = பொய்;
            m_clockedProcess = உண்மை;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    வெறுமை visit(AstVar*) override {}  // Accelerate
    வெறுமை visit(AstVarScope*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (!v3Global.opt.timing().isSetTrue() && m_walkingBody && !m_canBeComb) {
            திரும்பு;  // Accelerate
        }
        எனில் (!nodep->isPure()) m_canBeComb = பொய்;
        எனில் (nodep->isTimingControl()) {
            m_canBeComb = பொய்;
            m_clockedProcess = உண்மை;
            திரும்பு;
        }
        iterateChildren(nodep);
    }

பொது:
    // CONSTRUCTORS
    explicit ActiveVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ActiveVisitor() override = இயல்பு;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

வகுப்பு CovergroupCollectVisitor final : பொது VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = nullptr;  // Current covergroup class context, or nullptr

    // VISITORS
    வெறுமை visit(AstClass* nodep) override {
        எனில் (!nodep->isCovergroup()) திரும்பு;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    வெறுமை visit(AstScope* nodep) override { iterateChildren(nodep); }

    வெறுமை visit(AstCFunc* nodep) override {
        எனில் (!m_classp) திரும்பு;
        எனில் (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    வெறுமை visit(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlinkFrBack());
        nodep->unlinkFrBack();
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = இயல்பு;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

வகுப்பு CovergroupEventBindVisitor final : பொது VNVisitor {
    AstVarScope* மாறிலி m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_set<மாறிலி AstVar*> m_memberps;  // Non-static variables in the covergroup class

    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (!m_memberps.மொத்தம்(nodep->varp())) திரும்பு;
        File_வரி* மாறிலி fl = nodep->fileline();
        AstMemberSel* மாறிலி selp
            = புதிய AstMemberSel{fl, புதிய AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    CovergroupEventBindVisitor(Ast_Sen_மரம்* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* மாறிலி, AstVar* மாறிலி varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = இயல்பு;
};

வகுப்பு CovergroupInjectVisitor final : பொது VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    ActiveNamer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    வெறுமை visit(AstScope* nodep) override {
        m_namer.முதன்மை(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    வெறுமை visit(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* மாறிலி varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        மாறிலி Ast_முனை_DType* மாறிலி dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        மாறிலி AstClassRefDType* மாறிலி classRefp = VN_CAST(dtypep, ClassRefDType);
        எனில் (!classRefp) திரும்பு;

        AstClass* மாறிலி classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_மரம்* மாறிலி eventp = VN_CAST(classp->user2p(), Sen_மரம்);
        எனில் (!eventp) திரும்பு;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* மாறிலி sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_வரி* மாறிலி fl = nodep->fileline();
        AstVarRef* மாறிலி varrefp = புதிய AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* மாறிலி cmethodCallp
            = புதிய AstCMethodCall{fl, varrefp, sampleCFuncp, nullptr};

        cmethodCallp->dtypeSetVoid();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_மரம்* senTreep = eventp->clone_மரம்(பொய்);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        AstActive* மாறிலி activep = m_namer.getActive(fl, senTreep);
        VL_DO_DANGLING(தள்ளு_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->addStmtsp(
            புதிய AstAlways{fl, VAlwaysKwd::ALWAYS_FF, nullptr, cmethodCallp->makeStmt()});
    }

    வெறுமை visit(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        எனில் (Ast_Sen_மரம்* மாறிலி eventp = VN_CAST(nodep->user2p(), Sen_மரம்)) {
            VL_DO_DANGLING(தள்ளு_Deletep(eventp), eventp);
        }
    }

    வெறுமை visit(AstActive*) override {}  // Don't iterate into actives
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = இயல்பு;
};

//######################################################################
// Active class functions

வெறுமை V3Active::activeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ActiveVisitor{nodep}; }  // Destruct before checking
    எனில் (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        மாறிலி VNUser1InUse user1InUse;
        மாறிலி VNUser2InUse user2InUse;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_மரம்("active", 0, dump_மரம்_Either_Level() >= 3);
}
