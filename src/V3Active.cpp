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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Active.h"

#समावेश "V3Const.h"
#समावेश "V3Graph.h"

#समावेश <unordered_map>
#समावेश <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

श्रेणी LatchDetectGraphVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(LatchDetectGraphVertex, V3GraphVertex)
खुला:
    क्रमागत VertexType : uint8_t { VT_BLOCK, VT_BRANCH, VT_OUTPUT };

निजी:
    स्थिर string m_name;  // Only used for .dot file generation
    स्थिर VertexType m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() स्थिर VL_MT_SAFE {  //   "
        चयन (m_type) {
        हाल VT_BLOCK: वापस "(||)";  // basic block node
        हाल VT_BRANCH: वापस "(&&)";  // if/else branch mode
        हाल VT_OUTPUT: वापस "(out)";  // var assignment
        शेष: वापस "??";  // unknown
        }
    }

खुला:
    LatchDetectGraphVertex(V3Graph* graphp, स्थिर string& name, VertexType type = VT_BLOCK)
        : V3GraphVertex{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() स्थिर override VL_MT_STABLE { वापस m_name + " " + typestr(); }
    string dotColor() स्थिर override { वापस user() ? "green" : "black"; }
    भव पूर्णांक type() स्थिर { वापस m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

श्रेणी LatchDetectGraph final : खुला V3Graph {
रक्षित:
    LatchDetectGraphVertex* m_curVertexp = nullptr;  // Current latch detection graph vertex
    std::vector<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    अटल LatchDetectGraphVertex* castVertexp(व्योम* vertexp) {
        वापस reinterpret_cast<LatchDetectGraphVertex*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    bool latchCheckInternal(LatchDetectGraphVertex* vertexp) {
        bool परिणाम = false;
        चयन (vertexp->type()) {
        हाल LatchDetectGraphVertex::VT_OUTPUT:  // Base case
            परिणाम = vertexp->user();
            अवरोध;
        हाल LatchDetectGraphVertex::VT_BLOCK:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            क्रम (V3GraphEdge& edge : vertexp->outEdges()) {
                अगर (latchCheckInternal(castVertexp(edge.शीर्ष()))) {
                    परिणाम = true;
                    अवरोध;
                }
            }
            अवरोध;
        हाल LatchDetectGraphVertex::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            LatchDetectGraphVertex* स्थिर ifp = castVertexp(vertexp->outEdges().frontp()->शीर्ष());
            LatchDetectGraphVertex* स्थिर elsp = castVertexp(vertexp->outEdges().backp()->शीर्ष());
            परिणाम = latchCheckInternal(ifp) && latchCheckInternal(elsp);
            अवरोध;
        }
        vertexp->user(परिणाम);
        वापस परिणाम;
    }

खुला:
    LatchDetectGraph() { सफा(); }
    ~LatchDetectGraph() override { सफा(); }
    // ACCESSORS
    LatchDetectGraphVertex* currentp() { वापस m_curVertexp; }
    व्योम currentp(LatchDetectGraphVertex* vertex) { m_curVertexp = vertex; }
    // METHODS
    व्योम begin() {
        // Start a new if/else tracking graph
        m_curVertexp = नया LatchDetectGraphVertex{यह, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    व्योम सफा() {
        m_outputs.सफा();
        // Calling base class clear will unlink & delete all edges & vertices
        V3Graph::सफा();
        m_curVertexp = nullptr;
    }
    // Add a new control path and connect it to its parent
    LatchDetectGraphVertex* addPathVertex(LatchDetectGraphVertex* parent, स्थिर string& name,
                                          bool branch = false) {
        m_curVertexp = नया LatchDetectGraphVertex{यह, name,
                                                  branch ? LatchDetectGraphVertex::VT_BRANCH
                                                         : LatchDetectGraphVertex::VT_BLOCK};
        नया V3GraphEdge{यह, parent, m_curVertexp, 1};
        वापस m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    LatchDetectGraphVertex* addOutputVertex(AstVarRef* nodep) {
        LatchDetectGraphVertex* स्थिर outVertexp
            = नया LatchDetectGraphVertex{यह, nodep->name(), LatchDetectGraphVertex::VT_OUTPUT};
        nodep->varp()->user1p(outVertexp);
        m_outputs.धकेलो_back(nodep);
        वापस outVertexp;
    }
    // Connect an output assignment to its parent control block
    व्योम addAssignment(AstVarRef* nodep) {
        LatchDetectGraphVertex* outVertexp;
        अगर (!nodep->varp()->user1p()) {  // Not seen this output before
            outVertexp = addOutputVertex(nodep);
        } अन्यथा {
            outVertexp = castVertexp(nodep->varp()->user1p());
        }
        नया V3GraphEdge{यह, m_curVertexp, outVertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    व्योम latchCheck(Ast_गाँठ* nodep, bool latch_expected) {
        bool latch_detected = false;
        क्रम (स्थिर AstVarRef* स्थिर vrp : m_outputs) {
            LatchDetectGraphVertex* स्थिर vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(true);  // Identify the output vertex we are checking paths _to_
            अगर (!latchCheckInternal(castVertexp(vertices().frontp()))) latch_detected = true;
            अगर (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warnMore()
                        << "... Suggest use of always_latch for intentional latches");
                अगर (dumpGraphLevel() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(false);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        अगर (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

श्रेणी ActiveNamer final : खुला VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope to add statement to
    AstActive* m_sActivep = nullptr;  // For current scope, the Static active we're building
    AstActive* m_iActivep = nullptr;  // For current scope, the Initial active we're building
    AstActive* m_fActivep = nullptr;  // For current scope, the Final active we're building
    AstActive* m_cActivep = nullptr;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_वृक्ष>, AstActive*> m_activeMap;

    // METHODS
    व्योम addActive(AstActive* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->addBlocksp(nodep);
    }

    // VISITORS
    व्योम visit(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = nullptr;
        m_iActivep = nullptr;
        m_fActivep = nullptr;
        m_cActivep = nullptr;
        m_activeMap.सफा();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    व्योम visit(Ast_Sen_वृक्ष* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    व्योम visit(Ast_गाँठ_Stmt*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    ढाँचा <typename T_SenItemKind>
    AstActive*& getSpecialActive();

खुला:
    // METHODS
    AstScope* scopep() { वापस m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    AstActive* makeActive(File_पंक्ति* स्थिर fl, Ast_Sen_वृक्ष* स्थिर senTreep) {
        AstActive* स्थिर activep = नया AstActive{fl, "", senTreep};
        activep->sen_वृक्ष_Storep(activep->sentreep());
        addActive(activep);
        वापस activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    ढाँचा <typename T_SenItemKind>
    AstActive* makeSpecialActive(File_पंक्ति* स्थिर fl) {
        Ast_Sen_वृक्ष* स्थिर senTreep = नया Ast_Sen_वृक्ष{fl, नया AstSenItem{fl, T_SenItemKind{}}};
        वापस makeActive(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    ढाँचा <typename T_SenItemKind>
    AstActive* getSpecialActive(File_पंक्ति* fl) {
        AstActive*& cachep = getSpecialActive<T_SenItemKind>();
        अगर (!cachep) cachep = makeSpecialActive<T_SenItemKind>(fl);
        वापस cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    AstActive* getActive(File_पंक्ति* fl, Ast_Sen_वृक्ष* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        स्वतः it = m_activeMap.find(*sentreep);
        // If found matching AstActive, return it
        अगर (it != m_activeMap.end()) वापस it->second;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_वृक्ष* स्थिर newsenp = sentreep->clone_वृक्ष(false);
        AstActive* स्थिर activep = नया AstActive{fl, "sequent", newsenp};
        activep->sen_वृक्ष_Storep(activep->sentreep());
        addActive(activep);
        m_activeMap.emplace(*newsenp, activep);
        वापस activep;
    }

    // CONSTRUCTORS
    ActiveNamer() = शेष;
    ~ActiveNamer() override = शेष;
    व्योम मुख्य(AstScope* nodep) { iterate(nodep); }
};

ढाँचा <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Static>() {
    वापस m_sActivep;
}
ढाँचा <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Initial>() {
    वापस m_iActivep;
}
ढाँचा <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Final>() {
    वापस m_fActivep;
}
ढाँचा <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Combo>() {
    वापस m_cActivep;
}

//######################################################################
// Latch checking visitor

श्रेणी ActiveLatchCheckVisitor final : खुला VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    स्थिर VNUser1InUse m_inuser1;
    // STATE
    LatchDetectGraph m_graph;  // Graph used to detect latches in combo always
    // VISITORS
    व्योम visit(AstVarRef* nodep) override {
        स्थिर AstVar* स्थिर varp = nodep->varp();
        अगर (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->isUsedLoopIdx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_graph.addAssignment(nodep);
        }
    }
    व्योम visit(Ast_गाँठ_If* nodep) override {
        अगर (!nodep->isBoundsCheck()) {
            LatchDetectGraphVertex* स्थिर parentp = m_graph.currentp();
            LatchDetectGraphVertex* स्थिर branchp = m_graph.addPathVertex(parentp, "BRANCH", true);
            m_graph.addPathVertex(branchp, "IF");
            iterate_And_अग्र_Const_Null(nodep->thensp());
            m_graph.addPathVertex(branchp, "ELSE");
            iterate_And_अग्र_Const_Null(nodep->elsesp());
            m_graph.currentp(parentp);
        } अन्यथा {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    // CONSTRUCTORS
    ActiveLatchCheckVisitor(Ast_गाँठ* nodep, bool expectLatch) {
        m_graph.begin();
        iterateConst(nodep);
        m_graph.latchCheck(nodep, expectLatch);
    }
    ~ActiveLatchCheckVisitor() override = शेष;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

श्रेणी ActiveDlyVisitor final : खुला VNVisitor {
खुला:
    क्रमागत CheckType : uint8_t { CT_COMB, CT_FINAL };

निजी:
    // MEMBERS
    स्थिर CheckType m_check;  // Process type we are checking

    // VISITORS
    व्योम visit(AstAssignDly* nodep) override {
        // Issue appropriate warning
        अगर (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } अन्यथा {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warnMore()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(नया AstAssign{
            nodep->fileline(),  //
            nodep->lhsp()->unlinkFrBack(),  //
            nodep->rhsp()->unlinkFrBack(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlinkFrBack() : nullptr});
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }

    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    ActiveDlyVisitor(Ast_गाँठ* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ActiveDlyVisitor() override = शेष;
};

//######################################################################
// Active class functions

श्रेणी ActiveVisitor final : खुला VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ActiveNamer m_namer;  // Tracking of active names
    bool m_clockedProcess = false;  // Whether current process is a clocked process
    bool m_allChanged = false;  // Whether all SenItem in the SenTree are ET_CHANGED
    bool m_walkingBody = false;  // Walking body of a process
    bool m_canBeComb = false;  // Whether current clocked process can be turned into a comb process

    // METHODS
    ढाँचा <typename T>
    व्योम moveUnderSpecial(Ast_गाँठ* nodep) {
        AstActive* स्थिर wantactivep = m_namer.getSpecialActive<T>(nodep->fileline());
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);
    }

    व्योम visitAlways(Ast_गाँठ* nodep, Ast_Sen_वृक्ष* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        अगर (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            वापस;
        }

        {
            स्थिर VNUser1InUse user1InUse;

            // Walk sensitivity list
            m_clockedProcess = false;
            m_allChanged = true;
            अगर (oldsentreep) {
                oldsentreep->unlinkFrBack();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            अगर (m_allChanged) {
                स्थिर VNUser2InUse user2InUse;
                m_walkingBody = true;
                m_canBeComb = true;
                iterateChildrenConst(nodep);
                m_walkingBody = false;
                अगर (m_canBeComb) m_clockedProcess = false;
            }
        }

        AstActive* स्थिर wantactivep
            = !m_clockedProcess ? m_namer.getSpecialActive<AstSenItem::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.getActive(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.getSpecialActive<AstSenItem::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);

        // Warn and convert any delayed assignments
        अगर (!m_clockedProcess) ActiveDlyVisitor{nodep, ActiveDlyVisitor::CT_COMB};

        // Delete sensitivity list
        अगर (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_वृक्ष(), oldsentreep);

        // check combinational processes for latches
        अगर (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            स्थिर ActiveLatchCheckVisitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    व्योम visit_Sen_वस्तुएँ(Ast_गाँठ* nodep) {
        अगर (v3Global.opt.timing().isSetTrue()) {
            nodep->foreach([यह](AstSenItem* senItemp) { visit(senItemp); });
        }
    }

    अटल व्योम markEventEdges(Ast_Sen_वृक्ष* sentreep) {
        क्रम (AstSenItem* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), SenItem)) {
            अगर (!senip->sensp()) जारी;
            अगर (स्थिर Ast_गाँठ_DType* स्थिर dtypep = senip->sensp()->dtypep()) {
                अगर (स्थिर AstBasicDType* स्थिर basicp = dtypep->basicp()) {
                    अगर (basicp->isEvent()) senip->edgeType(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    व्योम visit(AstScope* nodep) override {
        m_namer.मुख्य(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    व्योम visit(AstActive* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    व्योम visit(AstInitialStatic* nodep) override { moveUnderSpecial<AstSenItem::Static>(nodep); }
    व्योम visit(AstInitial* nodep) override {
        visit_Sen_वस्तुएँ(nodep);
        moveUnderSpecial<AstSenItem::Initial>(nodep);
    }
    व्योम visit(AstFinal* nodep) override {
        स्थिर ActiveDlyVisitor dlyvisitor{nodep, ActiveDlyVisitor::CT_FINAL};
        moveUnderSpecial<AstSenItem::Final>(nodep);
    }
    व्योम visit(AstCoverToggle* nodep) override { moveUnderSpecial<AstSenItem::Combo>(nodep); }
    व्योम visit(AstAlways* nodep) override {
        अगर (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            वापस;
        }
        अगर (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            moveUnderSpecial<AstSenItem::Combo>(nodep);
            वापस;
        }
        visit_Sen_वस्तुएँ(nodep);
        visitAlways(nodep, nodep->sentreep(), nodep->keyword());
    }
    व्योम visit(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* स्थिर activep = m_namer.makeSpecialActive<AstSenItem::Combo>(nodep->fileline());
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    व्योम visit(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_वृक्ष* स्थिर sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* स्थिर activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    व्योम visit(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_वृक्ष* स्थिर sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* स्थिर activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    व्योम visit(AstCFunc* nodep) override { visit_Sen_वस्तुएँ(nodep); }
    व्योम visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        अगर (!nodep->sensp()) वापस;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = true;
        अगर (nodep->edgeType() != VEdgeType::ET_CHANGED) m_allChanged = false;

        अगर (स्थिर Ast_गाँठ_DType* स्थिर dtypep = nodep->sensp()->dtypep()) {
            अगर (स्थिर AstBasicDType* स्थिर basicp = dtypep->basicp()) {
                अगर (basicp->isEvent()) nodep->edgeType(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](स्थिर AstVarRef* refp) { refp->varScopep()->user1(true); });
    }

    व्योम visit(AstVarRef* nodep) override {
        AstVarScope* स्थिर vscp = nodep->varScopep();
        अगर (nodep->access().isWriteOnly()) {
            vscp->user2(true);
        } अन्यथा {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            स्थिर AstVar* स्थिर varp = vscp->varp();
            अगर (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = false;
            }
        }
    }
    व्योम visit(AstAssignDly* nodep) override {
        m_canBeComb = false;
        अगर (nodep->isTimingControl()) m_clockedProcess = true;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstFireEvent* nodep) override {
        m_canBeComb = false;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstAssignForce* nodep) override {
        m_canBeComb = false;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstRelease* nodep) override {
        m_canBeComb = false;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstFork* nodep) override {
        अगर (nodep->isTimingControl()) {
            m_canBeComb = false;
            m_clockedProcess = true;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    व्योम visit(AstVar*) override {}  // Accelerate
    व्योम visit(AstVarScope*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (!v3Global.opt.timing().isSetTrue() && m_walkingBody && !m_canBeComb) {
            वापस;  // Accelerate
        }
        अगर (!nodep->isPure()) m_canBeComb = false;
        अगर (nodep->isTimingControl()) {
            m_canBeComb = false;
            m_clockedProcess = true;
            वापस;
        }
        iterateChildren(nodep);
    }

खुला:
    // CONSTRUCTORS
    explicit ActiveVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ActiveVisitor() override = शेष;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

श्रेणी CovergroupCollectVisitor final : खुला VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = nullptr;  // Current covergroup class context, or nullptr

    // VISITORS
    व्योम visit(AstClass* nodep) override {
        अगर (!nodep->isCovergroup()) वापस;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    व्योम visit(AstScope* nodep) override { iterateChildren(nodep); }

    व्योम visit(AstCFunc* nodep) override {
        अगर (!m_classp) वापस;
        अगर (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    व्योम visit(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlinkFrBack());
        nodep->unlinkFrBack();
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = शेष;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

श्रेणी CovergroupEventBindVisitor final : खुला VNVisitor {
    AstVarScope* स्थिर m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_set<स्थिर AstVar*> m_memberps;  // Non-static variables in the covergroup class

    व्योम visit(AstVarRef* nodep) override {
        अगर (!m_memberps.गिनती(nodep->varp())) वापस;
        File_पंक्ति* स्थिर fl = nodep->fileline();
        AstMemberSel* स्थिर selp
            = नया AstMemberSel{fl, नया AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    CovergroupEventBindVisitor(Ast_Sen_वृक्ष* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* स्थिर, AstVar* स्थिर varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = शेष;
};

श्रेणी CovergroupInjectVisitor final : खुला VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    ActiveNamer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    व्योम visit(AstScope* nodep) override {
        m_namer.मुख्य(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    व्योम visit(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* स्थिर varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        स्थिर Ast_गाँठ_DType* स्थिर dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        स्थिर AstClassRefDType* स्थिर classRefp = VN_CAST(dtypep, ClassRefDType);
        अगर (!classRefp) वापस;

        AstClass* स्थिर classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_वृक्ष* स्थिर eventp = VN_CAST(classp->user2p(), Sen_वृक्ष);
        अगर (!eventp) वापस;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* स्थिर sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_पंक्ति* स्थिर fl = nodep->fileline();
        AstVarRef* स्थिर varrefp = नया AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* स्थिर cmethodCallp
            = नया AstCMethodCall{fl, varrefp, sampleCFuncp, nullptr};

        cmethodCallp->dtypeSetVoid();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_वृक्ष* senTreep = eventp->clone_वृक्ष(false);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        AstActive* स्थिर activep = m_namer.getActive(fl, senTreep);
        VL_DO_DANGLING(धकेलो_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->addStmtsp(
            नया AstAlways{fl, VAlwaysKwd::ALWAYS_FF, nullptr, cmethodCallp->makeStmt()});
    }

    व्योम visit(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        अगर (Ast_Sen_वृक्ष* स्थिर eventp = VN_CAST(nodep->user2p(), Sen_वृक्ष)) {
            VL_DO_DANGLING(धकेलो_Deletep(eventp), eventp);
        }
    }

    व्योम visit(AstActive*) override {}  // Don't iterate into actives
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = शेष;
};

//######################################################################
// Active class functions

व्योम V3Active::activeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ActiveVisitor{nodep}; }  // Destruct before checking
    अगर (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        स्थिर VNUser1InUse user1InUse;
        स्थिर VNUser2InUse user2InUse;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_वृक्ष("active", 0, dump_वृक्ष_Either_Level() >= 3);
}
