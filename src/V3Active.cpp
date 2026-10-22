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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Active.h"

#شامل "V3Const.h"
#شامل "V3Graph.h"

#شامل <unordered_map>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

رده LatchDetectGraphVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(LatchDetectGraphVertex, V3GraphVertex)
عمومی:
    شمارش VertexType : uint8_t { VT_BLOCK, VT_BRANCH, VT_OUTPUT };

خصوصی:
    ثابت string m_name;  // Only used for .dot file generation
    ثابت VertexType m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ثابت VL_MT_SAFE {  //   "
        انتخاب (m_type) {
        حالت VT_BLOCK: بازگشت "(||)";  // basic block node
        حالت VT_BRANCH: بازگشت "(&&)";  // if/else branch mode
        حالت VT_OUTPUT: بازگشت "(out)";  // var assignment
        پیشفرض: بازگشت "??";  // unknown
        }
    }

عمومی:
    LatchDetectGraphVertex(V3Graph* graphp, ثابت string& name, VertexType type = VT_BLOCK)
        : V3GraphVertex{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ثابت override VL_MT_STABLE { بازگشت m_name + " " + typestr(); }
    string dotColor() ثابت override { بازگشت user() ? "green" : "black"; }
    مجازی صحیح type() ثابت { بازگشت m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

رده LatchDetectGraph final : عمومی V3Graph {
محافظتشده:
    LatchDetectGraphVertex* m_curVertexp = nullptr;  // Current latch detection graph vertex
    std::vector<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    ایستا LatchDetectGraphVertex* castVertexp(تهی* vertexp) {
        بازگشت reinterpret_cast<LatchDetectGraphVertex*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    منطقی latchCheckInternal(LatchDetectGraphVertex* vertexp) {
        منطقی result = نادرست;
        انتخاب (vertexp->type()) {
        حالت LatchDetectGraphVertex::VT_OUTPUT:  // Base case
            result = vertexp->user();
            بشکن;
        حالت LatchDetectGraphVertex::VT_BLOCK:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            برای (V3GraphEdge& edge : vertexp->outEdges()) {
                اگر (latchCheckInternal(castVertexp(edge.بالاترین()))) {
                    result = درست;
                    بشکن;
                }
            }
            بشکن;
        حالت LatchDetectGraphVertex::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            LatchDetectGraphVertex* ثابت ifp = castVertexp(vertexp->outEdges().frontp()->بالاترین());
            LatchDetectGraphVertex* ثابت elsp = castVertexp(vertexp->outEdges().backp()->بالاترین());
            result = latchCheckInternal(ifp) && latchCheckInternal(elsp);
            بشکن;
        }
        vertexp->user(result);
        بازگشت result;
    }

عمومی:
    LatchDetectGraph() { clear(); }
    ~LatchDetectGraph() override { clear(); }
    // ACCESSORS
    LatchDetectGraphVertex* currentp() { بازگشت m_curVertexp; }
    تهی currentp(LatchDetectGraphVertex* vertex) { m_curVertexp = vertex; }
    // METHODS
    تهی begin() {
        // Start a new if/else tracking graph
        m_curVertexp = جدید LatchDetectGraphVertex{این, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    تهی clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3Graph::clear();
        m_curVertexp = nullptr;
    }
    // Add a new control path and connect it to its parent
    LatchDetectGraphVertex* addPathVertex(LatchDetectGraphVertex* parent, ثابت string& name,
                                          منطقی branch = نادرست) {
        m_curVertexp = جدید LatchDetectGraphVertex{این, name,
                                                  branch ? LatchDetectGraphVertex::VT_BRANCH
                                                         : LatchDetectGraphVertex::VT_BLOCK};
        جدید V3GraphEdge{این, parent, m_curVertexp, 1};
        بازگشت m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    LatchDetectGraphVertex* addOutputVertex(AstVarRef* nodep) {
        LatchDetectGraphVertex* ثابت outVertexp
            = جدید LatchDetectGraphVertex{این, nodep->name(), LatchDetectGraphVertex::VT_OUTPUT};
        nodep->varp()->user1p(outVertexp);
        m_outputs.افزودن_back(nodep);
        بازگشت outVertexp;
    }
    // Connect an output assignment to its parent control block
    تهی addAssignment(AstVarRef* nodep) {
        LatchDetectGraphVertex* outVertexp;
        اگر (!nodep->varp()->user1p()) {  // Not seen this output before
            outVertexp = addOutputVertex(nodep);
        } وگرنه {
            outVertexp = castVertexp(nodep->varp()->user1p());
        }
        جدید V3GraphEdge{این, m_curVertexp, outVertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    تهی latchCheck(Ast_گره* nodep, منطقی latch_expected) {
        منطقی latch_detected = نادرست;
        برای (ثابت AstVarRef* ثابت vrp : m_outputs) {
            LatchDetectGraphVertex* ثابت vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(درست);  // Identify the output vertex we are checking paths _to_
            اگر (!latchCheckInternal(castVertexp(vertices().frontp()))) latch_detected = درست;
            اگر (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warnMore()
                        << "... Suggest use of always_latch for intentional latches");
                اگر (dumpGraphLevel() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(نادرست);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        اگر (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

رده ActiveNamer final : عمومی VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope to add statement to
    AstActive* m_sActivep = nullptr;  // For current scope, the Static active we're building
    AstActive* m_iActivep = nullptr;  // For current scope, the Initial active we're building
    AstActive* m_fActivep = nullptr;  // For current scope, the Final active we're building
    AstActive* m_cActivep = nullptr;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_درخت>, AstActive*> m_activeMap;

    // METHODS
    تهی addActive(AstActive* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->addBlocksp(nodep);
    }

    // VISITORS
    تهی visit(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = nullptr;
        m_iActivep = nullptr;
        m_fActivep = nullptr;
        m_cActivep = nullptr;
        m_activeMap.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    تهی visit(Ast_Sen_درخت* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    تهی visit(Ast_گره_Stmt*) override {}  // Accelerate
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    template <typename T_SenItemKind>
    AstActive*& getSpecialActive();

عمومی:
    // METHODS
    AstScope* scopep() { بازگشت m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    AstActive* makeActive(File_سطر* ثابت fl, Ast_Sen_درخت* ثابت senTreep) {
        AstActive* ثابت activep = جدید AstActive{fl, "", senTreep};
        activep->sen_درخت_Storep(activep->sentreep());
        addActive(activep);
        بازگشت activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    template <typename T_SenItemKind>
    AstActive* makeSpecialActive(File_سطر* ثابت fl) {
        Ast_Sen_درخت* ثابت senTreep = جدید Ast_Sen_درخت{fl, جدید AstSenItem{fl, T_SenItemKind{}}};
        بازگشت makeActive(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    template <typename T_SenItemKind>
    AstActive* getSpecialActive(File_سطر* fl) {
        AstActive*& cachep = getSpecialActive<T_SenItemKind>();
        اگر (!cachep) cachep = makeSpecialActive<T_SenItemKind>(fl);
        بازگشت cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    AstActive* getActive(File_سطر* fl, Ast_Sen_درخت* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        auto it = m_activeMap.find(*sentreep);
        // If found matching AstActive, return it
        اگر (it != m_activeMap.end()) بازگشت it->second;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_درخت* ثابت newsenp = sentreep->clone_درخت(نادرست);
        AstActive* ثابت activep = جدید AstActive{fl, "sequent", newsenp};
        activep->sen_درخت_Storep(activep->sentreep());
        addActive(activep);
        m_activeMap.emplace(*newsenp, activep);
        بازگشت activep;
    }

    // CONSTRUCTORS
    ActiveNamer() = پیشفرض;
    ~ActiveNamer() override = پیشفرض;
    تهی اصلی(AstScope* nodep) { iterate(nodep); }
};

template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Static>() {
    بازگشت m_sActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Initial>() {
    بازگشت m_iActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Final>() {
    بازگشت m_fActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Combo>() {
    بازگشت m_cActivep;
}

//######################################################################
// Latch checking visitor

رده ActiveLatchCheckVisitor final : عمومی VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ثابت VNUser1InUse m_inuser1;
    // STATE
    LatchDetectGraph m_graph;  // Graph used to detect latches in combo always
    // VISITORS
    تهی visit(AstVarRef* nodep) override {
        ثابت AstVar* ثابت varp = nodep->varp();
        اگر (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->isUsedLoopIdx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_graph.addAssignment(nodep);
        }
    }
    تهی visit(Ast_گره_If* nodep) override {
        اگر (!nodep->isBoundsCheck()) {
            LatchDetectGraphVertex* ثابت parentp = m_graph.currentp();
            LatchDetectGraphVertex* ثابت branchp = m_graph.addPathVertex(parentp, "BRANCH", درست);
            m_graph.addPathVertex(branchp, "IF");
            iterateAndNextConstNull(nodep->thensp());
            m_graph.addPathVertex(branchp, "ELSE");
            iterateAndNextConstNull(nodep->elsesp());
            m_graph.currentp(parentp);
        } وگرنه {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عمومی:
    // CONSTRUCTORS
    ActiveLatchCheckVisitor(Ast_گره* nodep, منطقی expectLatch) {
        m_graph.begin();
        iterateConst(nodep);
        m_graph.latchCheck(nodep, expectLatch);
    }
    ~ActiveLatchCheckVisitor() override = پیشفرض;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

رده ActiveDlyVisitor final : عمومی VNVisitor {
عمومی:
    شمارش CheckType : uint8_t { CT_COMB, CT_FINAL };

خصوصی:
    // MEMBERS
    ثابت CheckType m_check;  // Process type we are checking

    // VISITORS
    تهی visit(AstAssignDly* nodep) override {
        // Issue appropriate warning
        اگر (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } وگرنه {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warnMore()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(جدید AstAssign{
            nodep->fileline(),  //
            nodep->lhsp()->unlinkFrBack(),  //
            nodep->rhsp()->unlinkFrBack(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlinkFrBack() : nullptr});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    ActiveDlyVisitor(Ast_گره* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ActiveDlyVisitor() override = پیشفرض;
};

//######################################################################
// Active class functions

رده ActiveVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ActiveNamer m_namer;  // Tracking of active names
    منطقی m_clockedProcess = نادرست;  // Whether current process is a clocked process
    منطقی m_allChanged = نادرست;  // Whether all SenItem in the SenTree are ET_CHANGED
    منطقی m_walkingBody = نادرست;  // Walking body of a process
    منطقی m_canBeComb = نادرست;  // Whether current clocked process can be turned into a comb process

    // METHODS
    template <typename T>
    تهی moveUnderSpecial(Ast_گره* nodep) {
        AstActive* ثابت wantactivep = m_namer.getSpecialActive<T>(nodep->fileline());
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);
    }

    تهی visitAlways(Ast_گره* nodep, Ast_Sen_درخت* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        اگر (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            بازگشت;
        }

        {
            ثابت VNUser1InUse user1InUse;

            // Walk sensitivity list
            m_clockedProcess = نادرست;
            m_allChanged = درست;
            اگر (oldsentreep) {
                oldsentreep->unlinkFrBack();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            اگر (m_allChanged) {
                ثابت VNUser2InUse user2InUse;
                m_walkingBody = درست;
                m_canBeComb = درست;
                iterateChildrenConst(nodep);
                m_walkingBody = نادرست;
                اگر (m_canBeComb) m_clockedProcess = نادرست;
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
        اگر (!m_clockedProcess) ActiveDlyVisitor{nodep, ActiveDlyVisitor::CT_COMB};

        // Delete sensitivity list
        اگر (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_درخت(), oldsentreep);

        // check combinational processes for latches
        اگر (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ثابت ActiveLatchCheckVisitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    تهی visit_Sen_اقلام(Ast_گره* nodep) {
        اگر (v3Global.opt.timing().isSetTrue()) {
            nodep->foreach([این](AstSenItem* senItemp) { visit(senItemp); });
        }
    }

    ایستا تهی markEventEdges(Ast_Sen_درخت* sentreep) {
        برای (AstSenItem* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), SenItem)) {
            اگر (!senip->sensp()) ادامه;
            اگر (ثابت Ast_گره_DType* ثابت dtypep = senip->sensp()->dtypep()) {
                اگر (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                    اگر (basicp->isEvent()) senip->edgeType(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    تهی visit(AstScope* nodep) override {
        m_namer.اصلی(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    تهی visit(AstActive* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    تهی visit(AstInitialStatic* nodep) override { moveUnderSpecial<AstSenItem::Static>(nodep); }
    تهی visit(AstInitial* nodep) override {
        visit_Sen_اقلام(nodep);
        moveUnderSpecial<AstSenItem::Initial>(nodep);
    }
    تهی visit(AstFinal* nodep) override {
        ثابت ActiveDlyVisitor dlyvisitor{nodep, ActiveDlyVisitor::CT_FINAL};
        moveUnderSpecial<AstSenItem::Final>(nodep);
    }
    تهی visit(AstCoverToggle* nodep) override { moveUnderSpecial<AstSenItem::Combo>(nodep); }
    تهی visit(AstAlways* nodep) override {
        اگر (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            بازگشت;
        }
        اگر (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            moveUnderSpecial<AstSenItem::Combo>(nodep);
            بازگشت;
        }
        visit_Sen_اقلام(nodep);
        visitAlways(nodep, nodep->sentreep(), nodep->keyword());
    }
    تهی visit(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ثابت activep = m_namer.makeSpecialActive<AstSenItem::Combo>(nodep->fileline());
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    تهی visit(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_درخت* ثابت sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ثابت activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    تهی visit(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_درخت* ثابت sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ثابت activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    تهی visit(AstCFunc* nodep) override { visit_Sen_اقلام(nodep); }
    تهی visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        اگر (!nodep->sensp()) بازگشت;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = درست;
        اگر (nodep->edgeType() != VEdgeType::ET_CHANGED) m_allChanged = نادرست;

        اگر (ثابت Ast_گره_DType* ثابت dtypep = nodep->sensp()->dtypep()) {
            اگر (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                اگر (basicp->isEvent()) nodep->edgeType(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ثابت AstVarRef* refp) { refp->varScopep()->user1(درست); });
    }

    تهی visit(AstVarRef* nodep) override {
        AstVarScope* ثابت vscp = nodep->varScopep();
        اگر (nodep->access().isWriteOnly()) {
            vscp->user2(درست);
        } وگرنه {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            ثابت AstVar* ثابت varp = vscp->varp();
            اگر (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = نادرست;
            }
        }
    }
    تهی visit(AstAssignDly* nodep) override {
        m_canBeComb = نادرست;
        اگر (nodep->isTimingControl()) m_clockedProcess = درست;
        iterateChildrenConst(nodep);
    }
    تهی visit(AstFireEvent* nodep) override {
        m_canBeComb = نادرست;
        iterateChildrenConst(nodep);
    }
    تهی visit(AstAssignForce* nodep) override {
        m_canBeComb = نادرست;
        iterateChildrenConst(nodep);
    }
    تهی visit(AstRelease* nodep) override {
        m_canBeComb = نادرست;
        iterateChildrenConst(nodep);
    }
    تهی visit(AstFork* nodep) override {
        اگر (nodep->isTimingControl()) {
            m_canBeComb = نادرست;
            m_clockedProcess = درست;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    تهی visit(AstVar*) override {}  // Accelerate
    تهی visit(AstVarScope*) override {}  // Accelerate
    تهی visit(Ast_گره* nodep) override {
        اگر (!v3Global.opt.timing().isSetTrue() && m_walkingBody && !m_canBeComb) {
            بازگشت;  // Accelerate
        }
        اگر (!nodep->isPure()) m_canBeComb = نادرست;
        اگر (nodep->isTimingControl()) {
            m_canBeComb = نادرست;
            m_clockedProcess = درست;
            بازگشت;
        }
        iterateChildren(nodep);
    }

عمومی:
    // CONSTRUCTORS
    explicit ActiveVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ActiveVisitor() override = پیشفرض;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

رده CovergroupCollectVisitor final : عمومی VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = nullptr;  // Current covergroup class context, or nullptr

    // VISITORS
    تهی visit(AstClass* nodep) override {
        اگر (!nodep->isCovergroup()) بازگشت;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    تهی visit(AstScope* nodep) override { iterateChildren(nodep); }

    تهی visit(AstCFunc* nodep) override {
        اگر (!m_classp) بازگشت;
        اگر (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    تهی visit(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlinkFrBack());
        nodep->unlinkFrBack();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = پیشفرض;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

رده CovergroupEventBindVisitor final : عمومی VNVisitor {
    AstVarScope* ثابت m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_set<ثابت AstVar*> m_memberps;  // Non-static variables in the covergroup class

    تهی visit(AstVarRef* nodep) override {
        اگر (!m_memberps.شمار(nodep->varp())) بازگشت;
        File_سطر* ثابت fl = nodep->fileline();
        AstMemberSel* ثابت selp
            = جدید AstMemberSel{fl, جدید AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    CovergroupEventBindVisitor(Ast_Sen_درخت* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* ثابت, AstVar* ثابت varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = پیشفرض;
};

رده CovergroupInjectVisitor final : عمومی VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    ActiveNamer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    تهی visit(AstScope* nodep) override {
        m_namer.اصلی(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    تهی visit(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ثابت varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ثابت Ast_گره_DType* ثابت dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
        اگر (!classRefp) بازگشت;

        AstClass* ثابت classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_درخت* ثابت eventp = VN_CAST(classp->user2p(), Sen_درخت);
        اگر (!eventp) بازگشت;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ثابت sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_سطر* ثابت fl = nodep->fileline();
        AstVarRef* ثابت varrefp = جدید AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* ثابت cmethodCallp
            = جدید AstCMethodCall{fl, varrefp, sampleCFuncp, nullptr};

        cmethodCallp->dtypeSetVoid();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_درخت* senTreep = eventp->clone_درخت(نادرست);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        AstActive* ثابت activep = m_namer.getActive(fl, senTreep);
        VL_DO_DANGLING(افزودن_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->addStmtsp(
            جدید AstAlways{fl, VAlwaysKwd::ALWAYS_FF, nullptr, cmethodCallp->makeStmt()});
    }

    تهی visit(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        اگر (Ast_Sen_درخت* ثابت eventp = VN_CAST(nodep->user2p(), Sen_درخت)) {
            VL_DO_DANGLING(افزودن_Deletep(eventp), eventp);
        }
    }

    تهی visit(AstActive*) override {}  // Don't iterate into actives
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = پیشفرض;
};

//######################################################################
// Active class functions

تهی V3Active::activeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ActiveVisitor{nodep}; }  // Destruct before checking
    اگر (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ثابت VNUser1InUse user1InUse;
        ثابت VNUser2InUse user2InUse;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_درخت("active", 0, dump_درخت_Either_Level() >= 3);
}
