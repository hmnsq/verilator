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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Active.h"

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"

#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

ଶ୍ରେଣୀ LatchDetectGraphVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(LatchDetectGraphVertex, V3GraphVertex)
ସର୍ବସାଧାରଣ:
    ଗଣନା VertexType : uint8_t { VT_BLOCK, VT_BRANCH, VT_OUTPUT };

ବ୍ୟକ୍ତିଗତ:
    ଅଚଳ string m_name;  // Only used for .dot file generation
    ଅଚଳ VertexType m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ଅଚଳ VL_MT_SAFE {  //   "
        ବାଛ (m_type) {
        ସ୍ଥିତି VT_BLOCK: ଫେରନ୍ତୁ "(||)";  // basic block node
        ସ୍ଥିତି VT_BRANCH: ଫେରନ୍ତୁ "(&&)";  // if/else branch mode
        ସ୍ଥିତି VT_OUTPUT: ଫେରନ୍ତୁ "(out)";  // var assignment
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ "??";  // unknown
        }
    }

ସର୍ବସାଧାରଣ:
    LatchDetectGraphVertex(V3Graph* graphp, ଅଚଳ string& name, VertexType type = VT_BLOCK)
        : V3GraphVertex{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name + " " + typestr(); }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ user() ? "green" : "black"; }
    ଆଭାସୀ ପୂର୍ଣ୍ଣସଂଖ୍ୟା type() ଅଚଳ { ଫେରନ୍ତୁ m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

ଶ୍ରେଣୀ LatchDetectGraph final : ସର୍ବସାଧାରଣ V3Graph {
ସୁରକ୍ଷିତ:
    LatchDetectGraphVertex* m_curVertexp = nullptr;  // Current latch detection graph vertex
    std::vector<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    ସ୍ଥିର LatchDetectGraphVertex* castVertexp(ଶୂନ୍ୟ* vertexp) {
        ଫେରନ୍ତୁ reinterpret_cast<LatchDetectGraphVertex*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    ତାର୍କିକ latchCheckInternal(LatchDetectGraphVertex* vertexp) {
        ତାର୍କିକ result = ମିଥ୍ୟା;
        ବାଛ (vertexp->type()) {
        ସ୍ଥିତି LatchDetectGraphVertex::VT_OUTPUT:  // Base case
            result = vertexp->user();
            ଭାଙ୍ଗ;
        ସ୍ଥିତି LatchDetectGraphVertex::VT_BLOCK:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            ପାଇଁ (V3GraphEdge& edge : vertexp->outEdges()) {
                ଯଦି (latchCheckInternal(castVertexp(edge.ଶୀର୍ଷ()))) {
                    result = ସତ୍ୟ;
                    ଭାଙ୍ଗ;
                }
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି LatchDetectGraphVertex::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            LatchDetectGraphVertex* ଅଚଳ ifp = castVertexp(vertexp->outEdges().frontp()->ଶୀର୍ଷ());
            LatchDetectGraphVertex* ଅଚଳ elsp = castVertexp(vertexp->outEdges().backp()->ଶୀର୍ଷ());
            result = latchCheckInternal(ifp) && latchCheckInternal(elsp);
            ଭାଙ୍ଗ;
        }
        vertexp->user(result);
        ଫେରନ୍ତୁ result;
    }

ସର୍ବସାଧାରଣ:
    LatchDetectGraph() { clear(); }
    ~LatchDetectGraph() override { clear(); }
    // ACCESSORS
    LatchDetectGraphVertex* currentp() { ଫେରନ୍ତୁ m_curVertexp; }
    ଶୂନ୍ୟ currentp(LatchDetectGraphVertex* vertex) { m_curVertexp = vertex; }
    // METHODS
    ଶୂନ୍ୟ begin() {
        // Start a new if/else tracking graph
        m_curVertexp = ନୂତନ LatchDetectGraphVertex{ଏହି, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    ଶୂନ୍ୟ clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3Graph::clear();
        m_curVertexp = nullptr;
    }
    // Add a new control path and connect it to its parent
    LatchDetectGraphVertex* addPathVertex(LatchDetectGraphVertex* parent, ଅଚଳ string& name,
                                          ତାର୍କିକ branch = ମିଥ୍ୟା) {
        m_curVertexp = ନୂତନ LatchDetectGraphVertex{ଏହି, name,
                                                  branch ? LatchDetectGraphVertex::VT_BRANCH
                                                         : LatchDetectGraphVertex::VT_BLOCK};
        ନୂତନ V3GraphEdge{ଏହି, parent, m_curVertexp, 1};
        ଫେରନ୍ତୁ m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    LatchDetectGraphVertex* addOutputVertex(AstVarRef* nodep) {
        LatchDetectGraphVertex* ଅଚଳ outVertexp
            = ନୂତନ LatchDetectGraphVertex{ଏହି, nodep->name(), LatchDetectGraphVertex::VT_OUTPUT};
        nodep->varp()->user1p(outVertexp);
        m_outputs.ଠେଲ_back(nodep);
        ଫେରନ୍ତୁ outVertexp;
    }
    // Connect an output assignment to its parent control block
    ଶୂନ୍ୟ addAssignment(AstVarRef* nodep) {
        LatchDetectGraphVertex* outVertexp;
        ଯଦି (!nodep->varp()->user1p()) {  // Not seen this output before
            outVertexp = addOutputVertex(nodep);
        } ଅନ୍ୟଥା {
            outVertexp = castVertexp(nodep->varp()->user1p());
        }
        ନୂତନ V3GraphEdge{ଏହି, m_curVertexp, outVertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    ଶୂନ୍ୟ latchCheck(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ latch_expected) {
        ତାର୍କିକ latch_detected = ମିଥ୍ୟା;
        ପାଇଁ (ଅଚଳ AstVarRef* ଅଚଳ vrp : m_outputs) {
            LatchDetectGraphVertex* ଅଚଳ vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(ସତ୍ୟ);  // Identify the output vertex we are checking paths _to_
            ଯଦି (!latchCheckInternal(castVertexp(vertices().frontp()))) latch_detected = ସତ୍ୟ;
            ଯଦି (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warnMore()
                        << "... Suggest use of always_latch for intentional latches");
                ଯଦି (dumpGraphLevel() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(ମିଥ୍ୟା);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        ଯଦି (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

ଶ୍ରେଣୀ ActiveNamer final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope to add statement to
    AstActive* m_sActivep = nullptr;  // For current scope, the Static active we're building
    AstActive* m_iActivep = nullptr;  // For current scope, the Initial active we're building
    AstActive* m_fActivep = nullptr;  // For current scope, the Final active we're building
    AstActive* m_cActivep = nullptr;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_ବୃକ୍ଷ>, AstActive*> m_activeMap;

    // METHODS
    ଶୂନ୍ୟ addActive(AstActive* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->addBlocksp(nodep);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = nullptr;
        m_iActivep = nullptr;
        m_fActivep = nullptr;
        m_cActivep = nullptr;
        m_activeMap.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    ଶୂନ୍ୟ visit(Ast_Sen_ବୃକ୍ଷ* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    template <typename T_SenItemKind>
    AstActive*& getSpecialActive();

ସର୍ବସାଧାରଣ:
    // METHODS
    AstScope* scopep() { ଫେରନ୍ତୁ m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    AstActive* makeActive(File_ଧାଡ଼ି* ଅଚଳ fl, Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep) {
        AstActive* ଅଚଳ activep = ନୂତନ AstActive{fl, "", senTreep};
        activep->sen_ବୃକ୍ଷ_Storep(activep->sentreep());
        addActive(activep);
        ଫେରନ୍ତୁ activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    template <typename T_SenItemKind>
    AstActive* makeSpecialActive(File_ଧାଡ଼ି* ଅଚଳ fl) {
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = ନୂତନ Ast_Sen_ବୃକ୍ଷ{fl, ନୂତନ AstSenItem{fl, T_SenItemKind{}}};
        ଫେରନ୍ତୁ makeActive(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    template <typename T_SenItemKind>
    AstActive* getSpecialActive(File_ଧାଡ଼ି* fl) {
        AstActive*& cachep = getSpecialActive<T_SenItemKind>();
        ଯଦି (!cachep) cachep = makeSpecialActive<T_SenItemKind>(fl);
        ଫେରନ୍ତୁ cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    AstActive* getActive(File_ଧାଡ଼ି* fl, Ast_Sen_ବୃକ୍ଷ* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        auto it = m_activeMap.find(*sentreep);
        // If found matching AstActive, return it
        ଯଦି (it != m_activeMap.end()) ଫେରନ୍ତୁ it->second;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ newsenp = sentreep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        AstActive* ଅଚଳ activep = ନୂତନ AstActive{fl, "sequent", newsenp};
        activep->sen_ବୃକ୍ଷ_Storep(activep->sentreep());
        addActive(activep);
        m_activeMap.emplace(*newsenp, activep);
        ଫେରନ୍ତୁ activep;
    }

    // CONSTRUCTORS
    ActiveNamer() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~ActiveNamer() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଶୂନ୍ୟ ମୁଖ୍ୟ(AstScope* nodep) { iterate(nodep); }
};

template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Static>() {
    ଫେରନ୍ତୁ m_sActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Initial>() {
    ଫେରନ୍ତୁ m_iActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Final>() {
    ଫେରନ୍ତୁ m_fActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Combo>() {
    ଫେରନ୍ତୁ m_cActivep;
}

//######################################################################
// Latch checking visitor

ଶ୍ରେଣୀ ActiveLatchCheckVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ଅଚଳ VNUser1InUse m_inuser1;
    // STATE
    LatchDetectGraph m_graph;  // Graph used to detect latches in combo always
    // VISITORS
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଅଚଳ AstVar* ଅଚଳ varp = nodep->varp();
        ଯଦି (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->isUsedLoopIdx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_graph.addAssignment(nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        ଯଦି (!nodep->isBoundsCheck()) {
            LatchDetectGraphVertex* ଅଚଳ parentp = m_graph.currentp();
            LatchDetectGraphVertex* ଅଚଳ branchp = m_graph.addPathVertex(parentp, "BRANCH", ସତ୍ୟ);
            m_graph.addPathVertex(branchp, "IF");
            iterateAndNextConstNull(nodep->thensp());
            m_graph.addPathVertex(branchp, "ELSE");
            iterateAndNextConstNull(nodep->elsesp());
            m_graph.currentp(parentp);
        } ଅନ୍ୟଥା {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    ActiveLatchCheckVisitor(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ expectLatch) {
        m_graph.begin();
        iterateConst(nodep);
        m_graph.latchCheck(nodep, expectLatch);
    }
    ~ActiveLatchCheckVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

ଶ୍ରେଣୀ ActiveDlyVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
ସର୍ବସାଧାରଣ:
    ଗଣନା CheckType : uint8_t { CT_COMB, CT_FINAL };

ବ୍ୟକ୍ତିଗତ:
    // MEMBERS
    ଅଚଳ CheckType m_check;  // Process type we are checking

    // VISITORS
    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override {
        // Issue appropriate warning
        ଯଦି (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } ଅନ୍ୟଥା {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warnMore()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(ନୂତନ AstAssign{
            nodep->fileline(),  //
            nodep->lhsp()->unlinkFrBack(),  //
            nodep->rhsp()->unlinkFrBack(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlinkFrBack() : nullptr});
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }

    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    ActiveDlyVisitor(Ast_ଗଣ୍ଠି* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ActiveDlyVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Active class functions

ଶ୍ରେଣୀ ActiveVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ActiveNamer m_namer;  // Tracking of active names
    ତାର୍କିକ m_clockedProcess = ମିଥ୍ୟା;  // Whether current process is a clocked process
    ତାର୍କିକ m_allChanged = ମିଥ୍ୟା;  // Whether all SenItem in the SenTree are ET_CHANGED
    ତାର୍କିକ m_walkingBody = ମିଥ୍ୟା;  // Walking body of a process
    ତାର୍କିକ m_canBeComb = ମିଥ୍ୟା;  // Whether current clocked process can be turned into a comb process

    // METHODS
    template <typename T>
    ଶୂନ୍ୟ moveUnderSpecial(Ast_ଗଣ୍ଠି* nodep) {
        AstActive* ଅଚଳ wantactivep = m_namer.getSpecialActive<T>(nodep->fileline());
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);
    }

    ଶୂନ୍ୟ visitAlways(Ast_ଗଣ୍ଠି* nodep, Ast_Sen_ବୃକ୍ଷ* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        ଯଦି (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }

        {
            ଅଚଳ VNUser1InUse user1InUse;

            // Walk sensitivity list
            m_clockedProcess = ମିଥ୍ୟା;
            m_allChanged = ସତ୍ୟ;
            ଯଦି (oldsentreep) {
                oldsentreep->unlinkFrBack();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            ଯଦି (m_allChanged) {
                ଅଚଳ VNUser2InUse user2InUse;
                m_walkingBody = ସତ୍ୟ;
                m_canBeComb = ସତ୍ୟ;
                iterateChildrenConst(nodep);
                m_walkingBody = ମିଥ୍ୟା;
                ଯଦି (m_canBeComb) m_clockedProcess = ମିଥ୍ୟା;
            }
        }

        AstActive* ଅଚଳ wantactivep
            = !m_clockedProcess ? m_namer.getSpecialActive<AstSenItem::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.getActive(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.getSpecialActive<AstSenItem::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);

        // Warn and convert any delayed assignments
        ଯଦି (!m_clockedProcess) ActiveDlyVisitor{nodep, ActiveDlyVisitor::CT_COMB};

        // Delete sensitivity list
        ଯଦି (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_ବୃକ୍ଷ(), oldsentreep);

        // check combinational processes for latches
        ଯଦି (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ଅଚଳ ActiveLatchCheckVisitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    ଶୂନ୍ୟ visit_Sen_ବସ୍ତୁଗୁଡ଼ିକ(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (v3Global.opt.timing().isSetTrue()) {
            nodep->foreach([ଏହି](AstSenItem* senItemp) { visit(senItemp); });
        }
    }

    ସ୍ଥିର ଶୂନ୍ୟ markEventEdges(Ast_Sen_ବୃକ୍ଷ* sentreep) {
        ପାଇଁ (AstSenItem* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), SenItem)) {
            ଯଦି (!senip->sensp()) ଜାରି;
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = senip->sensp()->dtypep()) {
                ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ basicp = dtypep->basicp()) {
                    ଯଦି (basicp->isEvent()) senip->edgeType(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        m_namer.ମୁଖ୍ୟ(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstActive* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    ଶୂନ୍ୟ visit(AstInitialStatic* nodep) override { moveUnderSpecial<AstSenItem::Static>(nodep); }
    ଶୂନ୍ୟ visit(AstInitial* nodep) override {
        visit_Sen_ବସ୍ତୁଗୁଡ଼ିକ(nodep);
        moveUnderSpecial<AstSenItem::Initial>(nodep);
    }
    ଶୂନ୍ୟ visit(AstFinal* nodep) override {
        ଅଚଳ ActiveDlyVisitor dlyvisitor{nodep, ActiveDlyVisitor::CT_FINAL};
        moveUnderSpecial<AstSenItem::Final>(nodep);
    }
    ଶୂନ୍ୟ visit(AstCoverToggle* nodep) override { moveUnderSpecial<AstSenItem::Combo>(nodep); }
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        ଯଦି (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            moveUnderSpecial<AstSenItem::Combo>(nodep);
            ଫେରନ୍ତୁ;
        }
        visit_Sen_ବସ୍ତୁଗୁଡ଼ିକ(nodep);
        visitAlways(nodep, nodep->sentreep(), nodep->keyword());
    }
    ଶୂନ୍ୟ visit(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ଅଚଳ activep = m_namer.makeSpecialActive<AstSenItem::Combo>(nodep->fileline());
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    ଶୂନ୍ୟ visit(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ଅଚଳ activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    ଶୂନ୍ୟ visit(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* ଅଚଳ activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override { visit_Sen_ବସ୍ତୁଗୁଡ଼ିକ(nodep); }
    ଶୂନ୍ୟ visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        ଯଦି (!nodep->sensp()) ଫେରନ୍ତୁ;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = ସତ୍ୟ;
        ଯଦି (nodep->edgeType() != VEdgeType::ET_CHANGED) m_allChanged = ମିଥ୍ୟା;

        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->sensp()->dtypep()) {
            ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ basicp = dtypep->basicp()) {
                ଯଦି (basicp->isEvent()) nodep->edgeType(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ଅଚଳ AstVarRef* refp) { refp->varScopep()->user1(ସତ୍ୟ); });
    }

    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        AstVarScope* ଅଚଳ vscp = nodep->varScopep();
        ଯଦି (nodep->access().isWriteOnly()) {
            vscp->user2(ସତ୍ୟ);
        } ଅନ୍ୟଥା {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            ଅଚଳ AstVar* ଅଚଳ varp = vscp->varp();
            ଯଦି (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = ମିଥ୍ୟା;
            }
        }
    }
    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override {
        m_canBeComb = ମିଥ୍ୟା;
        ଯଦି (nodep->isTimingControl()) m_clockedProcess = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstFireEvent* nodep) override {
        m_canBeComb = ମିଥ୍ୟା;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstAssignForce* nodep) override {
        m_canBeComb = ମିଥ୍ୟା;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstRelease* nodep) override {
        m_canBeComb = ମିଥ୍ୟା;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        ଯଦି (nodep->isTimingControl()) {
            m_canBeComb = ମିଥ୍ୟା;
            m_clockedProcess = ସତ୍ୟ;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    ଶୂନ୍ୟ visit(AstVar*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(AstVarScope*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (!v3Global.opt.timing().isSetTrue() && m_walkingBody && !m_canBeComb) {
            ଫେରନ୍ତୁ;  // Accelerate
        }
        ଯଦି (!nodep->isPure()) m_canBeComb = ମିଥ୍ୟା;
        ଯଦି (nodep->isTimingControl()) {
            m_canBeComb = ମିଥ୍ୟା;
            m_clockedProcess = ସତ୍ୟ;
            ଫେରନ୍ତୁ;
        }
        iterateChildren(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit ActiveVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ActiveVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

ଶ୍ରେଣୀ CovergroupCollectVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = nullptr;  // Current covergroup class context, or nullptr

    // VISITORS
    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        ଯଦି (!nodep->isCovergroup()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstScope* nodep) override { iterateChildren(nodep); }

    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        ଯଦି (!m_classp) ଫେରନ୍ତୁ;
        ଯଦି (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    ଶୂନ୍ୟ visit(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlinkFrBack());
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

ଶ୍ରେଣୀ CovergroupEventBindVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    AstVarScope* ଅଚଳ m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_set<ଅଚଳ AstVar*> m_memberps;  // Non-static variables in the covergroup class

    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (!m_memberps.ଗଣନା_2(nodep->varp())) ଫେରନ୍ତୁ;
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        AstMemberSel* ଅଚଳ selp
            = ନୂତନ AstMemberSel{fl, ନୂତନ AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    CovergroupEventBindVisitor(Ast_Sen_ବୃକ୍ଷ* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* ଅଚଳ, AstVar* ଅଚଳ varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

ଶ୍ରେଣୀ CovergroupInjectVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    ActiveNamer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        m_namer.ମୁଖ୍ୟ(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ଅଚଳ varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ଅଚଳ AstClassRefDType* ଅଚଳ classRefp = VN_CAST(dtypep, ClassRefDType);
        ଯଦି (!classRefp) ଫେରନ୍ତୁ;

        AstClass* ଅଚଳ classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ eventp = VN_CAST(classp->user2p(), Sen_ବୃକ୍ଷ);
        ଯଦି (!eventp) ଫେରନ୍ତୁ;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ଅଚଳ sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        AstVarRef* ଅଚଳ varrefp = ନୂତନ AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* ଅଚଳ cmethodCallp
            = ନୂତନ AstCMethodCall{fl, varrefp, sampleCFuncp, nullptr};

        cmethodCallp->dtypeSetVoid();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_ବୃକ୍ଷ* senTreep = eventp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        AstActive* ଅଚଳ activep = m_namer.getActive(fl, senTreep);
        VL_DO_DANGLING(ଠେଲ_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->addStmtsp(
            ନୂତନ AstAlways{fl, VAlwaysKwd::ALWAYS_FF, nullptr, cmethodCallp->makeStmt()});
    }

    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        ଯଦି (Ast_Sen_ବୃକ୍ଷ* ଅଚଳ eventp = VN_CAST(nodep->user2p(), Sen_ବୃକ୍ଷ)) {
            VL_DO_DANGLING(ଠେଲ_Deletep(eventp), eventp);
        }
    }

    ଶୂନ୍ୟ visit(AstActive*) override {}  // Don't iterate into actives
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Active class functions

ଶୂନ୍ୟ V3Active::activeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ActiveVisitor{nodep}; }  // Destruct before checking
    ଯଦି (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ଅଚଳ VNUser1InUse user1InUse;
        ଅଚଳ VNUser2InUse user2InUse;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("active", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
