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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Active.h"

#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3Graph.h"

#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

শ্ৰেণী LatchDetectGraphVertex final : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(LatchDetectGraphVertex, V3GraphVertex)
সার্ৱজনিক:
    গণনা VertexType : uint8_t { VT_BLOCK, VT_BRANCH, VT_OUTPUT };

গুপ্ত:
    ধ্রুৱ string m_name;  // Only used for .dot file generation
    ধ্রুৱ VertexType m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ধ্রুৱ VL_MT_SAFE {  //   "
        ৱিকল্প (m_type) {
        পক্ষ VT_BLOCK: ঘূৰাই "(||)";  // basic block node
        পক্ষ VT_BRANCH: ঘূৰাই "(&&)";  // if/else branch mode
        পক্ষ VT_OUTPUT: ঘূৰাই "(out)";  // var assignment
        স্ৱভাৱ: ঘূৰাই "??";  // unknown
        }
    }

সার্ৱজনিক:
    LatchDetectGraphVertex(V3Graph* graphp, ধ্রুৱ string& name, VertexType type = VT_BLOCK)
        : V3GraphVertex{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_name + " " + typestr(); }
    string dotColor() ধ্রুৱ override { ঘূৰাই user() ? "green" : "black"; }
    আভাস পূর্ণাঙ্ক type() ধ্রুৱ { ঘূৰাই m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

শ্ৰেণী LatchDetectGraph final : সার্ৱজনিক V3Graph {
রক্ষিত:
    LatchDetectGraphVertex* m_curVertexp = nullptr;  // Current latch detection graph vertex
    std::vector<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    স্থির LatchDetectGraphVertex* castVertexp(শূন্য* vertexp) {
        ঘূৰাই reinterpret_cast<LatchDetectGraphVertex*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    তার্কিক latchCheckInternal(LatchDetectGraphVertex* vertexp) {
        তার্কিক result = মিছা;
        ৱিকল্প (vertexp->type()) {
        পক্ষ LatchDetectGraphVertex::VT_OUTPUT:  // Base case
            result = vertexp->user();
            ভঙ্গ;
        পক্ষ LatchDetectGraphVertex::VT_BLOCK:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            বাবে (V3GraphEdge& edge : vertexp->outEdges()) {
                যদি (latchCheckInternal(castVertexp(edge.শীৰ্ষ()))) {
                    result = সঁচা;
                    ভঙ্গ;
                }
            }
            ভঙ্গ;
        পক্ষ LatchDetectGraphVertex::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            LatchDetectGraphVertex* ধ্রুৱ ifp = castVertexp(vertexp->outEdges().frontp()->শীৰ্ষ());
            LatchDetectGraphVertex* ধ্রুৱ elsp = castVertexp(vertexp->outEdges().backp()->শীৰ্ষ());
            result = latchCheckInternal(ifp) && latchCheckInternal(elsp);
            ভঙ্গ;
        }
        vertexp->user(result);
        ঘূৰাই result;
    }

সার্ৱজনিক:
    LatchDetectGraph() { clear(); }
    ~LatchDetectGraph() override { clear(); }
    // ACCESSORS
    LatchDetectGraphVertex* currentp() { ঘূৰাই m_curVertexp; }
    শূন্য currentp(LatchDetectGraphVertex* vertex) { m_curVertexp = vertex; }
    // METHODS
    শূন্য begin() {
        // Start a new if/else tracking graph
        m_curVertexp = নূতন LatchDetectGraphVertex{এতত্, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    শূন্য clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3Graph::clear();
        m_curVertexp = nullptr;
    }
    // Add a new control path and connect it to its parent
    LatchDetectGraphVertex* addPathVertex(LatchDetectGraphVertex* parent, ধ্রুৱ string& name,
                                          তার্কিক branch = মিছা) {
        m_curVertexp = নূতন LatchDetectGraphVertex{এতত্, name,
                                                  branch ? LatchDetectGraphVertex::VT_BRANCH
                                                         : LatchDetectGraphVertex::VT_BLOCK};
        নূতন V3GraphEdge{এতত্, parent, m_curVertexp, 1};
        ঘূৰাই m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    LatchDetectGraphVertex* addOutputVertex(AstVarRef* nodep) {
        LatchDetectGraphVertex* ধ্রুৱ outVertexp
            = নূতন LatchDetectGraphVertex{এতত্, nodep->name(), LatchDetectGraphVertex::VT_OUTPUT};
        nodep->varp()->user1p(outVertexp);
        m_outputs.ঠেলো_back(nodep);
        ঘূৰাই outVertexp;
    }
    // Connect an output assignment to its parent control block
    শূন্য addAssignment(AstVarRef* nodep) {
        LatchDetectGraphVertex* outVertexp;
        যদি (!nodep->varp()->user1p()) {  // Not seen this output before
            outVertexp = addOutputVertex(nodep);
        } অন্যথা {
            outVertexp = castVertexp(nodep->varp()->user1p());
        }
        নূতন V3GraphEdge{এতত্, m_curVertexp, outVertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    শূন্য latchCheck(Ast_গ্ৰন্থি* nodep, তার্কিক latch_expected) {
        তার্কিক latch_detected = মিছা;
        বাবে (ধ্রুৱ AstVarRef* ধ্রুৱ vrp : m_outputs) {
            LatchDetectGraphVertex* ধ্রুৱ vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(সঁচা);  // Identify the output vertex we are checking paths _to_
            যদি (!latchCheckInternal(castVertexp(vertices().frontp()))) latch_detected = সঁচা;
            যদি (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warnMore()
                        << "... Suggest use of always_latch for intentional latches");
                যদি (dumpGraphLevel() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(মিছা);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        যদি (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

শ্ৰেণী ActiveNamer final : সার্ৱজনিক VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope to add statement to
    AstActive* m_sActivep = nullptr;  // For current scope, the Static active we're building
    AstActive* m_iActivep = nullptr;  // For current scope, the Initial active we're building
    AstActive* m_fActivep = nullptr;  // For current scope, the Final active we're building
    AstActive* m_cActivep = nullptr;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_বৃক্ষ>, AstActive*> m_activeMap;

    // METHODS
    শূন্য addActive(AstActive* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->addBlocksp(nodep);
    }

    // VISITORS
    শূন্য visit(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = nullptr;
        m_iActivep = nullptr;
        m_fActivep = nullptr;
        m_cActivep = nullptr;
        m_activeMap.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    শূন্য visit(Ast_Sen_বৃক্ষ* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    শূন্য visit(Ast_গ্ৰন্থি_Stmt*) override {}  // Accelerate
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    template <typename T_SenItemKind>
    AstActive*& getSpecialActive();

সার্ৱজনিক:
    // METHODS
    AstScope* scopep() { ঘূৰাই m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    AstActive* makeActive(File_সাৰি* ধ্রুৱ fl, Ast_Sen_বৃক্ষ* ধ্রুৱ senTreep) {
        AstActive* ধ্রুৱ activep = নূতন AstActive{fl, "", senTreep};
        activep->sen_বৃক্ষ_Storep(activep->sentreep());
        addActive(activep);
        ঘূৰাই activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    template <typename T_SenItemKind>
    AstActive* makeSpecialActive(File_সাৰি* ধ্রুৱ fl) {
        Ast_Sen_বৃক্ষ* ধ্রুৱ senTreep = নূতন Ast_Sen_বৃক্ষ{fl, নূতন AstSenItem{fl, T_SenItemKind{}}};
        ঘূৰাই makeActive(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    template <typename T_SenItemKind>
    AstActive* getSpecialActive(File_সাৰি* fl) {
        AstActive*& cachep = getSpecialActive<T_SenItemKind>();
        যদি (!cachep) cachep = makeSpecialActive<T_SenItemKind>(fl);
        ঘূৰাই cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    AstActive* getActive(File_সাৰি* fl, Ast_Sen_বৃক্ষ* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        auto it = m_activeMap.find(*sentreep);
        // If found matching AstActive, return it
        যদি (it != m_activeMap.end()) ঘূৰাই it->second;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_বৃক্ষ* ধ্রুৱ newsenp = sentreep->clone_বৃক্ষ(মিছা);
        AstActive* ধ্রুৱ activep = নূতন AstActive{fl, "sequent", newsenp};
        activep->sen_বৃক্ষ_Storep(activep->sentreep());
        addActive(activep);
        m_activeMap.emplace(*newsenp, activep);
        ঘূৰাই activep;
    }

    // CONSTRUCTORS
    ActiveNamer() = স্ৱভাৱ;
    ~ActiveNamer() override = স্ৱভাৱ;
    শূন্য main(AstScope* nodep) { iterate(nodep); }
};

template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Static>() {
    ঘূৰাই m_sActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Initial>() {
    ঘূৰাই m_iActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Final>() {
    ঘূৰাই m_fActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Combo>() {
    ঘূৰাই m_cActivep;
}

//######################################################################
// Latch checking visitor

শ্ৰেণী ActiveLatchCheckVisitor final : সার্ৱজনিক VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ধ্রুৱ VNUser1InUse m_inuser1;
    // STATE
    LatchDetectGraph m_graph;  // Graph used to detect latches in combo always
    // VISITORS
    শূন্য visit(AstVarRef* nodep) override {
        ধ্রুৱ AstVar* ধ্রুৱ varp = nodep->varp();
        যদি (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->isUsedLoopIdx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_graph.addAssignment(nodep);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_If* nodep) override {
        যদি (!nodep->isBoundsCheck()) {
            LatchDetectGraphVertex* ধ্রুৱ parentp = m_graph.currentp();
            LatchDetectGraphVertex* ধ্রুৱ branchp = m_graph.addPathVertex(parentp, "BRANCH", সঁচা);
            m_graph.addPathVertex(branchp, "IF");
            iterateAndNextConstNull(nodep->thensp());
            m_graph.addPathVertex(branchp, "ELSE");
            iterateAndNextConstNull(nodep->elsesp());
            m_graph.currentp(parentp);
        } অন্যথা {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    ActiveLatchCheckVisitor(Ast_গ্ৰন্থি* nodep, তার্কিক expectLatch) {
        m_graph.begin();
        iterateConst(nodep);
        m_graph.latchCheck(nodep, expectLatch);
    }
    ~ActiveLatchCheckVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

শ্ৰেণী ActiveDlyVisitor final : সার্ৱজনিক VNVisitor {
সার্ৱজনিক:
    গণনা CheckType : uint8_t { CT_COMB, CT_FINAL };

গুপ্ত:
    // MEMBERS
    ধ্রুৱ CheckType m_check;  // Process type we are checking

    // VISITORS
    শূন্য visit(AstAssignDly* nodep) override {
        // Issue appropriate warning
        যদি (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } অন্যথা {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warnMore()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(নূতন AstAssign{
            nodep->fileline(),  //
            nodep->lhsp()->unlinkFrBack(),  //
            nodep->rhsp()->unlinkFrBack(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlinkFrBack() : nullptr});
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }

    //--------------------
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    ActiveDlyVisitor(Ast_গ্ৰন্থি* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ActiveDlyVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Active class functions

শ্ৰেণী ActiveVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ActiveNamer m_namer;  // Tracking of active names
    তার্কিক m_clockedProcess = মিছা;  // Whether current process is a clocked process
    তার্কিক m_allChanged = মিছা;  // Whether all SenItem in the SenTree are ET_CHANGED
    তার্কিক m_walkingBody = মিছা;  // Walking body of a process
    তার্কিক m_canBeComb = মিছা;  // Whether current clocked process can be turned into a comb process

    // METHODS
    template <typename T>
    শূন্য moveUnderSpecial(Ast_গ্ৰন্থি* nodep) {
        AstActive* ধ্রুৱ wantactivep = m_namer.getSpecialActive<T>(nodep->fileline());
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);
    }

    শূন্য visitAlways(Ast_গ্ৰন্থি* nodep, Ast_Sen_বৃক্ষ* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        যদি (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        }

        {
            ধ্রুৱ VNUser1InUse user1InUse;

            // Walk sensitivity list
            m_clockedProcess = মিছা;
            m_allChanged = সঁচা;
            যদি (oldsentreep) {
                oldsentreep->unlinkFrBack();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            যদি (m_allChanged) {
                ধ্রুৱ VNUser2InUse user2InUse;
                m_walkingBody = সঁচা;
                m_canBeComb = সঁচা;
                iterateChildrenConst(nodep);
                m_walkingBody = মিছা;
                যদি (m_canBeComb) m_clockedProcess = মিছা;
            }
        }

        AstActive* ধ্রুৱ wantactivep
            = !m_clockedProcess ? m_namer.getSpecialActive<AstSenItem::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.getActive(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.getSpecialActive<AstSenItem::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);

        // Warn and convert any delayed assignments
        যদি (!m_clockedProcess) ActiveDlyVisitor{nodep, ActiveDlyVisitor::CT_COMB};

        // Delete sensitivity list
        যদি (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_বৃক্ষ(), oldsentreep);

        // check combinational processes for latches
        যদি (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ধ্রুৱ ActiveLatchCheckVisitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    শূন্য visit_Sen_বস্তুসমূহ(Ast_গ্ৰন্থি* nodep) {
        যদি (v3Global.opt.timing().isSetTrue()) {
            nodep->foreach([এতত্](AstSenItem* senItemp) { visit(senItemp); });
        }
    }

    স্থির শূন্য markEventEdges(Ast_Sen_বৃক্ষ* sentreep) {
        বাবে (AstSenItem* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), SenItem)) {
            যদি (!senip->sensp()) অনুৱর্ত;
            যদি (ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = senip->sensp()->dtypep()) {
                যদি (ধ্রুৱ AstBasicDType* ধ্রুৱ basicp = dtypep->basicp()) {
                    যদি (basicp->isEvent()) senip->edgeType(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    শূন্য visit(AstScope* nodep) override {
        m_namer.main(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    শূন্য visit(AstActive* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    শূন্য visit(AstInitialStatic* nodep) override { moveUnderSpecial<AstSenItem::Static>(nodep); }
    শূন্য visit(AstInitial* nodep) override {
        visit_Sen_বস্তুসমূহ(nodep);
        moveUnderSpecial<AstSenItem::Initial>(nodep);
    }
    শূন্য visit(AstFinal* nodep) override {
        ধ্রুৱ ActiveDlyVisitor dlyvisitor{nodep, ActiveDlyVisitor::CT_FINAL};
        moveUnderSpecial<AstSenItem::Final>(nodep);
    }
    শূন্য visit(AstCoverToggle* nodep) override { moveUnderSpecial<AstSenItem::Combo>(nodep); }
    শূন্য visit(AstAlways* nodep) override {
        যদি (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        }
        যদি (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            moveUnderSpecial<AstSenItem::Combo>(nodep);
            ঘূৰাই;
        }
        visit_Sen_বস্তুসমূহ(nodep);
        visitAlways(nodep, nodep->sentreep(), nodep->keyword());
    }
    শূন্য visit(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ধ্রুৱ activep = m_namer.makeSpecialActive<AstSenItem::Combo>(nodep->fileline());
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    শূন্য visit(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_বৃক্ষ* ধ্রুৱ sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ধ্রুৱ activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    শূন্য visit(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_বৃক্ষ* ধ্রুৱ sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ধ্রুৱ activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    শূন্য visit(AstCFunc* nodep) override { visit_Sen_বস্তুসমূহ(nodep); }
    শূন্য visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        যদি (!nodep->sensp()) ঘূৰাই;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = সঁচা;
        যদি (nodep->edgeType() != VEdgeType::ET_CHANGED) m_allChanged = মিছা;

        যদি (ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = nodep->sensp()->dtypep()) {
            যদি (ধ্রুৱ AstBasicDType* ধ্রুৱ basicp = dtypep->basicp()) {
                যদি (basicp->isEvent()) nodep->edgeType(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ধ্রুৱ AstVarRef* refp) { refp->varScopep()->user1(সঁচা); });
    }

    শূন্য visit(AstVarRef* nodep) override {
        AstVarScope* ধ্রুৱ vscp = nodep->varScopep();
        যদি (nodep->access().isWriteOnly()) {
            vscp->user2(সঁচা);
        } অন্যথা {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            ধ্রুৱ AstVar* ধ্রুৱ varp = vscp->varp();
            যদি (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = মিছা;
            }
        }
    }
    শূন্য visit(AstAssignDly* nodep) override {
        m_canBeComb = মিছা;
        যদি (nodep->isTimingControl()) m_clockedProcess = সঁচা;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstFireEvent* nodep) override {
        m_canBeComb = মিছা;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstAssignForce* nodep) override {
        m_canBeComb = মিছা;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstRelease* nodep) override {
        m_canBeComb = মিছা;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstFork* nodep) override {
        যদি (nodep->isTimingControl()) {
            m_canBeComb = মিছা;
            m_clockedProcess = সঁচা;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    শূন্য visit(AstVar*) override {}  // Accelerate
    শূন্য visit(AstVarScope*) override {}  // Accelerate
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        যদি (!v3Global.opt.timing().isSetTrue() && m_walkingBody && !m_canBeComb) {
            ঘূৰাই;  // Accelerate
        }
        যদি (!nodep->isPure()) m_canBeComb = মিছা;
        যদি (nodep->isTimingControl()) {
            m_canBeComb = মিছা;
            m_clockedProcess = সঁচা;
            ঘূৰাই;
        }
        iterateChildren(nodep);
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit ActiveVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ActiveVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

শ্ৰেণী CovergroupCollectVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = nullptr;  // Current covergroup class context, or nullptr

    // VISITORS
    শূন্য visit(AstClass* nodep) override {
        যদি (!nodep->isCovergroup()) ঘূৰাই;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    শূন্য visit(AstScope* nodep) override { iterateChildren(nodep); }

    শূন্য visit(AstCFunc* nodep) override {
        যদি (!m_classp) ঘূৰাই;
        যদি (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    শূন্য visit(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlinkFrBack());
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

শ্ৰেণী CovergroupEventBindVisitor final : সার্ৱজনিক VNVisitor {
    AstVarScope* ধ্রুৱ m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_set<ধ্রুৱ AstVar*> m_memberps;  // Non-static variables in the covergroup class

    শূন্য visit(AstVarRef* nodep) override {
        যদি (!m_memberps.গুনতি(nodep->varp())) ঘূৰাই;
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        AstMemberSel* ধ্রুৱ selp
            = নূতন AstMemberSel{fl, নূতন AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    CovergroupEventBindVisitor(Ast_Sen_বৃক্ষ* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* ধ্রুৱ, AstVar* ধ্রুৱ varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = স্ৱভাৱ;
};

শ্ৰেণী CovergroupInjectVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    ActiveNamer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    শূন্য visit(AstScope* nodep) override {
        m_namer.main(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    শূন্য visit(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ধ্রুৱ varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ধ্রুৱ AstClassRefDType* ধ্রুৱ classRefp = VN_CAST(dtypep, ClassRefDType);
        যদি (!classRefp) ঘূৰাই;

        AstClass* ধ্রুৱ classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_বৃক্ষ* ধ্রুৱ eventp = VN_CAST(classp->user2p(), Sen_বৃক্ষ);
        যদি (!eventp) ঘূৰাই;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ধ্রুৱ sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        AstVarRef* ধ্রুৱ varrefp = নূতন AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* ধ্রুৱ cmethodCallp
            = নূতন AstCMethodCall{fl, varrefp, sampleCFuncp, nullptr};

        cmethodCallp->dtypeSetVoid();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_বৃক্ষ* senTreep = eventp->clone_বৃক্ষ(মিছা);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        AstActive* ধ্রুৱ activep = m_namer.getActive(fl, senTreep);
        VL_DO_DANGLING(ঠেলো_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->addStmtsp(
            নূতন AstAlways{fl, VAlwaysKwd::ALWAYS_FF, nullptr, cmethodCallp->makeStmt()});
    }

    শূন্য visit(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        যদি (Ast_Sen_বৃক্ষ* ধ্রুৱ eventp = VN_CAST(nodep->user2p(), Sen_বৃক্ষ)) {
            VL_DO_DANGLING(ঠেলো_Deletep(eventp), eventp);
        }
    }

    শূন্য visit(AstActive*) override {}  // Don't iterate into actives
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Active class functions

শূন্য V3Active::activeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ActiveVisitor{nodep}; }  // Destruct before checking
    যদি (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ধ্রুৱ VNUser1InUse user1InUse;
        ধ্রুৱ VNUser2InUse user2InUse;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_বৃক্ষ("active", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
