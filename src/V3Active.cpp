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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Active.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Graph.h"

#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

वग्ग LatchDetectGraphVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(LatchDetectGraphVertex, V3GraphVertex)
सार्वजनिक:
    गणना VertexType : uint8_t { VT_BLOCK, VT_BRANCH, VT_OUTPUT };

गुप्त:
    ध्रुव string m_name;  // Only used for .dot file generation
    ध्रुव VertexType m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ध्रुव VL_MT_SAFE {  //   "
        विकल्प (m_type) {
        पक्ष VT_BLOCK: पटिनिवत्त "(||)";  // basic block node
        पक्ष VT_BRANCH: पटिनिवत्त "(&&)";  // if/else branch mode
        पक्ष VT_OUTPUT: पटिनिवत्त "(out)";  // var assignment
        स्वभाव: पटिनिवत्त "??";  // unknown
        }
    }

सार्वजनिक:
    LatchDetectGraphVertex(V3Graph* graphp, ध्रुव string& name, VertexType type = VT_BLOCK)
        : V3GraphVertex{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name + " " + typestr(); }
    string dotColor() ध्रुव override { पटिनिवत्त user() ? "green" : "black"; }
    आभास पूर्णाङ्क type() ध्रुव { पटिनिवत्त m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

वग्ग LatchDetectGraph final : सार्वजनिक V3Graph {
रक्षित:
    LatchDetectGraphVertex* m_curVertexp = nullptr;  // Current latch detection graph vertex
    std::vector<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    स्थिर LatchDetectGraphVertex* castVertexp(शून्य* vertexp) {
        पटिनिवत्त reinterpret_cast<LatchDetectGraphVertex*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    तार्किक latchCheckInternal(LatchDetectGraphVertex* vertexp) {
        तार्किक result = मिच्छा;
        विकल्प (vertexp->type()) {
        पक्ष LatchDetectGraphVertex::VT_OUTPUT:  // Base case
            result = vertexp->user();
            भङ्ग;
        पक्ष LatchDetectGraphVertex::VT_BLOCK:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            अत्थाय (V3GraphEdge& edge : vertexp->outEdges()) {
                सचे (latchCheckInternal(castVertexp(edge.शिखरम्()))) {
                    result = सच्च;
                    भङ्ग;
                }
            }
            भङ्ग;
        पक्ष LatchDetectGraphVertex::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            LatchDetectGraphVertex* ध्रुव ifp = castVertexp(vertexp->outEdges().frontp()->शिखरम्());
            LatchDetectGraphVertex* ध्रुव elsp = castVertexp(vertexp->outEdges().backp()->शिखरम्());
            result = latchCheckInternal(ifp) && latchCheckInternal(elsp);
            भङ्ग;
        }
        vertexp->user(result);
        पटिनिवत्त result;
    }

सार्वजनिक:
    LatchDetectGraph() { clear(); }
    ~LatchDetectGraph() override { clear(); }
    // ACCESSORS
    LatchDetectGraphVertex* currentp() { पटिनिवत्त m_curVertexp; }
    शून्य currentp(LatchDetectGraphVertex* vertex) { m_curVertexp = vertex; }
    // METHODS
    शून्य begin() {
        // Start a new if/else tracking graph
        m_curVertexp = नूतन LatchDetectGraphVertex{एतत्, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    शून्य clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3Graph::clear();
        m_curVertexp = nullptr;
    }
    // Add a new control path and connect it to its parent
    LatchDetectGraphVertex* addPathVertex(LatchDetectGraphVertex* parent, ध्रुव string& name,
                                          तार्किक branch = मिच्छा) {
        m_curVertexp = नूतन LatchDetectGraphVertex{एतत्, name,
                                                  branch ? LatchDetectGraphVertex::VT_BRANCH
                                                         : LatchDetectGraphVertex::VT_BLOCK};
        नूतन V3GraphEdge{एतत्, parent, m_curVertexp, 1};
        पटिनिवत्त m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    LatchDetectGraphVertex* addOutputVertex(AstVarRef* nodep) {
        LatchDetectGraphVertex* ध्रुव outVertexp
            = नूतन LatchDetectGraphVertex{एतत्, nodep->name(), LatchDetectGraphVertex::VT_OUTPUT};
        nodep->varp()->user1p(outVertexp);
        m_outputs.प्रक्षिप_back(nodep);
        पटिनिवत्त outVertexp;
    }
    // Connect an output assignment to its parent control block
    शून्य addAssignment(AstVarRef* nodep) {
        LatchDetectGraphVertex* outVertexp;
        सचे (!nodep->varp()->user1p()) {  // Not seen this output before
            outVertexp = addOutputVertex(nodep);
        } अञ्ञथा {
            outVertexp = castVertexp(nodep->varp()->user1p());
        }
        नूतन V3GraphEdge{एतत्, m_curVertexp, outVertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    शून्य latchCheck(Ast_ग्रन्थिः* nodep, तार्किक latch_expected) {
        तार्किक latch_detected = मिच्छा;
        अत्थाय (ध्रुव AstVarRef* ध्रुव vrp : m_outputs) {
            LatchDetectGraphVertex* ध्रुव vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(सच्च);  // Identify the output vertex we are checking paths _to_
            सचे (!latchCheckInternal(castVertexp(vertices().frontp()))) latch_detected = सच्च;
            सचे (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warnMore()
                        << "... Suggest use of always_latch for intentional latches");
                सचे (dumpGraphLevel() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(मिच्छा);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        सचे (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

वग्ग ActiveNamer final : सार्वजनिक VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope to add statement to
    AstActive* m_sActivep = nullptr;  // For current scope, the Static active we're building
    AstActive* m_iActivep = nullptr;  // For current scope, the Initial active we're building
    AstActive* m_fActivep = nullptr;  // For current scope, the Final active we're building
    AstActive* m_cActivep = nullptr;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_वृक्षः>, AstActive*> m_activeMap;

    // METHODS
    शून्य addActive(AstActive* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->addBlocksp(nodep);
    }

    // VISITORS
    शून्य visit(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = nullptr;
        m_iActivep = nullptr;
        m_fActivep = nullptr;
        m_cActivep = nullptr;
        m_activeMap.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    शून्य visit(Ast_Sen_वृक्षः* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    शून्य visit(Ast_ग्रन्थिः_Stmt*) override {}  // Accelerate
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    template <typename T_SenItemKind>
    AstActive*& getSpecialActive();

सार्वजनिक:
    // METHODS
    AstScope* scopep() { पटिनिवत्त m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    AstActive* makeActive(File_पङ्क्तिः* ध्रुव fl, Ast_Sen_वृक्षः* ध्रुव senTreep) {
        AstActive* ध्रुव activep = नूतन AstActive{fl, "", senTreep};
        activep->sen_वृक्षः_Storep(activep->sentreep());
        addActive(activep);
        पटिनिवत्त activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    template <typename T_SenItemKind>
    AstActive* makeSpecialActive(File_पङ्क्तिः* ध्रुव fl) {
        Ast_Sen_वृक्षः* ध्रुव senTreep = नूतन Ast_Sen_वृक्षः{fl, नूतन AstSenItem{fl, T_SenItemKind{}}};
        पटिनिवत्त makeActive(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    template <typename T_SenItemKind>
    AstActive* getSpecialActive(File_पङ्क्तिः* fl) {
        AstActive*& cachep = getSpecialActive<T_SenItemKind>();
        सचे (!cachep) cachep = makeSpecialActive<T_SenItemKind>(fl);
        पटिनिवत्त cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    AstActive* getActive(File_पङ्क्तिः* fl, Ast_Sen_वृक्षः* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        auto it = m_activeMap.find(*sentreep);
        // If found matching AstActive, return it
        सचे (it != m_activeMap.end()) पटिनिवत्त it->second;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_वृक्षः* ध्रुव newsenp = sentreep->clone_वृक्षः(मिच्छा);
        AstActive* ध्रुव activep = नूतन AstActive{fl, "sequent", newsenp};
        activep->sen_वृक्षः_Storep(activep->sentreep());
        addActive(activep);
        m_activeMap.emplace(*newsenp, activep);
        पटिनिवत्त activep;
    }

    // CONSTRUCTORS
    ActiveNamer() = स्वभाव;
    ~ActiveNamer() override = स्वभाव;
    शून्य main(AstScope* nodep) { iterate(nodep); }
};

template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Static>() {
    पटिनिवत्त m_sActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Initial>() {
    पटिनिवत्त m_iActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Final>() {
    पटिनिवत्त m_fActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Combo>() {
    पटिनिवत्त m_cActivep;
}

//######################################################################
// Latch checking visitor

वग्ग ActiveLatchCheckVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ध्रुव VNUser1InUse m_inuser1;
    // STATE
    LatchDetectGraph m_graph;  // Graph used to detect latches in combo always
    // VISITORS
    शून्य visit(AstVarRef* nodep) override {
        ध्रुव AstVar* ध्रुव varp = nodep->varp();
        सचे (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->isUsedLoopIdx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_graph.addAssignment(nodep);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_If* nodep) override {
        सचे (!nodep->isBoundsCheck()) {
            LatchDetectGraphVertex* ध्रुव parentp = m_graph.currentp();
            LatchDetectGraphVertex* ध्रुव branchp = m_graph.addPathVertex(parentp, "BRANCH", सच्च);
            m_graph.addPathVertex(branchp, "IF");
            iterateAndNextConstNull(nodep->thensp());
            m_graph.addPathVertex(branchp, "ELSE");
            iterateAndNextConstNull(nodep->elsesp());
            m_graph.currentp(parentp);
        } अञ्ञथा {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    ActiveLatchCheckVisitor(Ast_ग्रन्थिः* nodep, तार्किक expectLatch) {
        m_graph.begin();
        iterateConst(nodep);
        m_graph.latchCheck(nodep, expectLatch);
    }
    ~ActiveLatchCheckVisitor() override = स्वभाव;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

वग्ग ActiveDlyVisitor final : सार्वजनिक VNVisitor {
सार्वजनिक:
    गणना CheckType : uint8_t { CT_COMB, CT_FINAL };

गुप्त:
    // MEMBERS
    ध्रुव CheckType m_check;  // Process type we are checking

    // VISITORS
    शून्य visit(AstAssignDly* nodep) override {
        // Issue appropriate warning
        सचे (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } अञ्ञथा {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warnMore()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(नूतन AstAssign{
            nodep->fileline(),  //
            nodep->lhsp()->unlinkFrBack(),  //
            nodep->rhsp()->unlinkFrBack(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlinkFrBack() : nullptr});
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }

    //--------------------
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    ActiveDlyVisitor(Ast_ग्रन्थिः* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ActiveDlyVisitor() override = स्वभाव;
};

//######################################################################
// Active class functions

वग्ग ActiveVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ActiveNamer m_namer;  // Tracking of active names
    तार्किक m_clockedProcess = मिच्छा;  // Whether current process is a clocked process
    तार्किक m_allChanged = मिच्छा;  // Whether all SenItem in the SenTree are ET_CHANGED
    तार्किक m_walkingBody = मिच्छा;  // Walking body of a process
    तार्किक m_canBeComb = मिच्छा;  // Whether current clocked process can be turned into a comb process

    // METHODS
    template <typename T>
    शून्य moveUnderSpecial(Ast_ग्रन्थिः* nodep) {
        AstActive* ध्रुव wantactivep = m_namer.getSpecialActive<T>(nodep->fileline());
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);
    }

    शून्य visitAlways(Ast_ग्रन्थिः* nodep, Ast_Sen_वृक्षः* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        सचे (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
            पटिनिवत्त;
        }

        {
            ध्रुव VNUser1InUse user1InUse;

            // Walk sensitivity list
            m_clockedProcess = मिच्छा;
            m_allChanged = सच्च;
            सचे (oldsentreep) {
                oldsentreep->unlinkFrBack();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            सचे (m_allChanged) {
                ध्रुव VNUser2InUse user2InUse;
                m_walkingBody = सच्च;
                m_canBeComb = सच्च;
                iterateChildrenConst(nodep);
                m_walkingBody = मिच्छा;
                सचे (m_canBeComb) m_clockedProcess = मिच्छा;
            }
        }

        AstActive* ध्रुव wantactivep
            = !m_clockedProcess ? m_namer.getSpecialActive<AstSenItem::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.getActive(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.getSpecialActive<AstSenItem::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);

        // Warn and convert any delayed assignments
        सचे (!m_clockedProcess) ActiveDlyVisitor{nodep, ActiveDlyVisitor::CT_COMB};

        // Delete sensitivity list
        सचे (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_वृक्षः(), oldsentreep);

        // check combinational processes for latches
        सचे (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ध्रुव ActiveLatchCheckVisitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    शून्य visit_Sen_वस्तूनि(Ast_ग्रन्थिः* nodep) {
        सचे (v3Global.opt.timing().isSetTrue()) {
            nodep->foreach([एतत्](AstSenItem* senItemp) { visit(senItemp); });
        }
    }

    स्थिर शून्य markEventEdges(Ast_Sen_वृक्षः* sentreep) {
        अत्थाय (AstSenItem* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), SenItem)) {
            सचे (!senip->sensp()) अनुवर्त;
            सचे (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = senip->sensp()->dtypep()) {
                सचे (ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp()) {
                    सचे (basicp->isEvent()) senip->edgeType(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    शून्य visit(AstScope* nodep) override {
        m_namer.main(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    शून्य visit(AstActive* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    शून्य visit(AstInitialStatic* nodep) override { moveUnderSpecial<AstSenItem::Static>(nodep); }
    शून्य visit(AstInitial* nodep) override {
        visit_Sen_वस्तूनि(nodep);
        moveUnderSpecial<AstSenItem::Initial>(nodep);
    }
    शून्य visit(AstFinal* nodep) override {
        ध्रुव ActiveDlyVisitor dlyvisitor{nodep, ActiveDlyVisitor::CT_FINAL};
        moveUnderSpecial<AstSenItem::Final>(nodep);
    }
    शून्य visit(AstCoverToggle* nodep) override { moveUnderSpecial<AstSenItem::Combo>(nodep); }
    शून्य visit(AstAlways* nodep) override {
        सचे (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
            पटिनिवत्त;
        }
        सचे (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            moveUnderSpecial<AstSenItem::Combo>(nodep);
            पटिनिवत्त;
        }
        visit_Sen_वस्तूनि(nodep);
        visitAlways(nodep, nodep->sentreep(), nodep->keyword());
    }
    शून्य visit(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ध्रुव activep = m_namer.makeSpecialActive<AstSenItem::Combo>(nodep->fileline());
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    शून्य visit(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_वृक्षः* ध्रुव sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ध्रुव activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    शून्य visit(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_वृक्षः* ध्रुव sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ध्रुव activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    शून्य visit(AstCFunc* nodep) override { visit_Sen_वस्तूनि(nodep); }
    शून्य visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        सचे (!nodep->sensp()) पटिनिवत्त;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = सच्च;
        सचे (nodep->edgeType() != VEdgeType::ET_CHANGED) m_allChanged = मिच्छा;

        सचे (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->sensp()->dtypep()) {
            सचे (ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp()) {
                सचे (basicp->isEvent()) nodep->edgeType(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ध्रुव AstVarRef* refp) { refp->varScopep()->user1(सच्च); });
    }

    शून्य visit(AstVarRef* nodep) override {
        AstVarScope* ध्रुव vscp = nodep->varScopep();
        सचे (nodep->access().isWriteOnly()) {
            vscp->user2(सच्च);
        } अञ्ञथा {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            ध्रुव AstVar* ध्रुव varp = vscp->varp();
            सचे (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = मिच्छा;
            }
        }
    }
    शून्य visit(AstAssignDly* nodep) override {
        m_canBeComb = मिच्छा;
        सचे (nodep->isTimingControl()) m_clockedProcess = सच्च;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstFireEvent* nodep) override {
        m_canBeComb = मिच्छा;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstAssignForce* nodep) override {
        m_canBeComb = मिच्छा;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstRelease* nodep) override {
        m_canBeComb = मिच्छा;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstFork* nodep) override {
        सचे (nodep->isTimingControl()) {
            m_canBeComb = मिच्छा;
            m_clockedProcess = सच्च;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    शून्य visit(AstVar*) override {}  // Accelerate
    शून्य visit(AstVarScope*) override {}  // Accelerate
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        सचे (!v3Global.opt.timing().isSetTrue() && m_walkingBody && !m_canBeComb) {
            पटिनिवत्त;  // Accelerate
        }
        सचे (!nodep->isPure()) m_canBeComb = मिच्छा;
        सचे (nodep->isTimingControl()) {
            m_canBeComb = मिच्छा;
            m_clockedProcess = सच्च;
            पटिनिवत्त;
        }
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ActiveVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ActiveVisitor() override = स्वभाव;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

वग्ग CovergroupCollectVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = nullptr;  // Current covergroup class context, or nullptr

    // VISITORS
    शून्य visit(AstClass* nodep) override {
        सचे (!nodep->isCovergroup()) पटिनिवत्त;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    शून्य visit(AstScope* nodep) override { iterateChildren(nodep); }

    शून्य visit(AstCFunc* nodep) override {
        सचे (!m_classp) पटिनिवत्त;
        सचे (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    शून्य visit(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlinkFrBack());
        nodep->unlinkFrBack();
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = स्वभाव;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

वग्ग CovergroupEventBindVisitor final : सार्वजनिक VNVisitor {
    AstVarScope* ध्रुव m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_set<ध्रुव AstVar*> m_memberps;  // Non-static variables in the covergroup class

    शून्य visit(AstVarRef* nodep) override {
        सचे (!m_memberps.गणनम्(nodep->varp())) पटिनिवत्त;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        AstMemberSel* ध्रुव selp
            = नूतन AstMemberSel{fl, नूतन AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    CovergroupEventBindVisitor(Ast_Sen_वृक्षः* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* ध्रुव, AstVar* ध्रुव varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = स्वभाव;
};

वग्ग CovergroupInjectVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    ActiveNamer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    शून्य visit(AstScope* nodep) override {
        m_namer.main(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    शून्य visit(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ध्रुव varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ध्रुव AstClassRefDType* ध्रुव classRefp = VN_CAST(dtypep, ClassRefDType);
        सचे (!classRefp) पटिनिवत्त;

        AstClass* ध्रुव classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_वृक्षः* ध्रुव eventp = VN_CAST(classp->user2p(), Sen_वृक्षः);
        सचे (!eventp) पटिनिवत्त;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ध्रुव sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        AstVarRef* ध्रुव varrefp = नूतन AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* ध्रुव cmethodCallp
            = नूतन AstCMethodCall{fl, varrefp, sampleCFuncp, nullptr};

        cmethodCallp->dtypeSetVoid();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_वृक्षः* senTreep = eventp->clone_वृक्षः(मिच्छा);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        AstActive* ध्रुव activep = m_namer.getActive(fl, senTreep);
        VL_DO_DANGLING(प्रक्षिप_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->addStmtsp(
            नूतन AstAlways{fl, VAlwaysKwd::ALWAYS_FF, nullptr, cmethodCallp->makeStmt()});
    }

    शून्य visit(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        सचे (Ast_Sen_वृक्षः* ध्रुव eventp = VN_CAST(nodep->user2p(), Sen_वृक्षः)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(eventp), eventp);
        }
    }

    शून्य visit(AstActive*) override {}  // Don't iterate into actives
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = स्वभाव;
};

//######################################################################
// Active class functions

शून्य V3Active::activeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ActiveVisitor{nodep}; }  // Destruct before checking
    सचे (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ध्रुव VNUser1InUse user1InUse;
        ध्रुव VNUser2InUse user2InUse;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_वृक्षः("active", 0, dump_वृक्षः_Either_Level() >= 3);
}
