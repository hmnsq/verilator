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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Active.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

ವರ್ಗ LatchDetectGraphVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(LatchDetectGraphVertex, V3GraphVertex)
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ VertexType : uint8_t { VT_BLOCK, VT_BRANCH, VT_OUTPUT };

ಗುಪ್ತ:
    ಧ್ರುವ string m_name;  // Only used for .dot file generation
    ಧ್ರುವ VertexType m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ಧ್ರುವ VL_MT_SAFE {  //   "
        ವಿಕಲ್ಪ (m_type) {
        ಪಕ್ಷ VT_BLOCK: ಪ್ರತ್ಯಾಗಮ "(||)";  // basic block node
        ಪಕ್ಷ VT_BRANCH: ಪ್ರತ್ಯಾಗಮ "(&&)";  // if/else branch mode
        ಪಕ್ಷ VT_OUTPUT: ಪ್ರತ್ಯಾಗಮ "(out)";  // var assignment
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ "??";  // unknown
        }
    }

ಸಾರ್ವಜನಿಕ:
    LatchDetectGraphVertex(V3Graph* graphp, ಧ್ರುವ string& name, VertexType type = VT_BLOCK)
        : V3GraphVertex{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name + " " + typestr(); }
    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ user() ? "green" : "black"; }
    ಆಭಾಸ ಪೂರ್ಣಾಙ್ಕ type() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

