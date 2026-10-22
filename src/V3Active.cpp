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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Active.h"

#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Graph.h"

#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

শ্রেণী LatchDetectGraphVertex final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(LatchDetectGraphVertex, V3GraphVertex)
সর্বজনীন:
    গণনা VertexType : uint8_t { VT_BLOCK, VT_BRANCH, VT_OUTPUT };

ব্যক্তিগত:
    ধ্রুবক string m_name;  // Only used for .dot file generation
    ধ্রুবক VertexType m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ধ্রুবক VL_MT_SAFE {  //   "
        নির্বাচন (m_type) {
        ক্ষেত্র VT_BLOCK: ফেরত "(||)";  // basic block node
        ক্ষেত্র VT_BRANCH: ফেরত "(&&)";  // if/else branch mode
        ক্ষেত্র VT_OUTPUT: ফেরত "(out)";  // var assignment
        পূর্বনির্ধারিত: ফেরত "??";  // unknown
        }
    }

সর্বজনীন:
    LatchDetectGraphVertex(V3Graph* graphp, ধ্রুবক string& name, VertexType type = VT_BLOCK)
        : V3GraphVertex{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name + " " + typestr(); }
    string dotColor() ধ্রুবক override { ফেরত user() ? "green" : "black"; }
    আভাসী পূর্ণসংখ্যা type() ধ্রুবক { ফেরত m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

শ্রেণী LatchDetectGraph final : সর্বজনীন V3Graph {
সুরক্ষিত:
    LatchDetectGraphVertex* m_curVertexp = nullptr;  // Current latch detection graph vertex
    std::vector<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    স্থির LatchDetectGraphVertex* castVertexp(শূন্য* vertexp) {
        ফেরত reinterpret_cast<LatchDetectGraphVertex*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    যৌক্তিক latchCheckInternal(LatchDetectGraphVertex* vertexp) {
        যৌক্তিক ফলাফল = মিথ্যা;
        নির্বাচন (vertexp->type()) {
        ক্ষেত্র LatchDetectGraphVertex::VT_OUTPUT:  // Base case
            ফলাফল = vertexp->user();
            বিরতি;
        ক্ষেত্র LatchDetectGraphVertex::VT_BLOCK:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            জন্য (V3GraphEdge& edge : vertexp->outEdges()) {
                যদি (latchCheckInternal(castVertexp(edge.শীর্ষ()))) {
                    ফলাফল = সত্য;
                    বিরতি;
                }
            }
            বিরতি;
        ক্ষেত্র LatchDetectGraphVertex::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            LatchDetectGraphVertex* ধ্রুবক ifp = castVertexp(vertexp->outEdges().frontp()->শীর্ষ());
            LatchDetectGraphVertex* ধ্রুবক elsp = castVertexp(vertexp->outEdges().backp()->শীর্ষ());
            ফলাফল = latchCheckInternal(ifp) && latchCheckInternal(elsp);
            বিরতি;
        }
        vertexp->user(ফলাফল);
        ফেরত ফলাফল;
    }

সর্বজনীন:
    LatchDetectGraph() { clear(); }
    ~LatchDetectGraph() override { clear(); }
    // ACCESSORS
    LatchDetectGraphVertex* currentp() { ফেরত m_curVertexp; }
    শূন্য currentp(LatchDetectGraphVertex* vertex) { m_curVertexp = vertex; }
    // METHODS
    শূন্য begin() {
        // Start a new if/else tracking graph
        m_curVertexp = নতুন LatchDetectGraphVertex{এই, "ROOT"};
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
    LatchDetectGraphVertex* addPathVertex(LatchDetectGraphVertex* parent, ধ্রুবক string& name,
                                          যৌক্তিক branch = মিথ্যা) {
        m_curVertexp = নতুন LatchDetectGraphVertex{এই, name,
                                                  branch ? LatchDetectGraphVertex::VT_BRANCH
                                                         : LatchDetectGraphVertex::VT_BLOCK};
        নতুন V3GraphEdge{এই, parent, m_curVertexp, 1};
        ফেরত m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    LatchDetectGraphVertex* addOutputVertex(AstVarRef* nodep) {
        LatchDetectGraphVertex* ধ্রুবক outVertexp
            = নতুন LatchDetectGraphVertex{এই, nodep->name(), LatchDetectGraphVertex::VT_OUTPUT};
        nodep->varp()->user1p(outVertexp);
        m_outputs.ঠেলো_back(nodep);
        ফেরত outVertexp;
    }
    // Connect an output assignment to its parent control block
    শূন্য addAssignment(AstVarRef* nodep) {
        LatchDetectGraphVertex* outVertexp;
        যদি (!nodep->varp()->user1p()) {  // Not seen this output before
            outVertexp = addOutputVertex(nodep);
        } অন্যথা {
            outVertexp = castVertexp(nodep->varp()->user1p());
        }
        নতুন V3GraphEdge{এই, m_curVertexp, outVertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    শূন্য latchCheck(Ast_গ্রন্থি* nodep, যৌক্তিক latch_expected) {
        যৌক্তিক latch_detected = মিথ্যা;
        জন্য (ধ্রুবক AstVarRef* ধ্রুবক vrp : m_outputs) {
            LatchDetectGraphVertex* ধ্রুবক vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(সত্য);  // Identify the output vertex we are checking paths _to_
            যদি (!latchCheckInternal(castVertexp(vertices().frontp()))) latch_detected = সত্য;
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
            vertp->user(মিথ্যা);  // Clear again (see above)
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

শ্রেণী ActiveNamer final : সর্বজনীন VNVisitor {
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
    শূন্য visit(Ast_গ্রন্থি_Stmt*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    template <typename T_SenItemKind>
    AstActive*& getSpecialActive();

সর্বজনীন:
    // METHODS
    AstScope* scopep() { ফেরত m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    AstActive* makeActive(File_সারি* ধ্রুবক fl, Ast_Sen_বৃক্ষ* ধ্রুবক senTreep) {
        AstActive* ধ্রুবক activep = নতুন AstActive{fl, "", senTreep};
        activep->sen_বৃক্ষ_Storep(activep->sentreep());
        addActive(activep);
        ফেরত activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    template <typename T_SenItemKind>
    AstActive* makeSpecialActive(File_সারি* ধ্রুবক fl) {
        Ast_Sen_বৃক্ষ* ধ্রুবক senTreep = নতুন Ast_Sen_বৃক্ষ{fl, নতুন AstSenItem{fl, T_SenItemKind{}}};
        ফেরত makeActive(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    template <typename T_SenItemKind>
    AstActive* getSpecialActive(File_সারি* fl) {
        AstActive*& cachep = getSpecialActive<T_SenItemKind>();
        যদি (!cachep) cachep = makeSpecialActive<T_SenItemKind>(fl);
        ফেরত cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    AstActive* getActive(File_সারি* fl, Ast_Sen_বৃক্ষ* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        auto it = m_activeMap.find(*sentreep);
        // If found matching AstActive, return it
        যদি (it != m_activeMap.end()) ফেরত it->second;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_বৃক্ষ* ধ্রুবক newsenp = sentreep->clone_বৃক্ষ(মিথ্যা);
        AstActive* ধ্রুবক activep = নতুন AstActive{fl, "sequent", newsenp};
        activep->sen_বৃক্ষ_Storep(activep->sentreep());
        addActive(activep);
        m_activeMap.emplace(*newsenp, activep);
        ফেরত activep;
    }

    // CONSTRUCTORS
    ActiveNamer() = পূর্বনির্ধারিত;
    ~ActiveNamer() override = পূর্বনির্ধারিত;
    শূন্য প্রধান(AstScope* nodep) { iterate(nodep); }
};

template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Static>() {
    ফেরত m_sActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Initial>() {
    ফেরত m_iActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Final>() {
    ফেরত m_fActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Combo>() {
    ফেরত m_cActivep;
}

//######################################################################
// Latch checking visitor

শ্রেণী ActiveLatchCheckVisitor final : সর্বজনীন VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ধ্রুবক VNUser1InUse m_inuser1;
    // STATE
    LatchDetectGraph m_graph;  // Graph used to detect latches in combo always
    // VISITORS
    শূন্য visit(AstVarRef* nodep) override {
        ধ্রুবক AstVar* ধ্রুবক varp = nodep->varp();
        যদি (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->isUsedLoopIdx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_graph.addAssignment(nodep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_If* nodep) override {
        যদি (!nodep->isBoundsCheck()) {
            LatchDetectGraphVertex* ধ্রুবক parentp = m_graph.currentp();
            LatchDetectGraphVertex* ধ্রুবক branchp = m_graph.addPathVertex(parentp, "BRANCH", সত্য);
            m_graph.addPathVertex(branchp, "IF");
            iterate_And_পরবর্তী_Const_Null(nodep->thensp());
            m_graph.addPathVertex(branchp, "ELSE");
            iterate_And_পরবর্তী_Const_Null(nodep->elsesp());
            m_graph.currentp(parentp);
        } অন্যথা {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    ActiveLatchCheckVisitor(Ast_গ্রন্থি* nodep, যৌক্তিক expectLatch) {
        m_graph.begin();
        iterateConst(nodep);
        m_graph.latchCheck(nodep, expectLatch);
    }
    ~ActiveLatchCheckVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

শ্রেণী ActiveDlyVisitor final : সর্বজনীন VNVisitor {
সর্বজনীন:
    গণনা CheckType : uint8_t { CT_COMB, CT_FINAL };

ব্যক্তিগত:
    // MEMBERS
    ধ্রুবক CheckType m_check;  // Process type we are checking

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
        nodep->replaceWith(নতুন AstAssign{
            nodep->fileline(),  //
            nodep->lhsp()->unlinkFrBack(),  //
            nodep->rhsp()->unlinkFrBack(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlinkFrBack() : nullptr});
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }

    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    ActiveDlyVisitor(Ast_গ্রন্থি* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ActiveDlyVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Active class functions

শ্রেণী ActiveVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ActiveNamer m_namer;  // Tracking of active names
    যৌক্তিক m_clockedProcess = মিথ্যা;  // Whether current process is a clocked process
    যৌক্তিক m_allChanged = মিথ্যা;  // Whether all SenItem in the SenTree are ET_CHANGED
    যৌক্তিক m_walkingBody = মিথ্যা;  // Walking body of a process
    যৌক্তিক m_canBeComb = মিথ্যা;  // Whether current clocked process can be turned into a comb process

    // METHODS
    template <typename T>
    শূন্য moveUnderSpecial(Ast_গ্রন্থি* nodep) {
        AstActive* ধ্রুবক wantactivep = m_namer.getSpecialActive<T>(nodep->fileline());
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);
    }

    শূন্য visitAlways(Ast_গ্রন্থি* nodep, Ast_Sen_বৃক্ষ* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        যদি (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ফেরত;
        }

        {
            ধ্রুবক VNUser1InUse user1InUse;

            // Walk sensitivity list
            m_clockedProcess = মিথ্যা;
            m_allChanged = সত্য;
            যদি (oldsentreep) {
                oldsentreep->unlinkFrBack();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            যদি (m_allChanged) {
                ধ্রুবক VNUser2InUse user2InUse;
                m_walkingBody = সত্য;
                m_canBeComb = সত্য;
                iterateChildrenConst(nodep);
                m_walkingBody = মিথ্যা;
                যদি (m_canBeComb) m_clockedProcess = মিথ্যা;
            }
        }

        AstActive* ধ্রুবক wantactivep
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
            ধ্রুবক ActiveLatchCheckVisitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    শূন্য visit_Sen_বস্তুসমূহ(Ast_গ্রন্থি* nodep) {
        যদি (v3Global.opt.timing().isSetTrue()) {
            nodep->foreach([এই](AstSenItem* senItemp) { visit(senItemp); });
        }
    }

    স্থির শূন্য markEventEdges(Ast_Sen_বৃক্ষ* sentreep) {
        জন্য (AstSenItem* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), SenItem)) {
            যদি (!senip->sensp()) চালু;
            যদি (ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = senip->sensp()->dtypep()) {
                যদি (ধ্রুবক AstBasicDType* ধ্রুবক basicp = dtypep->basicp()) {
                    যদি (basicp->isEvent()) senip->edgeType(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    শূন্য visit(AstScope* nodep) override {
        m_namer.প্রধান(nodep);  // Clear last scope's names, and collect this scope's existing names
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
        ধ্রুবক ActiveDlyVisitor dlyvisitor{nodep, ActiveDlyVisitor::CT_FINAL};
        moveUnderSpecial<AstSenItem::Final>(nodep);
    }
    শূন্য visit(AstCoverToggle* nodep) override { moveUnderSpecial<AstSenItem::Combo>(nodep); }
    শূন্য visit(AstAlways* nodep) override {
        যদি (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ফেরত;
        }
        যদি (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            moveUnderSpecial<AstSenItem::Combo>(nodep);
            ফেরত;
        }
        visit_Sen_বস্তুসমূহ(nodep);
        visitAlways(nodep, nodep->sentreep(), nodep->keyword());
    }
    শূন্য visit(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ধ্রুবক activep = m_namer.makeSpecialActive<AstSenItem::Combo>(nodep->fileline());
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    শূন্য visit(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_বৃক্ষ* ধ্রুবক sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ধ্রুবক activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    শূন্য visit(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_বৃক্ষ* ধ্রুবক sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ধ্রুবক activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    শূন্য visit(AstCFunc* nodep) override { visit_Sen_বস্তুসমূহ(nodep); }
    শূন্য visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        যদি (!nodep->sensp()) ফেরত;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = সত্য;
        যদি (nodep->edgeType() != VEdgeType::ET_CHANGED) m_allChanged = মিথ্যা;

        যদি (ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = nodep->sensp()->dtypep()) {
            যদি (ধ্রুবক AstBasicDType* ধ্রুবক basicp = dtypep->basicp()) {
                যদি (basicp->isEvent()) nodep->edgeType(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ধ্রুবক AstVarRef* refp) { refp->varScopep()->user1(সত্য); });
    }

    শূন্য visit(AstVarRef* nodep) override {
        AstVarScope* ধ্রুবক vscp = nodep->varScopep();
        যদি (nodep->access().isWriteOnly()) {
            vscp->user2(সত্য);
        } অন্যথা {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            ধ্রুবক AstVar* ধ্রুবক varp = vscp->varp();
            যদি (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = মিথ্যা;
            }
        }
    }
    শূন্য visit(AstAssignDly* nodep) override {
        m_canBeComb = মিথ্যা;
        যদি (nodep->isTimingControl()) m_clockedProcess = সত্য;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstFireEvent* nodep) override {
        m_canBeComb = মিথ্যা;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstAssignForce* nodep) override {
        m_canBeComb = মিথ্যা;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstRelease* nodep) override {
        m_canBeComb = মিথ্যা;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstFork* nodep) override {
        যদি (nodep->isTimingControl()) {
            m_canBeComb = মিথ্যা;
            m_clockedProcess = সত্য;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    শূন্য visit(AstVar*) override {}  // Accelerate
    শূন্য visit(AstVarScope*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (!v3Global.opt.timing().isSetTrue() && m_walkingBody && !m_canBeComb) {
            ফেরত;  // Accelerate
        }
        যদি (!nodep->isPure()) m_canBeComb = মিথ্যা;
        যদি (nodep->isTimingControl()) {
            m_canBeComb = মিথ্যা;
            m_clockedProcess = সত্য;
            ফেরত;
        }
        iterateChildren(nodep);
    }

সর্বজনীন:
    // CONSTRUCTORS
    explicit ActiveVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ActiveVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

শ্রেণী CovergroupCollectVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = nullptr;  // Current covergroup class context, or nullptr

    // VISITORS
    শূন্য visit(AstClass* nodep) override {
        যদি (!nodep->isCovergroup()) ফেরত;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    শূন্য visit(AstScope* nodep) override { iterateChildren(nodep); }

    শূন্য visit(AstCFunc* nodep) override {
        যদি (!m_classp) ফেরত;
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

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

শ্রেণী CovergroupEventBindVisitor final : সর্বজনীন VNVisitor {
    AstVarScope* ধ্রুবক m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_set<ধ্রুবক AstVar*> m_memberps;  // Non-static variables in the covergroup class

    শূন্য visit(AstVarRef* nodep) override {
        যদি (!m_memberps.গুনতি(nodep->varp())) ফেরত;
        File_সারি* ধ্রুবক fl = nodep->fileline();
        AstMemberSel* ধ্রুবক selp
            = নতুন AstMemberSel{fl, নতুন AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    CovergroupEventBindVisitor(Ast_Sen_বৃক্ষ* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* ধ্রুবক, AstVar* ধ্রুবক varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = পূর্বনির্ধারিত;
};

শ্রেণী CovergroupInjectVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    ActiveNamer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    শূন্য visit(AstScope* nodep) override {
        m_namer.প্রধান(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    শূন্য visit(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ধ্রুবক varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ধ্রুবক AstClassRefDType* ধ্রুবক classRefp = VN_CAST(dtypep, ClassRefDType);
        যদি (!classRefp) ফেরত;

        AstClass* ধ্রুবক classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_বৃক্ষ* ধ্রুবক eventp = VN_CAST(classp->user2p(), Sen_বৃক্ষ);
        যদি (!eventp) ফেরত;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ধ্রুবক sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_সারি* ধ্রুবক fl = nodep->fileline();
        AstVarRef* ধ্রুবক varrefp = নতুন AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* ধ্রুবক cmethodCallp
            = নতুন AstCMethodCall{fl, varrefp, sampleCFuncp, nullptr};

        cmethodCallp->dtypeSetVoid();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_বৃক্ষ* senTreep = eventp->clone_বৃক্ষ(মিথ্যা);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        AstActive* ধ্রুবক activep = m_namer.getActive(fl, senTreep);
        VL_DO_DANGLING(ঠেলো_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->addStmtsp(
            নতুন AstAlways{fl, VAlwaysKwd::ALWAYS_FF, nullptr, cmethodCallp->makeStmt()});
    }

    শূন্য visit(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        যদি (Ast_Sen_বৃক্ষ* ধ্রুবক eventp = VN_CAST(nodep->user2p(), Sen_বৃক্ষ)) {
            VL_DO_DANGLING(ঠেলো_Deletep(eventp), eventp);
        }
    }

    শূন্য visit(AstActive*) override {}  // Don't iterate into actives
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Active class functions

শূন্য V3Active::activeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ActiveVisitor{nodep}; }  // Destruct before checking
    যদি (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ধ্রুবক VNUser1InUse user1InUse;
        ধ্রুবক VNUser2InUse user2InUse;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_বৃক্ষ("active", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
