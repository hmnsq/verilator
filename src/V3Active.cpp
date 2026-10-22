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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Active.h"

#ضمن "V3Const.h"
#ضمن "V3Graph.h"

#ضمن <unordered_map>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

صنف LatchDetectGraphVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(LatchDetectGraphVertex, V3GraphVertex)
عام:
    تعداد VertexType : uint8_t { VT_BLOCK, VT_BRANCH, VT_OUTPUT };

خاص:
    ثابت string m_name;  // Only used for .dot file generation
    ثابت VertexType m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ثابت VL_MT_SAFE {  //   "
        بدل (m_type) {
        حالة VT_BLOCK: أرجع "(||)";  // basic block node
        حالة VT_BRANCH: أرجع "(&&)";  // if/else branch mode
        حالة VT_OUTPUT: أرجع "(out)";  // var assignment
        افتراضي: أرجع "??";  // unknown
        }
    }

عام:
    LatchDetectGraphVertex(V3Graph* graphp, ثابت string& name, VertexType type = VT_BLOCK)
        : V3GraphVertex{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ثابت override VL_MT_STABLE { أرجع m_name + " " + typestr(); }
    string dotColor() ثابت override { أرجع user() ? "green" : "black"; }
    virtual صحيح type() ثابت { أرجع m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

صنف LatchDetectGraph final : عام V3Graph {
محمي:
    LatchDetectGraphVertex* m_curVertexp = nullptr;  // Current latch detection graph vertex
    std::vector<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    ساكن LatchDetectGraphVertex* castVertexp(فراغ* vertexp) {
        أرجع reinterpret_cast<LatchDetectGraphVertex*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    منطقي latchCheckInternal(LatchDetectGraphVertex* vertexp) {
        منطقي result = خطأ;
        بدل (vertexp->type()) {
        حالة LatchDetectGraphVertex::VT_OUTPUT:  // Base case
            result = vertexp->user();
            اكسر;
        حالة LatchDetectGraphVertex::VT_BLOCK:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            لكل (V3GraphEdge& edge : vertexp->outEdges()) {
                إذا (latchCheckInternal(castVertexp(edge.قمة()))) {
                    result = true;
                    اكسر;
                }
            }
            اكسر;
        حالة LatchDetectGraphVertex::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            LatchDetectGraphVertex* ثابت ifp = castVertexp(vertexp->outEdges().frontp()->قمة());
            LatchDetectGraphVertex* ثابت elsp = castVertexp(vertexp->outEdges().backp()->قمة());
            result = latchCheckInternal(ifp) && latchCheckInternal(elsp);
            اكسر;
        }
        vertexp->user(result);
        أرجع result;
    }

عام:
    LatchDetectGraph() { clear(); }
    ~LatchDetectGraph() override { clear(); }
    // ACCESSORS
    LatchDetectGraphVertex* currentp() { أرجع m_curVertexp; }
    فراغ currentp(LatchDetectGraphVertex* vertex) { m_curVertexp = vertex; }
    // METHODS
    فراغ begin() {
        // Start a new if/else tracking graph
        m_curVertexp = جديد LatchDetectGraphVertex{هذا, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    فراغ clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3Graph::clear();
        m_curVertexp = nullptr;
    }
    // Add a new control path and connect it to its parent
    LatchDetectGraphVertex* addPathVertex(LatchDetectGraphVertex* parent, ثابت string& name,
                                          منطقي branch = خطأ) {
        m_curVertexp = جديد LatchDetectGraphVertex{هذا, name,
                                                  branch ? LatchDetectGraphVertex::VT_BRANCH
                                                         : LatchDetectGraphVertex::VT_BLOCK};
        جديد V3GraphEdge{هذا, parent, m_curVertexp, 1};
        أرجع m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    LatchDetectGraphVertex* addOutputVertex(AstVarRef* nodep) {
        LatchDetectGraphVertex* ثابت outVertexp
            = جديد LatchDetectGraphVertex{هذا, nodep->name(), LatchDetectGraphVertex::VT_OUTPUT};
        nodep->varp()->user1p(outVertexp);
        m_outputs.ادفع_back(nodep);
        أرجع outVertexp;
    }
    // Connect an output assignment to its parent control block
    فراغ addAssignment(AstVarRef* nodep) {
        LatchDetectGraphVertex* outVertexp;
        إذا (!nodep->varp()->user1p()) {  // Not seen this output before
            outVertexp = addOutputVertex(nodep);
        } وإلا {
            outVertexp = castVertexp(nodep->varp()->user1p());
        }
        جديد V3GraphEdge{هذا, m_curVertexp, outVertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    فراغ latchCheck(Ast_عقدة* nodep, منطقي latch_expected) {
        منطقي latch_detected = خطأ;
        لكل (ثابت AstVarRef* ثابت vrp : m_outputs) {
            LatchDetectGraphVertex* ثابت vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(true);  // Identify the output vertex we are checking paths _to_
            إذا (!latchCheckInternal(castVertexp(vertices().frontp()))) latch_detected = true;
            إذا (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warnMore()
                        << "... Suggest use of always_latch for intentional latches");
                إذا (dumpGraphLevel() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(خطأ);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        إذا (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

صنف ActiveNamer final : عام VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope to add statement to
    AstActive* m_sActivep = nullptr;  // For current scope, the Static active we're building
    AstActive* m_iActivep = nullptr;  // For current scope, the Initial active we're building
    AstActive* m_fActivep = nullptr;  // For current scope, the Final active we're building
    AstActive* m_cActivep = nullptr;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_شجرة>, AstActive*> m_activeMap;

    // METHODS
    فراغ addActive(AstActive* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->addBlocksp(nodep);
    }

    // VISITORS
    فراغ visit(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = nullptr;
        m_iActivep = nullptr;
        m_fActivep = nullptr;
        m_cActivep = nullptr;
        m_activeMap.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    فراغ visit(Ast_Sen_شجرة* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    فراغ visit(Ast_عقدة_Stmt*) override {}  // Accelerate
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    template <typename T_SenItemKind>
    AstActive*& getSpecialActive();

عام:
    // METHODS
    AstScope* scopep() { أرجع m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    AstActive* makeActive(File_سطر* ثابت fl, Ast_Sen_شجرة* ثابت senTreep) {
        AstActive* ثابت activep = جديد AstActive{fl, "", senTreep};
        activep->sen_شجرة_Storep(activep->sentreep());
        addActive(activep);
        أرجع activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    template <typename T_SenItemKind>
    AstActive* makeSpecialActive(File_سطر* ثابت fl) {
        Ast_Sen_شجرة* ثابت senTreep = جديد Ast_Sen_شجرة{fl, جديد AstSenItem{fl, T_SenItemKind{}}};
        أرجع makeActive(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    template <typename T_SenItemKind>
    AstActive* getSpecialActive(File_سطر* fl) {
        AstActive*& cachep = getSpecialActive<T_SenItemKind>();
        إذا (!cachep) cachep = makeSpecialActive<T_SenItemKind>(fl);
        أرجع cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    AstActive* getActive(File_سطر* fl, Ast_Sen_شجرة* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        auto it = m_activeMap.find(*sentreep);
        // If found matching AstActive, return it
        إذا (it != m_activeMap.end()) أرجع it->second;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_شجرة* ثابت newsenp = sentreep->clone_شجرة(خطأ);
        AstActive* ثابت activep = جديد AstActive{fl, "sequent", newsenp};
        activep->sen_شجرة_Storep(activep->sentreep());
        addActive(activep);
        m_activeMap.emplace(*newsenp, activep);
        أرجع activep;
    }

    // CONSTRUCTORS
    ActiveNamer() = افتراضي;
    ~ActiveNamer() override = افتراضي;
    فراغ main(AstScope* nodep) { iterate(nodep); }
};

template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Static>() {
    أرجع m_sActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Initial>() {
    أرجع m_iActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Final>() {
    أرجع m_fActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Combo>() {
    أرجع m_cActivep;
}

//######################################################################
// Latch checking visitor

صنف ActiveLatchCheckVisitor final : عام VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ثابت VNUser1InUse m_inuser1;
    // STATE
    LatchDetectGraph m_graph;  // Graph used to detect latches in combo always
    // VISITORS
    فراغ visit(AstVarRef* nodep) override {
        ثابت AstVar* ثابت varp = nodep->varp();
        إذا (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->isUsedLoopIdx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_graph.addAssignment(nodep);
        }
    }
    فراغ visit(Ast_عقدة_If* nodep) override {
        إذا (!nodep->isBoundsCheck()) {
            LatchDetectGraphVertex* ثابت parentp = m_graph.currentp();
            LatchDetectGraphVertex* ثابت branchp = m_graph.addPathVertex(parentp, "BRANCH", true);
            m_graph.addPathVertex(branchp, "IF");
            iterateAndNextConstNull(nodep->thensp());
            m_graph.addPathVertex(branchp, "ELSE");
            iterateAndNextConstNull(nodep->elsesp());
            m_graph.currentp(parentp);
        } وإلا {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    فراغ visit(Ast_عقدة* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    ActiveLatchCheckVisitor(Ast_عقدة* nodep, منطقي expectLatch) {
        m_graph.begin();
        iterateConst(nodep);
        m_graph.latchCheck(nodep, expectLatch);
    }
    ~ActiveLatchCheckVisitor() override = افتراضي;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

صنف ActiveDlyVisitor final : عام VNVisitor {
عام:
    تعداد CheckType : uint8_t { CT_COMB, CT_FINAL };

خاص:
    // MEMBERS
    ثابت CheckType m_check;  // Process type we are checking

    // VISITORS
    فراغ visit(AstAssignDly* nodep) override {
        // Issue appropriate warning
        إذا (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } وإلا {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warnMore()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(جديد AstAssign{
            nodep->fileline(),  //
            nodep->lhsp()->unlinkFrBack(),  //
            nodep->rhsp()->unlinkFrBack(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlinkFrBack() : nullptr});
        VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
    }

    //--------------------
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    ActiveDlyVisitor(Ast_عقدة* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ActiveDlyVisitor() override = افتراضي;
};

//######################################################################
// Active class functions

صنف ActiveVisitor final : عام VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ActiveNamer m_namer;  // Tracking of active names
    منطقي m_clockedProcess = خطأ;  // Whether current process is a clocked process
    منطقي m_allChanged = خطأ;  // Whether all SenItem in the SenTree are ET_CHANGED
    منطقي m_walkingBody = خطأ;  // Walking body of a process
    منطقي m_canBeComb = خطأ;  // Whether current clocked process can be turned into a comb process

    // METHODS
    template <typename T>
    فراغ moveUnderSpecial(Ast_عقدة* nodep) {
        AstActive* ثابت wantactivep = m_namer.getSpecialActive<T>(nodep->fileline());
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);
    }

    فراغ visitAlways(Ast_عقدة* nodep, Ast_Sen_شجرة* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        إذا (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_شجرة(), nodep);
            أرجع;
        }

        {
            ثابت VNUser1InUse user1InUse;

            // Walk sensitivity list
            m_clockedProcess = خطأ;
            m_allChanged = true;
            إذا (oldsentreep) {
                oldsentreep->unlinkFrBack();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            إذا (m_allChanged) {
                ثابت VNUser2InUse user2InUse;
                m_walkingBody = true;
                m_canBeComb = true;
                iterateChildrenConst(nodep);
                m_walkingBody = خطأ;
                إذا (m_canBeComb) m_clockedProcess = خطأ;
            }
        }

        AstActive* ثابت wantactivep
            = !m_clockedProcess ? m_namer.getSpecialActive<AstSenItem::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.getActive(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.getSpecialActive<AstSenItem::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);

        // Warn and convert any delayed assignments
        إذا (!m_clockedProcess) ActiveDlyVisitor{nodep, ActiveDlyVisitor::CT_COMB};

        // Delete sensitivity list
        إذا (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_شجرة(), oldsentreep);

        // check combinational processes for latches
        إذا (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ثابت ActiveLatchCheckVisitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    فراغ visit_Sen_عناصر(Ast_عقدة* nodep) {
        إذا (v3Global.opt.timing().isSetTrue()) {
            nodep->foreach([هذا](AstSenItem* senItemp) { visit(senItemp); });
        }
    }

    ساكن فراغ markEventEdges(Ast_Sen_شجرة* sentreep) {
        لكل (AstSenItem* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), SenItem)) {
            إذا (!senip->sensp()) استمر;
            إذا (ثابت Ast_عقدة_DType* ثابت dtypep = senip->sensp()->dtypep()) {
                إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                    إذا (basicp->isEvent()) senip->edgeType(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    فراغ visit(AstScope* nodep) override {
        m_namer.main(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    فراغ visit(AstActive* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    فراغ visit(AstInitialStatic* nodep) override { moveUnderSpecial<AstSenItem::Static>(nodep); }
    فراغ visit(AstInitial* nodep) override {
        visit_Sen_عناصر(nodep);
        moveUnderSpecial<AstSenItem::Initial>(nodep);
    }
    فراغ visit(AstFinal* nodep) override {
        ثابت ActiveDlyVisitor dlyvisitor{nodep, ActiveDlyVisitor::CT_FINAL};
        moveUnderSpecial<AstSenItem::Final>(nodep);
    }
    فراغ visit(AstCoverToggle* nodep) override { moveUnderSpecial<AstSenItem::Combo>(nodep); }
    فراغ visit(AstAlways* nodep) override {
        إذا (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_شجرة(), nodep);
            أرجع;
        }
        إذا (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            moveUnderSpecial<AstSenItem::Combo>(nodep);
            أرجع;
        }
        visit_Sen_عناصر(nodep);
        visitAlways(nodep, nodep->sentreep(), nodep->keyword());
    }
    فراغ visit(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ثابت activep = m_namer.makeSpecialActive<AstSenItem::Combo>(nodep->fileline());
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    فراغ visit(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_شجرة* ثابت sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ثابت activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    فراغ visit(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_شجرة* ثابت sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ثابت activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    فراغ visit(AstCFunc* nodep) override { visit_Sen_عناصر(nodep); }
    فراغ visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        إذا (!nodep->sensp()) أرجع;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = true;
        إذا (nodep->edgeType() != VEdgeType::ET_CHANGED) m_allChanged = خطأ;

        إذا (ثابت Ast_عقدة_DType* ثابت dtypep = nodep->sensp()->dtypep()) {
            إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                إذا (basicp->isEvent()) nodep->edgeType(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ثابت AstVarRef* refp) { refp->varScopep()->user1(true); });
    }

    فراغ visit(AstVarRef* nodep) override {
        AstVarScope* ثابت vscp = nodep->varScopep();
        إذا (nodep->access().isWriteOnly()) {
            vscp->user2(true);
        } وإلا {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            ثابت AstVar* ثابت varp = vscp->varp();
            إذا (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = خطأ;
            }
        }
    }
    فراغ visit(AstAssignDly* nodep) override {
        m_canBeComb = خطأ;
        إذا (nodep->isTimingControl()) m_clockedProcess = true;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstFireEvent* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstAssignForce* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstRelease* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstFork* nodep) override {
        إذا (nodep->isTimingControl()) {
            m_canBeComb = خطأ;
            m_clockedProcess = true;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    فراغ visit(AstVar*) override {}  // Accelerate
    فراغ visit(AstVarScope*) override {}  // Accelerate
    فراغ visit(Ast_عقدة* nodep) override {
        إذا (!v3Global.opt.timing().isSetTrue() && m_walkingBody && !m_canBeComb) {
            أرجع;  // Accelerate
        }
        إذا (!nodep->isPure()) m_canBeComb = خطأ;
        إذا (nodep->isTimingControl()) {
            m_canBeComb = خطأ;
            m_clockedProcess = true;
            أرجع;
        }
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    explicit ActiveVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ActiveVisitor() override = افتراضي;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

صنف CovergroupCollectVisitor final : عام VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = nullptr;  // Current covergroup class context, or nullptr

    // VISITORS
    فراغ visit(AstClass* nodep) override {
        إذا (!nodep->isCovergroup()) أرجع;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    فراغ visit(AstScope* nodep) override { iterateChildren(nodep); }

    فراغ visit(AstCFunc* nodep) override {
        إذا (!m_classp) أرجع;
        إذا (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    فراغ visit(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlinkFrBack());
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }

    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = افتراضي;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

صنف CovergroupEventBindVisitor final : عام VNVisitor {
    AstVarScope* ثابت m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_set<ثابت AstVar*> m_memberps;  // Non-static variables in the covergroup class

    فراغ visit(AstVarRef* nodep) override {
        إذا (!m_memberps.عدد(nodep->varp())) أرجع;
        File_سطر* ثابت fl = nodep->fileline();
        AstMemberSel* ثابت selp
            = جديد AstMemberSel{fl, جديد AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    CovergroupEventBindVisitor(Ast_Sen_شجرة* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* ثابت, AstVar* ثابت varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = افتراضي;
};

صنف CovergroupInjectVisitor final : عام VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    ActiveNamer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    فراغ visit(AstScope* nodep) override {
        m_namer.main(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    فراغ visit(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ثابت varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ثابت Ast_عقدة_DType* ثابت dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
        إذا (!classRefp) أرجع;

        AstClass* ثابت classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_شجرة* ثابت eventp = VN_CAST(classp->user2p(), Sen_شجرة);
        إذا (!eventp) أرجع;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ثابت sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_سطر* ثابت fl = nodep->fileline();
        AstVarRef* ثابت varrefp = جديد AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* ثابت cmethodCallp
            = جديد AstCMethodCall{fl, varrefp, sampleCFuncp, nullptr};

        cmethodCallp->dtypeSetVoid();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_شجرة* senTreep = eventp->clone_شجرة(خطأ);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        AstActive* ثابت activep = m_namer.getActive(fl, senTreep);
        VL_DO_DANGLING(ادفع_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->addStmtsp(
            جديد AstAlways{fl, VAlwaysKwd::ALWAYS_FF, nullptr, cmethodCallp->makeStmt()});
    }

    فراغ visit(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        إذا (Ast_Sen_شجرة* ثابت eventp = VN_CAST(nodep->user2p(), Sen_شجرة)) {
            VL_DO_DANGLING(ادفع_Deletep(eventp), eventp);
        }
    }

    فراغ visit(AstActive*) override {}  // Don't iterate into actives
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = افتراضي;
};

//######################################################################
// Active class functions

فراغ V3Active::activeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ActiveVisitor{nodep}; }  // Destruct before checking
    إذا (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ثابت VNUser1InUse user1InUse;
        ثابت VNUser2InUse user2InUse;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_شجرة("active", 0, dump_شجرة_Either_Level() >= 3);
}