ವರ್ಗ LatchDetectGraph final : ಸಾರ್ವಜನಿಕ V3Graph {
ರಕ್ಷಿತ:
    LatchDetectGraphVertex* m_curVertexp = nullptr;  // Current latch detection graph vertex
    std::vector<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    ಸ್ಥಿರ LatchDetectGraphVertex* castVertexp(ಶೂನ್ಯ* vertexp) {
        ಪ್ರತ್ಯಾಗಮ reinterpret_cast<LatchDetectGraphVertex*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    ತಾರ್ಕಿಕ latchCheckInternal(LatchDetectGraphVertex* vertexp) {
        ತಾರ್ಕಿಕ result = ಅಸತ್ಯ;
        ವಿಕಲ್ಪ (vertexp->type()) {
        ಪಕ್ಷ LatchDetectGraphVertex::VT_OUTPUT:  // Base case
            result = vertexp->user();
            ಭಙ್ಗ;
        ಪಕ್ಷ LatchDetectGraphVertex::VT_BLOCK:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            ಪ್ರತಿ (V3GraphEdge& edge : vertexp->outEdges()) {
                ಯದಿ (latchCheckInternal(castVertexp(edge.ತುದಿ()))) {
                    result = ಸತ್ಯ;
                    ಭಙ್ಗ;
                }
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ LatchDetectGraphVertex::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            LatchDetectGraphVertex* ಧ್ರುವ ifp = castVertexp(vertexp->outEdges().frontp()->ತುದಿ());
            LatchDetectGraphVertex* ಧ್ರುವ elsp = castVertexp(vertexp->outEdges().backp()->ತುದಿ());
            result = latchCheckInternal(ifp) && latchCheckInternal(elsp);
            ಭಙ್ಗ;
        }
        vertexp->user(result);
        ಪ್ರತ್ಯಾಗಮ result;
    }

ಸಾರ್ವಜನಿಕ:
    LatchDetectGraph() { clear(); }
    ~LatchDetectGraph() override { clear(); }
    // ACCESSORS
    LatchDetectGraphVertex* currentp() { ಪ್ರತ್ಯಾಗಮ m_curVertexp; }
    ಶೂನ್ಯ currentp(LatchDetectGraphVertex* vertex) { m_curVertexp = vertex; }
    // METHODS
    ಶೂನ್ಯ begin() {
        // Start a new if/else tracking graph
        m_curVertexp = ನೂತನ LatchDetectGraphVertex{ಏತತ್, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    ಶೂನ್ಯ clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3Graph::clear();
        m_curVertexp = nullptr;
    }
    // Add a new control path and connect it to its parent
    LatchDetectGraphVertex* addPathVertex(LatchDetectGraphVertex* parent, ಧ್ರುವ string& name,
                                          ತಾರ್ಕಿಕ branch = ಅಸತ್ಯ) {
        m_curVertexp = ನೂತನ LatchDetectGraphVertex{ಏತತ್, name,
                                                  branch ? LatchDetectGraphVertex::VT_BRANCH
                                                         : LatchDetectGraphVertex::VT_BLOCK};
        ನೂತನ V3GraphEdge{ಏತತ್, parent, m_curVertexp, 1};
        ಪ್ರತ್ಯಾಗಮ m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    LatchDetectGraphVertex* addOutputVertex(AstVarRef* nodep) {
        LatchDetectGraphVertex* ಧ್ರುವ outVertexp
            = ನೂತನ LatchDetectGraphVertex{ಏತತ್, nodep->name(), LatchDetectGraphVertex::VT_OUTPUT};
        nodep->varp()->user1p(outVertexp);
        m_outputs.ತಳ್ಳು_back(nodep);
        ಪ್ರತ್ಯಾಗಮ outVertexp;
    }
    // Connect an output assignment to its parent control block
    ಶೂನ್ಯ addAssignment(AstVarRef* nodep) {
        LatchDetectGraphVertex* outVertexp;
        ಯದಿ (!nodep->varp()->user1p()) {  // Not seen this output before
            outVertexp = addOutputVertex(nodep);
        } ಅನ್ಯಥಾ {
            outVertexp = castVertexp(nodep->varp()->user1p());
        }
        ನೂತನ V3GraphEdge{ಏತತ್, m_curVertexp, outVertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    ಶೂನ್ಯ latchCheck(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ latch_expected) {
        ತಾರ್ಕಿಕ latch_detected = ಅಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ vrp : m_outputs) {
            LatchDetectGraphVertex* ಧ್ರುವ vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(ಸತ್ಯ);  // Identify the output vertex we are checking paths _to_
            ಯದಿ (!latchCheckInternal(castVertexp(vertices().frontp()))) latch_detected = ಸತ್ಯ;
            ಯದಿ (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warnMore()
                        << "... Suggest use of always_latch for intentional latches");
                ಯದಿ (dumpGraphLevel() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(ಅಸತ್ಯ);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        ಯದಿ (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

ವರ್ಗ ActiveNamer final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope to add statement to
    AstActive* m_sActivep = nullptr;  // For current scope, the Static active we're building
    AstActive* m_iActivep = nullptr;  // For current scope, the Initial active we're building
    AstActive* m_fActivep = nullptr;  // For current scope, the Final active we're building
    AstActive* m_cActivep = nullptr;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_ವೃಕ್ಷ>, AstActive*> m_activeMap;

    // METHODS
    ಶೂನ್ಯ addActive(AstActive* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->addBlocksp(nodep);
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = nullptr;
        m_iActivep = nullptr;
        m_fActivep = nullptr;
        m_cActivep = nullptr;
        m_activeMap.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    ಶೂನ್ಯ visit(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    template <typename T_SenItemKind>
    AstActive*& getSpecialActive();

ಸಾರ್ವಜನಿಕ:
    // METHODS
    AstScope* scopep() { ಪ್ರತ್ಯಾಗಮ m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    AstActive* makeActive(File_ಸಾಲು* ಧ್ರುವ fl, Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep) {
        AstActive* ಧ್ರುವ activep = ನೂತನ AstActive{fl, "", senTreep};
        activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());
        addActive(activep);
        ಪ್ರತ್ಯಾಗಮ activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    template <typename T_SenItemKind>
    AstActive* makeSpecialActive(File_ಸಾಲು* ಧ್ರುವ fl) {
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = ನೂತನ Ast_Sen_ವೃಕ್ಷ{fl, ನೂತನ AstSenItem{fl, T_SenItemKind{}}};
        ಪ್ರತ್ಯಾಗಮ makeActive(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    template <typename T_SenItemKind>
    AstActive* getSpecialActive(File_ಸಾಲು* fl) {
        AstActive*& cachep = getSpecialActive<T_SenItemKind>();
        ಯದಿ (!cachep) cachep = makeSpecialActive<T_SenItemKind>(fl);
        ಪ್ರತ್ಯಾಗಮ cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    AstActive* getActive(File_ಸಾಲು* fl, Ast_Sen_ವೃಕ್ಷ* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        auto it = m_activeMap.find(*sentreep);
        // If found matching AstActive, return it
        ಯದಿ (it != m_activeMap.end()) ಪ್ರತ್ಯಾಗಮ it->second;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ newsenp = sentreep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        AstActive* ಧ್ರುವ activep = ನೂತನ AstActive{fl, "sequent", newsenp};
        activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());
        addActive(activep);
        m_activeMap.emplace(*newsenp, activep);
        ಪ್ರತ್ಯಾಗಮ activep;
    }

    // CONSTRUCTORS
    ActiveNamer() = ಸ್ವಭಾವ;
    ~ActiveNamer() override = ಸ್ವಭಾವ;
    ಶೂನ್ಯ main(AstScope* nodep) { iterate(nodep); }
};

template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Static>() {
    ಪ್ರತ್ಯಾಗಮ m_sActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Initial>() {
    ಪ್ರತ್ಯಾಗಮ m_iActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Final>() {
    ಪ್ರತ್ಯಾಗಮ m_fActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Combo>() {
    ಪ್ರತ್ಯಾಗಮ m_cActivep;
}

//######################################################################
// Latch checking visitor

ವರ್ಗ ActiveLatchCheckVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ಧ್ರುವ VNUser1InUse m_inuser1;
    // STATE
    LatchDetectGraph m_graph;  // Graph used to detect latches in combo always
    // VISITORS
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = nodep->varp();
        ಯದಿ (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->isUsedLoopIdx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_graph.addAssignment(nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        ಯದಿ (!nodep->isBoundsCheck()) {
            LatchDetectGraphVertex* ಧ್ರುವ parentp = m_graph.currentp();
            LatchDetectGraphVertex* ಧ್ರುವ branchp = m_graph.addPathVertex(parentp, "BRANCH", ಸತ್ಯ);
            m_graph.addPathVertex(branchp, "IF");
            iterateAndNextConstNull(nodep->thensp());
            m_graph.addPathVertex(branchp, "ELSE");
            iterateAndNextConstNull(nodep->elsesp());
            m_graph.currentp(parentp);
        } ಅನ್ಯಥಾ {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ActiveLatchCheckVisitor(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ expectLatch) {
        m_graph.begin();
        iterateConst(nodep);
        m_graph.latchCheck(nodep, expectLatch);
    }
    ~ActiveLatchCheckVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

ವರ್ಗ ActiveDlyVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ CheckType : uint8_t { CT_COMB, CT_FINAL };

ಗುಪ್ತ:
    // MEMBERS
    ಧ್ರುವ CheckType m_check;  // Process type we are checking

    // VISITORS
    ಶೂನ್ಯ visit(AstAssignDly* nodep) override {
        // Issue appropriate warning
        ಯದಿ (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } ಅನ್ಯಥಾ {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warnMore()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(ನೂತನ AstAssign{
            nodep->fileline(),  //
            nodep->lhsp()->unlinkFrBack(),  //
            nodep->rhsp()->unlinkFrBack(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlinkFrBack() : nullptr});
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }

    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ActiveDlyVisitor(Ast_ಗಂಟು* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ActiveDlyVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Active class functions

ವರ್ಗ ActiveVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ActiveNamer m_namer;  // Tracking of active names
    ತಾರ್ಕಿಕ m_clockedProcess = ಅಸತ್ಯ;  // Whether current process is a clocked process
    ತಾರ್ಕಿಕ m_allChanged = ಅಸತ್ಯ;  // Whether all SenItem in the SenTree are ET_CHANGED
    ತಾರ್ಕಿಕ m_walkingBody = ಅಸತ್ಯ;  // Walking body of a process
    ತಾರ್ಕಿಕ m_canBeComb = ಅಸತ್ಯ;  // Whether current clocked process can be turned into a comb process

    // METHODS
    template <typename T>
    ಶೂನ್ಯ moveUnderSpecial(Ast_ಗಂಟು* nodep) {
        AstActive* ಧ್ರುವ wantactivep = m_namer.getSpecialActive<T>(nodep->fileline());
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);
    }

    ಶೂನ್ಯ visitAlways(Ast_ಗಂಟು* nodep, Ast_Sen_ವೃಕ್ಷ* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        ಯದಿ (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        {
            ಧ್ರುವ VNUser1InUse user1InUse;

            // Walk sensitivity list
            m_clockedProcess = ಅಸತ್ಯ;
            m_allChanged = ಸತ್ಯ;
            ಯದಿ (oldsentreep) {
                oldsentreep->unlinkFrBack();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            ಯದಿ (m_allChanged) {
                ಧ್ರುವ VNUser2InUse user2InUse;
                m_walkingBody = ಸತ್ಯ;
                m_canBeComb = ಸತ್ಯ;
                iterateChildrenConst(nodep);
                m_walkingBody = ಅಸತ್ಯ;
                ಯದಿ (m_canBeComb) m_clockedProcess = ಅಸತ್ಯ;
            }
        }

        AstActive* ಧ್ರುವ wantactivep
            = !m_clockedProcess ? m_namer.getSpecialActive<AstSenItem::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.getActive(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.getSpecialActive<AstSenItem::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);

        // Warn and convert any delayed assignments
        ಯದಿ (!m_clockedProcess) ActiveDlyVisitor{nodep, ActiveDlyVisitor::CT_COMB};

        // Delete sensitivity list
        ಯದಿ (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_ವೃಕ್ಷ(), oldsentreep);

        // check combinational processes for latches
        ಯದಿ (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ಧ್ರುವ ActiveLatchCheckVisitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    ಶೂನ್ಯ visit_Sen_ವಸ್ತುಗಳು(Ast_ಗಂಟು* nodep) {
        ಯದಿ (v3Global.opt.timing().isSetTrue()) {
            nodep->foreach([ಏತತ್](AstSenItem* senItemp) { visit(senItemp); });
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ markEventEdges(Ast_Sen_ವೃಕ್ಷ* sentreep) {
        ಪ್ರತಿ (AstSenItem* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), SenItem)) {
            ಯದಿ (!senip->sensp()) ಅನುವರ್ತ;
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = senip->sensp()->dtypep()) {
                ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = dtypep->basicp()) {
                    ಯದಿ (basicp->isEvent()) senip->edgeType(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        m_namer.main(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstActive* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    ಶೂನ್ಯ visit(AstInitialStatic* nodep) override { moveUnderSpecial<AstSenItem::Static>(nodep); }
    ಶೂನ್ಯ visit(AstInitial* nodep) override {
        visit_Sen_ವಸ್ತುಗಳು(nodep);
        moveUnderSpecial<AstSenItem::Initial>(nodep);
    }
    ಶೂನ್ಯ visit(AstFinal* nodep) override {
        ಧ್ರುವ ActiveDlyVisitor dlyvisitor{nodep, ActiveDlyVisitor::CT_FINAL};
        moveUnderSpecial<AstSenItem::Final>(nodep);
    }
    ಶೂನ್ಯ visit(AstCoverToggle* nodep) override { moveUnderSpecial<AstSenItem::Combo>(nodep); }
    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        ಯದಿ (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            moveUnderSpecial<AstSenItem::Combo>(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        visit_Sen_ವಸ್ತುಗಳು(nodep);
        visitAlways(nodep, nodep->sentreep(), nodep->keyword());
    }
    ಶೂನ್ಯ visit(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ಧ್ರುವ activep = m_namer.makeSpecialActive<AstSenItem::Combo>(nodep->fileline());
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    ಶೂನ್ಯ visit(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ಧ್ರುವ activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    ಶೂನ್ಯ visit(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ಧ್ರುವ activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override { visit_Sen_ವಸ್ತುಗಳು(nodep); }
    ಶೂನ್ಯ visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        ಯದಿ (!nodep->sensp()) ಪ್ರತ್ಯಾಗಮ;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = ಸತ್ಯ;
        ಯದಿ (nodep->edgeType() != VEdgeType::ET_CHANGED) m_allChanged = ಅಸತ್ಯ;

        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->sensp()->dtypep()) {
            ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = dtypep->basicp()) {
                ಯದಿ (basicp->isEvent()) nodep->edgeType(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ಧ್ರುವ AstVarRef* refp) { refp->varScopep()->user1(ಸತ್ಯ); });
    }

    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        AstVarScope* ಧ್ರುವ vscp = nodep->varScopep();
        ಯದಿ (nodep->access().isWriteOnly()) {
            vscp->user2(ಸತ್ಯ);
        } ಅನ್ಯಥಾ {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            ಧ್ರುವ AstVar* ಧ್ರುವ varp = vscp->varp();
            ಯದಿ (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = ಅಸತ್ಯ;
            }
        }
    }
    ಶೂನ್ಯ visit(AstAssignDly* nodep) override {
        m_canBeComb = ಅಸತ್ಯ;
        ಯದಿ (nodep->isTimingControl()) m_clockedProcess = ಸತ್ಯ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstFireEvent* nodep) override {
        m_canBeComb = ಅಸತ್ಯ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstAssignForce* nodep) override {
        m_canBeComb = ಅಸತ್ಯ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstRelease* nodep) override {
        m_canBeComb = ಅಸತ್ಯ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        ಯದಿ (nodep->isTimingControl()) {
            m_canBeComb = ಅಸತ್ಯ;
            m_clockedProcess = ಸತ್ಯ;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    ಶೂನ್ಯ visit(AstVar*) override {}  // Accelerate
    ಶೂನ್ಯ visit(AstVarScope*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (!v3Global.opt.timing().isSetTrue() && m_walkingBody && !m_canBeComb) {
            ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        }
        ಯದಿ (!nodep->isPure()) m_canBeComb = ಅಸತ್ಯ;
        ಯದಿ (nodep->isTimingControl()) {
            m_canBeComb = ಅಸತ್ಯ;
            m_clockedProcess = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit ActiveVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ActiveVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

ವರ್ಗ CovergroupCollectVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = nullptr;  // Current covergroup class context, or nullptr

    // VISITORS
    ಶೂನ್ಯ visit(AstClass* nodep) override {
        ಯದಿ (!nodep->isCovergroup()) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstScope* nodep) override { iterateChildren(nodep); }

    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        ಯದಿ (!m_classp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    ಶೂನ್ಯ visit(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlinkFrBack());
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

ವರ್ಗ CovergroupEventBindVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    AstVarScope* ಧ್ರುವ m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_set<ಧ್ರುವ AstVar*> m_memberps;  // Non-static variables in the covergroup class

    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಯದಿ (!m_memberps.ಎಣಿಕೆ(nodep->varp())) ಪ್ರತ್ಯಾಗಮ;
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        AstMemberSel* ಧ್ರುವ selp
            = ನೂತನ AstMemberSel{fl, ನೂತನ AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    CovergroupEventBindVisitor(Ast_Sen_ವೃಕ್ಷ* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* ಧ್ರುವ, AstVar* ಧ್ರುವ varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = ಸ್ವಭಾವ;
};

ವರ್ಗ CovergroupInjectVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    ActiveNamer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        m_namer.main(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ಧ್ರುವ varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classRefp = VN_CAST(dtypep, ClassRefDType);
        ಯದಿ (!classRefp) ಪ್ರತ್ಯಾಗಮ;

        AstClass* ಧ್ರುವ classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ eventp = VN_CAST(classp->user2p(), Sen_ವೃಕ್ಷ);
        ಯದಿ (!eventp) ಪ್ರತ್ಯಾಗಮ;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ಧ್ರುವ sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        AstVarRef* ಧ್ರುವ varrefp = ನೂತನ AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* ಧ್ರುವ cmethodCallp
            = ನೂತನ AstCMethodCall{fl, varrefp, sampleCFuncp, nullptr};

        cmethodCallp->dtypeSetVoid();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_ವೃಕ್ಷ* senTreep = eventp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        AstActive* ಧ್ರುವ activep = m_namer.getActive(fl, senTreep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->addStmtsp(
            ನೂತನ AstAlways{fl, VAlwaysKwd::ALWAYS_FF, nullptr, cmethodCallp->makeStmt()});
    }

    ಶೂನ್ಯ visit(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        ಯದಿ (Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ eventp = VN_CAST(nodep->user2p(), Sen_ವೃಕ್ಷ)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(eventp), eventp);
        }
    }

    ಶೂನ್ಯ visit(AstActive*) override {}  // Don't iterate into actives
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Active class functions

ಶೂನ್ಯ V3Active::activeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ActiveVisitor{nodep}; }  // Destruct before checking
    ಯದಿ (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ಧ್ರುವ VNUser1InUse user1InUse;
        ಧ್ರುವ VNUser2InUse user2InUse;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("active", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
