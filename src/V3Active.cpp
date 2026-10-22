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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Active.h"

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Graph.h"

#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

വർഗ്ഗം LatchDetectGraphVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(LatchDetectGraphVertex, V3GraphVertex)
പൊതു:
    എണ്ണൽ VertexType : uint8_t { VT_BLOCK, VT_BRANCH, VT_OUTPUT };

സ്വകാര്യ:
    സ്ഥിരം string m_name;  // Only used for .dot file generation
    സ്ഥിരം VertexType m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() സ്ഥിരം VL_MT_SAFE {  //   "
        തിരഞ്ഞെടുപ്പ് (m_type) {
        അവസ്ഥ VT_BLOCK: തിരികെ "(||)";  // basic block node
        അവസ്ഥ VT_BRANCH: തിരികെ "(&&)";  // if/else branch mode
        അവസ്ഥ VT_OUTPUT: തിരികെ "(out)";  // var assignment
        സ്ഥിരസ്ഥിതി: തിരികെ "??";  // unknown
        }
    }

പൊതു:
    LatchDetectGraphVertex(V3Graph* graphp, സ്ഥിരം string& name, VertexType type = VT_BLOCK)
        : V3GraphVertex{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name + " " + typestr(); }
    string dotColor() സ്ഥിരം override { തിരികെ user() ? "green" : "black"; }
    മായാ പൂർണ്ണസംഖ്യ type() സ്ഥിരം { തിരികെ m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

വർഗ്ഗം LatchDetectGraph final : പൊതു V3Graph {
സംരക്ഷിത:
    LatchDetectGraphVertex* m_curVertexp = nullptr;  // Current latch detection graph vertex
    std::vector<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    സ്ഥിതം LatchDetectGraphVertex* castVertexp(ശൂന്യം* vertexp) {
        തിരികെ reinterpret_cast<LatchDetectGraphVertex*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    തർക്കം latchCheckInternal(LatchDetectGraphVertex* vertexp) {
        തർക്കം result = അസത്യം;
        തിരഞ്ഞെടുപ്പ് (vertexp->type()) {
        അവസ്ഥ LatchDetectGraphVertex::VT_OUTPUT:  // Base case
            result = vertexp->user();
            നിർത്തുക;
        അവസ്ഥ LatchDetectGraphVertex::VT_BLOCK:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            വേണ്ടി (V3GraphEdge& edge : vertexp->outEdges()) {
                എങ്കിൽ (latchCheckInternal(castVertexp(edge.മുകൾ()))) {
                    result = സത്യം;
                    നിർത്തുക;
                }
            }
            നിർത്തുക;
        അവസ്ഥ LatchDetectGraphVertex::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            LatchDetectGraphVertex* സ്ഥിരം ifp = castVertexp(vertexp->outEdges().frontp()->മുകൾ());
            LatchDetectGraphVertex* സ്ഥിരം elsp = castVertexp(vertexp->outEdges().backp()->മുകൾ());
            result = latchCheckInternal(ifp) && latchCheckInternal(elsp);
            നിർത്തുക;
        }
        vertexp->user(result);
        തിരികെ result;
    }

പൊതു:
    LatchDetectGraph() { clear(); }
    ~LatchDetectGraph() override { clear(); }
    // ACCESSORS
    LatchDetectGraphVertex* currentp() { തിരികെ m_curVertexp; }
    ശൂന്യം currentp(LatchDetectGraphVertex* vertex) { m_curVertexp = vertex; }
    // METHODS
    ശൂന്യം begin() {
        // Start a new if/else tracking graph
        m_curVertexp = പുതിയ LatchDetectGraphVertex{ഇത്, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    ശൂന്യം clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3Graph::clear();
        m_curVertexp = nullptr;
    }
    // Add a new control path and connect it to its parent
    LatchDetectGraphVertex* addPathVertex(LatchDetectGraphVertex* parent, സ്ഥിരം string& name,
                                          തർക്കം branch = അസത്യം) {
        m_curVertexp = പുതിയ LatchDetectGraphVertex{ഇത്, name,
                                                  branch ? LatchDetectGraphVertex::VT_BRANCH
                                                         : LatchDetectGraphVertex::VT_BLOCK};
        പുതിയ V3GraphEdge{ഇത്, parent, m_curVertexp, 1};
        തിരികെ m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    LatchDetectGraphVertex* addOutputVertex(AstVarRef* nodep) {
        LatchDetectGraphVertex* സ്ഥിരം outVertexp
            = പുതിയ LatchDetectGraphVertex{ഇത്, nodep->name(), LatchDetectGraphVertex::VT_OUTPUT};
        nodep->varp()->user1p(outVertexp);
        m_outputs.തള്ളുക_back(nodep);
        തിരികെ outVertexp;
    }
    // Connect an output assignment to its parent control block
    ശൂന്യം addAssignment(AstVarRef* nodep) {
        LatchDetectGraphVertex* outVertexp;
        എങ്കിൽ (!nodep->varp()->user1p()) {  // Not seen this output before
            outVertexp = addOutputVertex(nodep);
        } അല്ലെങ്കിൽ {
            outVertexp = castVertexp(nodep->varp()->user1p());
        }
        പുതിയ V3GraphEdge{ഇത്, m_curVertexp, outVertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    ശൂന്യം latchCheck(Ast_കണ്ണി* nodep, തർക്കം latch_expected) {
        തർക്കം latch_detected = അസത്യം;
        വേണ്ടി (സ്ഥിരം AstVarRef* സ്ഥിരം vrp : m_outputs) {
            LatchDetectGraphVertex* സ്ഥിരം vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(സത്യം);  // Identify the output vertex we are checking paths _to_
            എങ്കിൽ (!latchCheckInternal(castVertexp(vertices().frontp()))) latch_detected = സത്യം;
            എങ്കിൽ (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warnMore()
                        << "... Suggest use of always_latch for intentional latches");
                എങ്കിൽ (dumpGraphLevel() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(അസത്യം);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        എങ്കിൽ (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

വർഗ്ഗം ActiveNamer final : പൊതു VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope to add statement to
    AstActive* m_sActivep = nullptr;  // For current scope, the Static active we're building
    AstActive* m_iActivep = nullptr;  // For current scope, the Initial active we're building
    AstActive* m_fActivep = nullptr;  // For current scope, the Final active we're building
    AstActive* m_cActivep = nullptr;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_വൃക്ഷം>, AstActive*> m_activeMap;

    // METHODS
    ശൂന്യം addActive(AstActive* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->addBlocksp(nodep);
    }

    // VISITORS
    ശൂന്യം visit(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = nullptr;
        m_iActivep = nullptr;
        m_fActivep = nullptr;
        m_cActivep = nullptr;
        m_activeMap.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    ശൂന്യം visit(Ast_Sen_വൃക്ഷം* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി_Stmt*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    template <typename T_SenItemKind>
    AstActive*& getSpecialActive();

പൊതു:
    // METHODS
    AstScope* scopep() { തിരികെ m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    AstActive* makeActive(File_വരി* സ്ഥിരം fl, Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep) {
        AstActive* സ്ഥിരം activep = പുതിയ AstActive{fl, "", senTreep};
        activep->sen_വൃക്ഷം_Storep(activep->sentreep());
        addActive(activep);
        തിരികെ activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    template <typename T_SenItemKind>
    AstActive* makeSpecialActive(File_വരി* സ്ഥിരം fl) {
        Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = പുതിയ Ast_Sen_വൃക്ഷം{fl, പുതിയ AstSenItem{fl, T_SenItemKind{}}};
        തിരികെ makeActive(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    template <typename T_SenItemKind>
    AstActive* getSpecialActive(File_വരി* fl) {
        AstActive*& cachep = getSpecialActive<T_SenItemKind>();
        എങ്കിൽ (!cachep) cachep = makeSpecialActive<T_SenItemKind>(fl);
        തിരികെ cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    AstActive* getActive(File_വരി* fl, Ast_Sen_വൃക്ഷം* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        auto it = m_activeMap.find(*sentreep);
        // If found matching AstActive, return it
        എങ്കിൽ (it != m_activeMap.end()) തിരികെ it->second;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_വൃക്ഷം* സ്ഥിരം newsenp = sentreep->clone_വൃക്ഷം(അസത്യം);
        AstActive* സ്ഥിരം activep = പുതിയ AstActive{fl, "sequent", newsenp};
        activep->sen_വൃക്ഷം_Storep(activep->sentreep());
        addActive(activep);
        m_activeMap.emplace(*newsenp, activep);
        തിരികെ activep;
    }

    // CONSTRUCTORS
    ActiveNamer() = സ്ഥിരസ്ഥിതി;
    ~ActiveNamer() override = സ്ഥിരസ്ഥിതി;
    ശൂന്യം പ്രധാന(AstScope* nodep) { iterate(nodep); }
};

template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Static>() {
    തിരികെ m_sActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Initial>() {
    തിരികെ m_iActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Final>() {
    തിരികെ m_fActivep;
}
template <>
AstActive*& ActiveNamer::getSpecialActive<AstSenItem::Combo>() {
    തിരികെ m_cActivep;
}

//######################################################################
// Latch checking visitor

വർഗ്ഗം ActiveLatchCheckVisitor final : പൊതു VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    സ്ഥിരം VNUser1InUse m_inuser1;
    // STATE
    LatchDetectGraph m_graph;  // Graph used to detect latches in combo always
    // VISITORS
    ശൂന്യം visit(AstVarRef* nodep) override {
        സ്ഥിരം AstVar* സ്ഥിരം varp = nodep->varp();
        എങ്കിൽ (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->isUsedLoopIdx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_graph.addAssignment(nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        എങ്കിൽ (!nodep->isBoundsCheck()) {
            LatchDetectGraphVertex* സ്ഥിരം parentp = m_graph.currentp();
            LatchDetectGraphVertex* സ്ഥിരം branchp = m_graph.addPathVertex(parentp, "BRANCH", സത്യം);
            m_graph.addPathVertex(branchp, "IF");
            iterateAndNextConstNull(nodep->thensp());
            m_graph.addPathVertex(branchp, "ELSE");
            iterateAndNextConstNull(nodep->elsesp());
            m_graph.currentp(parentp);
        } അല്ലെങ്കിൽ {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    // CONSTRUCTORS
    ActiveLatchCheckVisitor(Ast_കണ്ണി* nodep, തർക്കം expectLatch) {
        m_graph.begin();
        iterateConst(nodep);
        m_graph.latchCheck(nodep, expectLatch);
    }
    ~ActiveLatchCheckVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

വർഗ്ഗം ActiveDlyVisitor final : പൊതു VNVisitor {
പൊതു:
    എണ്ണൽ CheckType : uint8_t { CT_COMB, CT_FINAL };

സ്വകാര്യ:
    // MEMBERS
    സ്ഥിരം CheckType m_check;  // Process type we are checking

    // VISITORS
    ശൂന്യം visit(AstAssignDly* nodep) override {
        // Issue appropriate warning
        എങ്കിൽ (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } അല്ലെങ്കിൽ {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warnMore()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(പുതിയ AstAssign{
            nodep->fileline(),  //
            nodep->lhsp()->unlinkFrBack(),  //
            nodep->rhsp()->unlinkFrBack(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlinkFrBack() : nullptr});
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }

    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    ActiveDlyVisitor(Ast_കണ്ണി* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ActiveDlyVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Active class functions

വർഗ്ഗം ActiveVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ActiveNamer m_namer;  // Tracking of active names
    തർക്കം m_clockedProcess = അസത്യം;  // Whether current process is a clocked process
    തർക്കം m_allChanged = അസത്യം;  // Whether all SenItem in the SenTree are ET_CHANGED
    തർക്കം m_walkingBody = അസത്യം;  // Walking body of a process
    തർക്കം m_canBeComb = അസത്യം;  // Whether current clocked process can be turned into a comb process

    // METHODS
    template <typename T>
    ശൂന്യം moveUnderSpecial(Ast_കണ്ണി* nodep) {
        AstActive* സ്ഥിരം wantactivep = m_namer.getSpecialActive<T>(nodep->fileline());
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);
    }

    ശൂന്യം visitAlways(Ast_കണ്ണി* nodep, Ast_Sen_വൃക്ഷം* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        എങ്കിൽ (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }

        {
            സ്ഥിരം VNUser1InUse user1InUse;

            // Walk sensitivity list
            m_clockedProcess = അസത്യം;
            m_allChanged = സത്യം;
            എങ്കിൽ (oldsentreep) {
                oldsentreep->unlinkFrBack();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            എങ്കിൽ (m_allChanged) {
                സ്ഥിരം VNUser2InUse user2InUse;
                m_walkingBody = സത്യം;
                m_canBeComb = സത്യം;
                iterateChildrenConst(nodep);
                m_walkingBody = അസത്യം;
                എങ്കിൽ (m_canBeComb) m_clockedProcess = അസത്യം;
            }
        }

        AstActive* സ്ഥിരം wantactivep
            = !m_clockedProcess ? m_namer.getSpecialActive<AstSenItem::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.getActive(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.getSpecialActive<AstSenItem::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlinkFrBack();
        wantactivep->addStmtsp(nodep);

        // Warn and convert any delayed assignments
        എങ്കിൽ (!m_clockedProcess) ActiveDlyVisitor{nodep, ActiveDlyVisitor::CT_COMB};

        // Delete sensitivity list
        എങ്കിൽ (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_വൃക്ഷം(), oldsentreep);

        // check combinational processes for latches
        എങ്കിൽ (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            സ്ഥിരം ActiveLatchCheckVisitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    ശൂന്യം visit_Sen_ഇനങ്ങൾ(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (v3Global.opt.timing().isSetTrue()) {
            nodep->foreach([ഇത്](AstSenItem* senItemp) { visit(senItemp); });
        }
    }

    സ്ഥിതം ശൂന്യം markEventEdges(Ast_Sen_വൃക്ഷം* sentreep) {
        വേണ്ടി (AstSenItem* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), SenItem)) {
            എങ്കിൽ (!senip->sensp()) തുടരുക;
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = senip->sensp()->dtypep()) {
                എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = dtypep->basicp()) {
                    എങ്കിൽ (basicp->isEvent()) senip->edgeType(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    ശൂന്യം visit(AstScope* nodep) override {
        m_namer.പ്രധാന(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstActive* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    ശൂന്യം visit(AstInitialStatic* nodep) override { moveUnderSpecial<AstSenItem::Static>(nodep); }
    ശൂന്യം visit(AstInitial* nodep) override {
        visit_Sen_ഇനങ്ങൾ(nodep);
        moveUnderSpecial<AstSenItem::Initial>(nodep);
    }
    ശൂന്യം visit(AstFinal* nodep) override {
        സ്ഥിരം ActiveDlyVisitor dlyvisitor{nodep, ActiveDlyVisitor::CT_FINAL};
        moveUnderSpecial<AstSenItem::Final>(nodep);
    }
    ശൂന്യം visit(AstCoverToggle* nodep) override { moveUnderSpecial<AstSenItem::Combo>(nodep); }
    ശൂന്യം visit(AstAlways* nodep) override {
        എങ്കിൽ (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }
        എങ്കിൽ (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            moveUnderSpecial<AstSenItem::Combo>(nodep);
            തിരികെ;
        }
        visit_Sen_ഇനങ്ങൾ(nodep);
        visitAlways(nodep, nodep->sentreep(), nodep->keyword());
    }
    ശൂന്യം visit(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* സ്ഥിരം activep = m_namer.makeSpecialActive<AstSenItem::Combo>(nodep->fileline());
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    ശൂന്യം visit(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* സ്ഥിരം activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    ശൂന്യം visit(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        markEventEdges(nodep->sentreep());
        Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep = nodep->sentreep();
        sentreep->unlinkFrBack();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        AstActive* സ്ഥിരം activep = m_namer.makeActive(nodep->fileline(), sentreep);
        activep->addStmtsp(nodep->unlinkFrBack());
    }
    ശൂന്യം visit(AstCFunc* nodep) override { visit_Sen_ഇനങ്ങൾ(nodep); }
    ശൂന്യം visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        എങ്കിൽ (!nodep->sensp()) തിരികെ;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = സത്യം;
        എങ്കിൽ (nodep->edgeType() != VEdgeType::ET_CHANGED) m_allChanged = അസത്യം;

        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->sensp()->dtypep()) {
            എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = dtypep->basicp()) {
                എങ്കിൽ (basicp->isEvent()) nodep->edgeType(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](സ്ഥിരം AstVarRef* refp) { refp->varScopep()->user1(സത്യം); });
    }

    ശൂന്യം visit(AstVarRef* nodep) override {
        AstVarScope* സ്ഥിരം vscp = nodep->varScopep();
        എങ്കിൽ (nodep->access().isWriteOnly()) {
            vscp->user2(സത്യം);
        } അല്ലെങ്കിൽ {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            സ്ഥിരം AstVar* സ്ഥിരം varp = vscp->varp();
            എങ്കിൽ (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = അസത്യം;
            }
        }
    }
    ശൂന്യം visit(AstAssignDly* nodep) override {
        m_canBeComb = അസത്യം;
        എങ്കിൽ (nodep->isTimingControl()) m_clockedProcess = സത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstFireEvent* nodep) override {
        m_canBeComb = അസത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstAssignForce* nodep) override {
        m_canBeComb = അസത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstRelease* nodep) override {
        m_canBeComb = അസത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstFork* nodep) override {
        എങ്കിൽ (nodep->isTimingControl()) {
            m_canBeComb = അസത്യം;
            m_clockedProcess = സത്യം;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    ശൂന്യം visit(AstVar*) override {}  // Accelerate
    ശൂന്യം visit(AstVarScope*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (!v3Global.opt.timing().isSetTrue() && m_walkingBody && !m_canBeComb) {
            തിരികെ;  // Accelerate
        }
        എങ്കിൽ (!nodep->isPure()) m_canBeComb = അസത്യം;
        എങ്കിൽ (nodep->isTimingControl()) {
            m_canBeComb = അസത്യം;
            m_clockedProcess = സത്യം;
            തിരികെ;
        }
        iterateChildren(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    explicit ActiveVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ActiveVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

വർഗ്ഗം CovergroupCollectVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = nullptr;  // Current covergroup class context, or nullptr

    // VISITORS
    ശൂന്യം visit(AstClass* nodep) override {
        എങ്കിൽ (!nodep->isCovergroup()) തിരികെ;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstScope* nodep) override { iterateChildren(nodep); }

    ശൂന്യം visit(AstCFunc* nodep) override {
        എങ്കിൽ (!m_classp) തിരികെ;
        എങ്കിൽ (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    ശൂന്യം visit(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlinkFrBack());
        nodep->unlinkFrBack();
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

വർഗ്ഗം CovergroupEventBindVisitor final : പൊതു VNVisitor {
    AstVarScope* സ്ഥിരം m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_set<സ്ഥിരം AstVar*> m_memberps;  // Non-static variables in the covergroup class

    ശൂന്യം visit(AstVarRef* nodep) override {
        എങ്കിൽ (!m_memberps.എണ്ണം(nodep->varp())) തിരികെ;
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        AstMemberSel* സ്ഥിരം selp
            = പുതിയ AstMemberSel{fl, പുതിയ AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    CovergroupEventBindVisitor(Ast_Sen_വൃക്ഷം* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* സ്ഥിരം, AstVar* സ്ഥിരം varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = സ്ഥിരസ്ഥിതി;
};

വർഗ്ഗം CovergroupInjectVisitor final : പൊതു VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    ActiveNamer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    ശൂന്യം visit(AstScope* nodep) override {
        m_namer.പ്രധാന(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* സ്ഥിരം varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        സ്ഥിരം AstClassRefDType* സ്ഥിരം classRefp = VN_CAST(dtypep, ClassRefDType);
        എങ്കിൽ (!classRefp) തിരികെ;

        AstClass* സ്ഥിരം classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_വൃക്ഷം* സ്ഥിരം eventp = VN_CAST(classp->user2p(), Sen_വൃക്ഷം);
        എങ്കിൽ (!eventp) തിരികെ;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* സ്ഥിരം sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        AstVarRef* സ്ഥിരം varrefp = പുതിയ AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* സ്ഥിരം cmethodCallp
            = പുതിയ AstCMethodCall{fl, varrefp, sampleCFuncp, nullptr};

        cmethodCallp->dtypeSetVoid();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_വൃക്ഷം* senTreep = eventp->clone_വൃക്ഷം(അസത്യം);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        AstActive* സ്ഥിരം activep = m_namer.getActive(fl, senTreep);
        VL_DO_DANGLING(തള്ളുക_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->addStmtsp(
            പുതിയ AstAlways{fl, VAlwaysKwd::ALWAYS_FF, nullptr, cmethodCallp->makeStmt()});
    }

    ശൂന്യം visit(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        എങ്കിൽ (Ast_Sen_വൃക്ഷം* സ്ഥിരം eventp = VN_CAST(nodep->user2p(), Sen_വൃക്ഷം)) {
            VL_DO_DANGLING(തള്ളുക_Deletep(eventp), eventp);
        }
    }

    ശൂന്യം visit(AstActive*) override {}  // Don't iterate into actives
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Active class functions

ശൂന്യം V3Active::activeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ActiveVisitor{nodep}; }  // Destruct before checking
    എങ്കിൽ (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        സ്ഥിരം VNUser1InUse user1InUse;
        സ്ഥിരം VNUser2InUse user2InUse;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_വൃക്ഷം("active", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
