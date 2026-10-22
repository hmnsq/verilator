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

वर्ग LatchDetectGraphVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(LatchDetectGraphVertex, V3GraphVertex)
सार्वजनिक:
    गणना VertexType : uint8_t { VT_BLOCK, VT_BRANCH, VT_OUTPUT };

गुप्त:
    ध्रुव string m_name;  // Only used for .dot file generation
    ध्रुव VertexType m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ध्रुव VL_MT_SAFE {  //   "
        विकल्प (m_type) {
        पक्ष VT_BLOCK: प्रत्यागम "(||)";  // basic block node
        पक्ष VT_BRANCH: प्रत्यागम "(&&)";  // if/else branch mode
        पक्ष VT_OUTPUT: प्रत्यागम "(out)";  // var assignment
        स्वभाव: प्रत्यागम "??";  // unknown
        }
    }

सार्वजनिक:
    LatchDetectGraphVertex(V3Graph* graphp, ध्रुव string& name, VertexType type = VT_BLOCK)
        : V3GraphVertex{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name + " " + typestr(); }
    string dotColor() ध्रुव override { प्रत्यागम user() ? "green" : "black"; }
    आभास पूर्णाङ्क type() ध्रुव { प्रत्यागम m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

वर्ग LatchDetectGraph final : सार्वजनिक V3Graph {
रक्षित:
    LatchDetectGraphVertex* m_curVertexp = nullptr;  // Current latch detection graph vertex
    std::vector<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    स्थिर LatchDetectGraphVertex* castVertexp(शून्य* vertexp) {
        प्रत्यागम reinterpret_cast<LatchDetectGraphVertex*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    तार्किक latchCheckInternal(LatchDetectGraphVertex* vertexp) {
        तार्किक result = असत्य;
        विकल्प (vertexp->type()) {
        पक्ष LatchDetectGraphVertex::VT_OUTPUT:  // Base case
            result = vertexp->user();
            भङ्ग;
        पक्ष LatchDetectGraphVertex::VT_BLOCK:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            प्रति (V3GraphEdge& edge : vertexp->outEdges()) {
                यदि (latchCheckInternal(castVertexp(edge.शीर्ष()))) {
                    result = सत्य;
                    भङ्ग;
                }
            }
            भङ्ग;
        पक्ष LatchDetectGraphVertex::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            LatchDetectGraphVertex* ध्रुव ifp = castVertexp(vertexp->outEdges().frontp()->शीर्ष());
            LatchDetectGraphVertex* ध्रुव elsp = castVertexp(vertexp->outEdges().backp()->शीर्ष());
            result = latchCheckInternal(ifp) && latchCheckInternal(elsp);
            भङ्ग;
        }
        vertexp->user(result);
        प्रत्यागम result;
    }

सार्वजनिक:
    LatchDetectGraph() { clear(); }
    ~LatchDetectGraph() override { clear(); }
    // ACCESSORS
    LatchDetectGraphVertex* currentp() { प्रत्यागम m_curVertexp; }
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
                                          तार्किक branch = असत्य) {
        m_curVertexp = नूतन LatchDetectGraphVertex{एतत्, name,
                                                  branch ? LatchDetectGraphVertex::VT_BRANCH
                                                         : LatchDetectGraphVertex::VT_BLOCK};
        नूतन V3GraphEdge{एतत्, parent, m_curVertexp, 1};
        प्रत्यागम m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    LatchDetectGraphVertex* addOutputVertex(AstVarRef* nodep) {
        LatchDetectGraphVertex* ध्रुव outVertexp
            = नूतन LatchDetectGraphVertex{एतत्, nodep->name(), LatchDetectGraphVertex::VT_OUTPUT};
        nodep->varp()->user1p(outVertexp);
        m_outputs.धकेलो_back(nodep);
        प्रत्यागम outVertexp;
    }
    // Connect an output assignment to its parent control block
    शून्य addAssignment(AstVarRef* nodep) {
        LatchDetectGraphVertex* outVertexp;
        यदि (!nodep->varp()->user1p()) {  // Not seen this output before
            outVertexp = addOutputVertex(nodep);
        } अन्यथा {
            outVertexp = castVertexp(nodep->varp()->user1p());
        }
        नूतन V3GraphEdge{एतत्, m_curVertexp, outVertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    शून्य latchCheck(Ast_गाँठ* nodep, तार्किक latch_expected) {
        तार्किक latch_detected = असत्य;
        प्रति (ध्रुव AstVarRef* ध्रुव vrp : m_outputs) {
            LatchDetectGraphVertex* ध्रुव vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(सत्य);  // Identify the output vertex we are checking paths _to_
            यदि (!latchCheckInternal(castVertexp(vertices().frontp()))) latch_detected = सत्य;
            यदि (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warnMore()
                        << "... Suggest use of always_latch for intentional latches");
                यदि (dumpGraphLevel() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(असत्य);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        यदि (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

वर्ग ActiveNamer final : सार्वजनिक VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope to add statement to
    AstActive* m_sActivep = nullptr;  // For current scope, the Static active we're building
    AstActive* m_iActivep = nullptr;  // For current scope, the Initial active we're building
    AstActive* m_fActivep = nullptr;  // For current scope, the Final active we're building
    AstActive* m_cActivep = nullptr;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_वृक्ष>, AstActive*> m_activeMap;

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
    शून्य visit(Ast_Sen_वृक्ष* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    शून्य visit(Ast_गाँठ_Stmt*) override {}  // Accelerate
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    ढाँचा <typename T_SenItemKind>
    AstActive*& getSpecialActive();

सार्वजनिक:
    // METHODS
    AstScope* scopep() { प्रत्यागम m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    AstActive* makeActive(File_पंक्ति* ध्रुव fl, Ast_Sen_वृक्ष* ध्रुव senTreep) {
        AstActive* ध्रुव activep = नूतन AstActive{fl, "", senTreep};
        activep->sen_वृक्ष_Storep(activep->sentreep());
        addActive(activep);
        प्रत्यागम activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    ढाँचा <typename T_SenItemKind>
    AstActive* makeSpecialActive(File_पंक्ति* ध्रुव fl) {
        Ast_Sen_वृक्ष* ध्रुव senTreep = नूतन Ast_Sen_वृक्ष{fl, नूतन AstSenItem{fl, T_SenItemKind{}}};
        प्रत्यागम makeActive(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    ढाँचा <typename T_SenItemKind>
    AstActive* getSpecialActive(File_पंक्ति* fl) {
        AstActive*& cachep = getSpecialActive<T_SenItemKind>();
        यदि (!cachep) cachep = makeSpecialActive<T_SenItemKind>(fl);
        प्रत्यागम cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    AstActive* getActive(File_पंक्ति* fl, Ast_Sen_वृक्ष* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        स्वतः it = m_activeMap.find(*sentreep);
        // If found matching AstActive, return it
        यदि (it != m_activeMap.end()) प्रत्यागम it->second;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_वृक्ष* ध्रुव newsenp = sentreep->clone_वृक्ष(असत्य);
        AstActive* ध्रुव activep = नूतन AstActive{fl, "sequent", newsenp};
        activep->sen_वृक्ष_Storep(activep->sentreep());
        addActive(activep);
        m_activeMap.emplace(*newsenp, activep);
        प्रत्यागम activep;
    }

    // CONSTRUCTORS
    ActiveNamer() = स्वभाव;
    ~ActiveNamer() override = स्वभाव;
    शून्य main(AstScope* nodep) { iterate(nodep); }
};

ढाँचा <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Static>() {
    प्रत्यागम m_sActivep;
}
ढाँचा <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Initial>() {
    प्रत्यागम m_iActivep;
}
ढाँचा <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Final>() {
    प्रत्यागम m_fActivep;
}
ढाँचा <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Combo>() {
    प्रत्यागम m_cActivep;
}

//######################################################################
// Latch checking visitor

वर्ग ActiveLatchCheckVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ध्रुव VNUser1InUse m_inuser1;
    // STATE
    LatchDetectGraph m_graph;  // Graph used to detect latches in combo always
    // VISITORS
    शून्य visit(AstVarRef* nodep) override {
        ध्रुव AstVar* ध्रुव varp = nodep->varp();
        यदि (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->isUsedLoopIdx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_graph.addAssignment(nodep);
        }
    }
    शून्य visit(Ast_गाँठ_If* nodep) override {
        यदि (!nodep->isBoundsCheck()) {
            LatchDetectGraphVertex* ध्रुव parentp = m_graph.currentp();
            LatchDetectGraphVertex* ध्रुव branchp = m_graph.addPathVertex(parentp, "BRANCH", सत्य);
            m_graph.addPathVertex(branchp, "IF");
            iterateAndNextConstNull(nodep->thensp());
            m_graph.addPathVertex(branchp, "ELSE");
            iterateAndNextConstNull(nodep->elsesp());
            m_graph.currentp(parentp);
        } अन्यथा {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    ActiveLatchCheckVisitor(Ast_गाँठ* nodep, तार्किक expectLatch) {
        m_graph.begin();
        iterateConst(nodep);
        m_graph.latchCheck(nodep, expectLatch);
    }
    ~ActiveLatchCheckVisitor() override = स्वभाव;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

वर्ग ActiveDlyVisitor final : सार्वजनिक VNVisitor {
सार्वजनिक:
    गणना CheckType : uint8_t { CT_COMB, CT_FINAL };

गुप्त:
    // MEMBERS
    ध्रुव CheckType m_check;  // Process type we are checking

    // VISITORS
    शून्य visit(AstAssignDly* nodep) override {
        // Issue appropriate warning
        यदि (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } अन्यथा {
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
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }

    //--------------------
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    ActiveDlyVisitor(Ast_गाँठ* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ActiveDlyVisitor() override = स्वभाव;
};

//######################################################################
// Active class functions

वर्ग ActiveVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ActiveNamer m_namer;  // Tracking of active names
    तार्किक m_clockedProcess = असत्य;  // Whether current process is a clocked process
    तार्किक m_allChanged = असत्य;  // Whether all SenItem in the SenTree are ET_CHANGED
    तार्किक m_walkingBody = असत्य;  // Walking body of a process
    तार्किक m_canBeComb = असत्य;  // Whether current clocked process can be turned into a comb process

    // METHODS
    ढाँचा <typename T>
    शून्य moveUnderSpecial(Ast_गाँठ* nodep) {
        AstActive* ध्रुव wantactivep = m_namer.getSpecialActive<T>(nodep->fileline());
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);
    }

    शून्य visitAlways(Ast_गाँठ* nodep, Ast_Sen_वृक्ष* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        यदि (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            प्रत्यागम;
        }

        {
            ध्रुव VNUser1InUse user1InUse;

            // Walk sensitivity list
            m_clockedProcess = असत्य;
            m_allChanged = सत्य;
            यदि (oldsentreep) {
                oldsentreep->unlinkFrBack();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            यदि (m_allChanged) {
                ध्रुव VNUser2InUse user2InUse;
                m_walkingBody = सत्य;
                m_canBeComb = सत्य;
                iterateChildrenConst(nodep);
                m_walkingBody = असत्य;
                यदि (m_canBeComb) m_clockedProcess = असत्य;
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
        यदि (!m_clockedProcess) ActiveDlyVisitor{nodep, ActiveDlyVisitor::CT_COMB};

        // Delete sensitivity list
        यदि (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_वृक्ष(), oldsentreep);

        // check combinational processes for latches
        यदि (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ध्रुव ActiveLatchCheckVisitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    शून्य visit_Sen_वस्तुएँ(Ast_गाँठ* nodep) {
        यदि (v3Global.opt.timing().isSetTrue()) {
            nodep->foreach([एतत्](AstSenItem* senItemp) { visit(senItemp); });
        }
    }

    स्थिर शून्य markEventEdges(Ast_Sen_वृक्ष* sentreep) {
        प्रति (AstSenItem* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), SenItem)) {
            यदि (!senip->sensp()) अनुवर्त;
            यदि (ध्रुव Ast_गाँठ_DType* ध्रुव dtypep = senip->sensp()->dtypep()) {
                यदि (ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp()) {
                    यदि (basicp->isEvent()) senip->edgeType(VEdgeType::ET_EVENT);
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
        visit_Sen_वस्तुएँ(nodep);
        moveUnderSpecial<AstSenItem::Initial>(nodep);
    }
    शून्य visit(AstFinal* nodep) override {
        ध्रुव ActiveDlyVisitor dlyvisitor{nodep, ActiveDlyVisitor::CT_FINAL};
        moveUnderSpecial<AstSenItem::Final>(nodep);
    }
    शून्य visit(AstCoverToggle* nodep) override { moveUnderSpecial<AstSenItem::Combo>(nodep); }
    शून्य visit(AstAlways* nodep) override {
        यदि (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            प्रत्यागम;
        }
        यदि (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            moveUnderSpecial<AstSenItem::Combo>(nodep);
            प्रत्यागम;
        }
        visit_Sen_वस्तुएँ(nodep);
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
        Ast_Sen_वृक्ष* ध्रुव sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ध्रुव activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    शून्य visit(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_वृक्ष* ध्रुव sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ध्रुव activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    शून्य visit(AstCFunc* nodep) override { visit_Sen_वस्तुएँ(nodep); }
    शून्य visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        यदि (!nodep->sensp()) प्रत्यागम;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = सत्य;
        यदि (nodep->edgeType() != VEdgeType::ET_CHANGED) m_allChanged = असत्य;

        यदि (ध्रुव Ast_गाँठ_DType* ध्रुव dtypep = nodep->sensp()->dtypep()) {
            यदि (ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp()) {
                यदि (basicp->isEvent()) nodep->edgeType(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ध्रुव AstVarRef* refp) { refp->varScopep()->user1(सत्य); });
    }

    शून्य visit(AstVarRef* nodep) override {
        AstVarScope* ध्रुव vscp = nodep->varScopep();
        यदि (nodep->access().isWriteOnly()) {
            vscp->user2(सत्य);
        } अन्यथा {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            ध्रुव AstVar* ध्रुव varp = vscp->varp();
            यदि (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = असत्य;
            }
        }
    }
    शून्य visit(AstAssignDly* nodep) override {
        m_canBeComb = असत्य;
        यदि (nodep->isTimingControl()) m_clockedProcess = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstFireEvent* nodep) override {
        m_canBeComb = असत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstAssignForce* nodep) override {
        m_canBeComb = असत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstRelease* nodep) override {
        m_canBeComb = असत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstFork* nodep) override {
        यदि (nodep->isTimingControl()) {
            m_canBeComb = असत्य;
            m_clockedProcess = सत्य;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    शून्य visit(AstVar*) override {}  // Accelerate
    शून्य visit(AstVarScope*) override {}  // Accelerate
    शून्य visit(Ast_गाँठ* nodep) override {
        यदि (!v3Global.opt.timing().isSetTrue() && m_walkingBody && !m_canBeComb) {
            प्रत्यागम;  // Accelerate
        }
        यदि (!nodep->isPure()) m_canBeComb = असत्य;
        यदि (nodep->isTimingControl()) {
            m_canBeComb = असत्य;
            m_clockedProcess = सत्य;
            प्रत्यागम;
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

वर्ग CovergroupCollectVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = nullptr;  // Current covergroup class context, or nullptr

    // VISITORS
    शून्य visit(AstClass* nodep) override {
        यदि (!nodep->isCovergroup()) प्रत्यागम;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    शून्य visit(AstScope* nodep) override { iterateChildren(nodep); }

    शून्य visit(AstCFunc* nodep) override {
        यदि (!m_classp) प्रत्यागम;
        यदि (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    शून्य visit(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlinkFrBack());
        nodep->unlinkFrBack();
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = स्वभाव;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

वर्ग CovergroupEventBindVisitor final : सार्वजनिक VNVisitor {
    AstVarScope* ध्रुव m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_set<ध्रुव AstVar*> m_memberps;  // Non-static variables in the covergroup class

    शून्य visit(AstVarRef* nodep) override {
        यदि (!m_memberps.गिनती(nodep->varp())) प्रत्यागम;
        File_पंक्ति* ध्रुव fl = nodep->fileline();
        AstMemberSel* ध्रुव selp
            = नूतन AstMemberSel{fl, नूतन AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    CovergroupEventBindVisitor(Ast_Sen_वृक्ष* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* ध्रुव, AstVar* ध्रुव varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = स्वभाव;
};

वर्ग CovergroupInjectVisitor final : सार्वजनिक VNVisitor {
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
        ध्रुव Ast_गाँठ_DType* ध्रुव dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ध्रुव AstClassRefDType* ध्रुव classRefp = VN_CAST(dtypep, ClassRefDType);
        यदि (!classRefp) प्रत्यागम;

        AstClass* ध्रुव classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_वृक्ष* ध्रुव eventp = VN_CAST(classp->user2p(), Sen_वृक्ष);
        यदि (!eventp) प्रत्यागम;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ध्रुव sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_पंक्ति* ध्रुव fl = nodep->fileline();
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
        Ast_Sen_वृक्ष* senTreep = eventp->clone_वृक्ष(असत्य);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        AstActive* ध्रुव activep = m_namer.getActive(fl, senTreep);
        VL_DO_DANGLING(धकेलो_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->addStmtsp(
            नूतन AstAlways{fl, VAlwaysKwd::ALWAYS_FF, nullptr, cmethodCallp->makeStmt()});
    }

    शून्य visit(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        यदि (Ast_Sen_वृक्ष* ध्रुव eventp = VN_CAST(nodep->user2p(), Sen_वृक्ष)) {
            VL_DO_DANGLING(धकेलो_Deletep(eventp), eventp);
        }
    }

    शून्य visit(AstActive*) override {}  // Don't iterate into actives
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

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
    यदि (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ध्रुव VNUser1InUse user1InUse;
        ध्रुव VNUser2InUse user2InUse;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_वृक्ष("active", 0, dump_वृक्ष_Either_Level() >= 3);
}
