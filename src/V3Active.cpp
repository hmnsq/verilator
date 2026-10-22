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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Active.h"

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Graph.h"

#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

વર્ગ LatchDetectGraphVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(LatchDetectGraphVertex, V3GraphVertex)
જાહેર:
    ગણના VertexType : uint8_t { VT_BLOCK, VT_BRANCH, VT_OUTPUT };

ખાનગી:
    અચલ string m_name;  // Only used for .dot file generation
    અચલ VertexType m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() અચલ VL_MT_SAFE {  //   "
        પસંદગી (m_type) {
        સ્થિતિ VT_BLOCK: પાછા "(||)";  // basic block node
        સ્થિતિ VT_BRANCH: પાછા "(&&)";  // if/else branch mode
        સ્થિતિ VT_OUTPUT: પાછા "(out)";  // var assignment
        મૂળભૂત: પાછા "??";  // unknown
        }
    }

જાહેર:
    LatchDetectGraphVertex(V3Graph* graphp, અચલ string& name, VertexType type = VT_BLOCK)
        : V3GraphVertex{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() અચલ override VL_MT_STABLE { પાછા m_name + " " + typestr(); }
    string dotColor() અચલ override { પાછા user() ? "green" : "black"; }
    આભાસી પૂર્ણાંક type() અચલ { પાછા m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

વર્ગ LatchDetectGraph final : જાહેર V3Graph {
સુરક્ષિત:
    LatchDetectGraphVertex* m_curVertexp = nullptr;  // Current latch detection graph vertex
    std::vector<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    સ્થિર LatchDetectGraphVertex* castVertexp(શૂન્ય* vertexp) {
        પાછા reinterpret_cast<LatchDetectGraphVertex*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    તાર્કિક latchCheckInternal(LatchDetectGraphVertex* vertexp) {
        તાર્કિક result = ખોટું;
        પસંદગી (vertexp->type()) {
        સ્થિતિ LatchDetectGraphVertex::VT_OUTPUT:  // Base case
            result = vertexp->user();
            તોડો;
        સ્થિતિ LatchDetectGraphVertex::VT_BLOCK:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            માટે (V3GraphEdge& edge : vertexp->outEdges()) {
                જો (latchCheckInternal(castVertexp(edge.ટોચ()))) {
                    result = સાચું;
                    તોડો;
                }
            }
            તોડો;
        સ્થિતિ LatchDetectGraphVertex::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            LatchDetectGraphVertex* અચલ ifp = castVertexp(vertexp->outEdges().frontp()->ટોચ());
            LatchDetectGraphVertex* અચલ elsp = castVertexp(vertexp->outEdges().backp()->ટોચ());
            result = latchCheckInternal(ifp) && latchCheckInternal(elsp);
            તોડો;
        }
        vertexp->user(result);
        પાછા result;
    }

જાહેર:
    LatchDetectGraph() { clear(); }
    ~LatchDetectGraph() override { clear(); }
    // ACCESSORS
    LatchDetectGraphVertex* currentp() { પાછા m_curVertexp; }
    શૂન્ય currentp(LatchDetectGraphVertex* vertex) { m_curVertexp = vertex; }
    // METHODS
    શૂન્ય begin() {
        // Start a new if/else tracking graph
        m_curVertexp = નવું LatchDetectGraphVertex{આ, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    શૂન્ય clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3Graph::clear();
        m_curVertexp = nullptr;
    }
    // Add a new control path and connect it to its parent
    LatchDetectGraphVertex* addPathVertex(LatchDetectGraphVertex* parent, અચલ string& name,
                                          તાર્કિક branch = ખોટું) {
        m_curVertexp = નવું LatchDetectGraphVertex{આ, name,
                                                  branch ? LatchDetectGraphVertex::VT_BRANCH
                                                         : LatchDetectGraphVertex::VT_BLOCK};
        નવું V3GraphEdge{આ, parent, m_curVertexp, 1};
        પાછા m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    LatchDetectGraphVertex* addOutputVertex(AstVarRef* nodep) {
        LatchDetectGraphVertex* અચલ outVertexp
            = નવું LatchDetectGraphVertex{આ, nodep->name(), LatchDetectGraphVertex::VT_OUTPUT};
        nodep->varp()->user1p(outVertexp);
        m_outputs.ધકેલો_back(nodep);
        પાછા outVertexp;
    }
    // Connect an output assignment to its parent control block
    શૂન્ય addAssignment(AstVarRef* nodep) {
        LatchDetectGraphVertex* outVertexp;
        જો (!nodep->varp()->user1p()) {  // Not seen this output before
            outVertexp = addOutputVertex(nodep);
        } નહીંતર {
            outVertexp = castVertexp(nodep->varp()->user1p());
        }
        નવું V3GraphEdge{આ, m_curVertexp, outVertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    શૂન્ય latchCheck(Ast_ગાંઠ* nodep, તાર્કિક latch_expected) {
        તાર્કિક latch_detected = ખોટું;
        માટે (અચલ AstVarRef* અચલ vrp : m_outputs) {
            LatchDetectGraphVertex* અચલ vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(સાચું);  // Identify the output vertex we are checking paths _to_
            જો (!latchCheckInternal(castVertexp(vertices().frontp()))) latch_detected = સાચું;
            જો (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warnMore()
                        << "... Suggest use of always_latch for intentional latches");
                જો (dumpGraphLevel() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(ખોટું);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        જો (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

વર્ગ ActiveNamer final : જાહેર VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope to add statement to
    AstActive* m_sActivep = nullptr;  // For current scope, the Static active we're building
    AstActive* m_iActivep = nullptr;  // For current scope, the Initial active we're building
    AstActive* m_fActivep = nullptr;  // For current scope, the Final active we're building
    AstActive* m_cActivep = nullptr;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_વૃક્ષ>, AstActive*> m_activeMap;

    // METHODS
    શૂન્ય addActive(AstActive* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->addBlocksp(nodep);
    }

    // VISITORS
    શૂન્ય visit(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = nullptr;
        m_iActivep = nullptr;
        m_fActivep = nullptr;
        m_cActivep = nullptr;
        m_activeMap.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    શૂન્ય visit(Ast_Sen_વૃક્ષ* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    શૂન્ય visit(Ast_ગાંઠ_Stmt*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    template <typename T_SenItemKind>
    AstActive*& getSpecialActive();

જાહેર:
    // METHODS
    AstScope* scopep() { પાછા m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    AstActive* makeActive(File_લીટી* અચલ fl, Ast_Sen_વૃક્ષ* અચલ senTreep) {
        AstActive* અચલ activep = નવું AstActive{fl, "", senTreep};
        activep->sen_વૃક્ષ_Storep(activep->sentreep());
        addActive(activep);
        પાછા activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    template <typename T_SenItemKind>
    AstActive* makeSpecialActive(File_લીટી* અચલ fl) {
        Ast_Sen_વૃક્ષ* અચલ senTreep = નવું Ast_Sen_વૃક્ષ{fl, નવું AstSenItem{fl, T_SenItemKind{}}};
        પાછા makeActive(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    template <typename T_SenItemKind>
    AstActive* getSpecialActive(File_લીટી* fl) {
        AstActive*& cachep = getSpecialActive<T_SenItemKind>();
        જો (!cachep) cachep = makeSpecialActive<T_SenItemKind>(fl);
        પાછા cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    AstActive* getActive(File_લીટી* fl, Ast_Sen_વૃક્ષ* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        auto it = m_activeMap.find(*sentreep);
        // If found matching AstActive, return it
        જો (it != m_activeMap.end()) પાછા it->second;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_વૃક્ષ* અચલ newsenp = sentreep->clone_વૃક્ષ(ખોટું);
        AstActive* અચલ activep = નવું AstActive{fl, "sequent", newsenp};
        activep->sen_વૃક્ષ_Storep(activep->sentreep());
        addActive(activep);
        m_activeMap.emplace(*newsenp, activep);
        પાછા activep;
    }

    // CONSTRUCTORS
    ActiveNamer() = મૂળભૂત;
    ~ActiveNamer() override = મૂળભૂત;
    શૂન્ય મુખ્ય(AstScope* nodep) { iterate(nodep); }
};

template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Static>() {
    પાછા m_sActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Initial>() {
    પાછા m_iActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Final>() {
    પાછા m_fActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Combo>() {
    પાછા m_cActivep;
}

//######################################################################
// Latch checking visitor

વર્ગ ActiveLatchCheckVisitor final : જાહેર VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    અચલ VNUser1InUse m_inuser1;
    // STATE
    LatchDetectGraph m_graph;  // Graph used to detect latches in combo always
    // VISITORS
    શૂન્ય visit(AstVarRef* nodep) override {
        અચલ AstVar* અચલ varp = nodep->varp();
        જો (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->isUsedLoopIdx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_graph.addAssignment(nodep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        જો (!nodep->isBoundsCheck()) {
            LatchDetectGraphVertex* અચલ parentp = m_graph.currentp();
            LatchDetectGraphVertex* અચલ branchp = m_graph.addPathVertex(parentp, "BRANCH", સાચું);
            m_graph.addPathVertex(branchp, "IF");
            iterateAndNextConstNull(nodep->thensp());
            m_graph.addPathVertex(branchp, "ELSE");
            iterateAndNextConstNull(nodep->elsesp());
            m_graph.currentp(parentp);
        } નહીંતર {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    // CONSTRUCTORS
    ActiveLatchCheckVisitor(Ast_ગાંઠ* nodep, તાર્કિક expectLatch) {
        m_graph.begin();
        iterateConst(nodep);
        m_graph.latchCheck(nodep, expectLatch);
    }
    ~ActiveLatchCheckVisitor() override = મૂળભૂત;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

વર્ગ ActiveDlyVisitor final : જાહેર VNVisitor {
જાહેર:
    ગણના CheckType : uint8_t { CT_COMB, CT_FINAL };

ખાનગી:
    // MEMBERS
    અચલ CheckType m_check;  // Process type we are checking

    // VISITORS
    શૂન્ય visit(AstAssignDly* nodep) override {
        // Issue appropriate warning
        જો (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } નહીંતર {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warnMore()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(નવું AstAssign{
            nodep->fileline(),  //
            nodep->lhsp()->unlinkFrBack(),  //
            nodep->rhsp()->unlinkFrBack(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlinkFrBack() : nullptr});
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }

    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    ActiveDlyVisitor(Ast_ગાંઠ* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ActiveDlyVisitor() override = મૂળભૂત;
};

//######################################################################
// Active class functions

વર્ગ ActiveVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ActiveNamer m_namer;  // Tracking of active names
    તાર્કિક m_clockedProcess = ખોટું;  // Whether current process is a clocked process
    તાર્કિક m_allChanged = ખોટું;  // Whether all SenItem in the SenTree are ET_CHANGED
    તાર્કિક m_walkingBody = ખોટું;  // Walking body of a process
    તાર્કિક m_canBeComb = ખોટું;  // Whether current clocked process can be turned into a comb process

    // METHODS
    template <typename T>
    શૂન્ય moveUnderSpecial(Ast_ગાંઠ* nodep) {
        AstActive* અચલ wantactivep = m_namer.getSpecialActive<T>(nodep->fileline());
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);
    }

    શૂન્ય visitAlways(Ast_ગાંઠ* nodep, Ast_Sen_વૃક્ષ* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        જો (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            પાછા;
        }

        {
            અચલ VNUser1InUse user1InUse;

            // Walk sensitivity list
            m_clockedProcess = ખોટું;
            m_allChanged = સાચું;
            જો (oldsentreep) {
                oldsentreep->unlinkFrBack();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            જો (m_allChanged) {
                અચલ VNUser2InUse user2InUse;
                m_walkingBody = સાચું;
                m_canBeComb = સાચું;
                iterateChildrenConst(nodep);
                m_walkingBody = ખોટું;
                જો (m_canBeComb) m_clockedProcess = ખોટું;
            }
        }

        AstActive* અચલ wantactivep
            = !m_clockedProcess ? m_namer.getSpecialActive<AstSenItem::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.getActive(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.getSpecialActive<AstSenItem::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);

        // Warn and convert any delayed assignments
        જો (!m_clockedProcess) ActiveDlyVisitor{nodep, ActiveDlyVisitor::CT_COMB};

        // Delete sensitivity list
        જો (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_વૃક્ષ(), oldsentreep);

        // check combinational processes for latches
        જો (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            અચલ ActiveLatchCheckVisitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    શૂન્ય visit_Sen_વસ્તુઓ(Ast_ગાંઠ* nodep) {
        જો (v3Global.opt.timing().isSetTrue()) {
            nodep->foreach([આ](AstSenItem* senItemp) { visit(senItemp); });
        }
    }

    સ્થિર શૂન્ય markEventEdges(Ast_Sen_વૃક્ષ* sentreep) {
        માટે (AstSenItem* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), SenItem)) {
            જો (!senip->sensp()) ચાલુ;
            જો (અચલ Ast_ગાંઠ_DType* અચલ dtypep = senip->sensp()->dtypep()) {
                જો (અચલ AstBasicDType* અચલ basicp = dtypep->basicp()) {
                    જો (basicp->isEvent()) senip->edgeType(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    શૂન્ય visit(AstScope* nodep) override {
        m_namer.મુખ્ય(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstActive* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    શૂન્ય visit(AstInitialStatic* nodep) override { moveUnderSpecial<AstSenItem::Static>(nodep); }
    શૂન્ય visit(AstInitial* nodep) override {
        visit_Sen_વસ્તુઓ(nodep);
        moveUnderSpecial<AstSenItem::Initial>(nodep);
    }
    શૂન્ય visit(AstFinal* nodep) override {
        અચલ ActiveDlyVisitor dlyvisitor{nodep, ActiveDlyVisitor::CT_FINAL};
        moveUnderSpecial<AstSenItem::Final>(nodep);
    }
    શૂન્ય visit(AstCoverToggle* nodep) override { moveUnderSpecial<AstSenItem::Combo>(nodep); }
    શૂન્ય visit(AstAlways* nodep) override {
        જો (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            પાછા;
        }
        જો (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            moveUnderSpecial<AstSenItem::Combo>(nodep);
            પાછા;
        }
        visit_Sen_વસ્તુઓ(nodep);
        visitAlways(nodep, nodep->sentreep(), nodep->keyword());
    }
    શૂન્ય visit(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* અચલ activep = m_namer.makeSpecialActive<AstSenItem::Combo>(nodep->fileline());
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    શૂન્ય visit(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_વૃક્ષ* અચલ sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* અચલ activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    શૂન્ય visit(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_વૃક્ષ* અચલ sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* અચલ activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    શૂન્ય visit(AstCFunc* nodep) override { visit_Sen_વસ્તુઓ(nodep); }
    શૂન્ય visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        જો (!nodep->sensp()) પાછા;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = સાચું;
        જો (nodep->edgeType() != VEdgeType::ET_CHANGED) m_allChanged = ખોટું;

        જો (અચલ Ast_ગાંઠ_DType* અચલ dtypep = nodep->sensp()->dtypep()) {
            જો (અચલ AstBasicDType* અચલ basicp = dtypep->basicp()) {
                જો (basicp->isEvent()) nodep->edgeType(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](અચલ AstVarRef* refp) { refp->varScopep()->user1(સાચું); });
    }

    શૂન્ય visit(AstVarRef* nodep) override {
        AstVarScope* અચલ vscp = nodep->varScopep();
        જો (nodep->access().isWriteOnly()) {
            vscp->user2(સાચું);
        } નહીંતર {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            અચલ AstVar* અચલ varp = vscp->varp();
            જો (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = ખોટું;
            }
        }
    }
    શૂન્ય visit(AstAssignDly* nodep) override {
        m_canBeComb = ખોટું;
        જો (nodep->isTimingControl()) m_clockedProcess = સાચું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstFireEvent* nodep) override {
        m_canBeComb = ખોટું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstAssignForce* nodep) override {
        m_canBeComb = ખોટું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstRelease* nodep) override {
        m_canBeComb = ખોટું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstFork* nodep) override {
        જો (nodep->isTimingControl()) {
            m_canBeComb = ખોટું;
            m_clockedProcess = સાચું;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    શૂન્ય visit(AstVar*) override {}  // Accelerate
    શૂન્ય visit(AstVarScope*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (!v3Global.opt.timing().isSetTrue() && m_walkingBody && !m_canBeComb) {
            પાછા;  // Accelerate
        }
        જો (!nodep->isPure()) m_canBeComb = ખોટું;
        જો (nodep->isTimingControl()) {
            m_canBeComb = ખોટું;
            m_clockedProcess = સાચું;
            પાછા;
        }
        iterateChildren(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    explicit ActiveVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ActiveVisitor() override = મૂળભૂત;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

વર્ગ CovergroupCollectVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = nullptr;  // Current covergroup class context, or nullptr

    // VISITORS
    શૂન્ય visit(AstClass* nodep) override {
        જો (!nodep->isCovergroup()) પાછા;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstScope* nodep) override { iterateChildren(nodep); }

    શૂન્ય visit(AstCFunc* nodep) override {
        જો (!m_classp) પાછા;
        જો (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    શૂન્ય visit(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlinkFrBack());
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = મૂળભૂત;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

વર્ગ CovergroupEventBindVisitor final : જાહેર VNVisitor {
    AstVarScope* અચલ m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_set<અચલ AstVar*> m_memberps;  // Non-static variables in the covergroup class

    શૂન્ય visit(AstVarRef* nodep) override {
        જો (!m_memberps.ગણતરી(nodep->varp())) પાછા;
        File_લીટી* અચલ fl = nodep->fileline();
        AstMemberSel* અચલ selp
            = નવું AstMemberSel{fl, નવું AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    CovergroupEventBindVisitor(Ast_Sen_વૃક્ષ* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* અચલ, AstVar* અચલ varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = મૂળભૂત;
};

વર્ગ CovergroupInjectVisitor final : જાહેર VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    ActiveNamer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    શૂન્ય visit(AstScope* nodep) override {
        m_namer.મુખ્ય(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* અચલ varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        અચલ Ast_ગાંઠ_DType* અચલ dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        અચલ AstClassRefDType* અચલ classRefp = VN_CAST(dtypep, ClassRefDType);
        જો (!classRefp) પાછા;

        AstClass* અચલ classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_વૃક્ષ* અચલ eventp = VN_CAST(classp->user2p(), Sen_વૃક્ષ);
        જો (!eventp) પાછા;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* અચલ sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_લીટી* અચલ fl = nodep->fileline();
        AstVarRef* અચલ varrefp = નવું AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* અચલ cmethodCallp
            = નવું AstCMethodCall{fl, varrefp, sampleCFuncp, nullptr};

        cmethodCallp->dtypeSetVoid();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_વૃક્ષ* senTreep = eventp->clone_વૃક્ષ(ખોટું);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        AstActive* અચલ activep = m_namer.getActive(fl, senTreep);
        VL_DO_DANGLING(ધકેલો_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->addStmtsp(
            નવું AstAlways{fl, VAlwaysKwd::ALWAYS_FF, nullptr, cmethodCallp->makeStmt()});
    }

    શૂન્ય visit(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        જો (Ast_Sen_વૃક્ષ* અચલ eventp = VN_CAST(nodep->user2p(), Sen_વૃક્ષ)) {
            VL_DO_DANGLING(ધકેલો_Deletep(eventp), eventp);
        }
    }

    શૂન્ય visit(AstActive*) override {}  // Don't iterate into actives
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = મૂળભૂત;
};

//######################################################################
// Active class functions

શૂન્ય V3Active::activeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ActiveVisitor{nodep}; }  // Destruct before checking
    જો (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        અચલ VNUser1InUse user1InUse;
        અચલ VNUser2InUse user2InUse;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_વૃક્ષ("active", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
