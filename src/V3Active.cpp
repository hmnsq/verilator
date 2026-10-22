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
    गणना VertexType : uint8_t { VT_खण्डम्, VT_BRANCH, VT_OUTPUT };

गुप्त:
    ध्रुव string m_name;  // Only used for .dot file generation
    ध्रुव VertexType m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ध्रुव VL_MT_SAFE {  //   "
        विकल्प (m_type) {
        पक्ष VT_खण्डम्: प्रत्यागम "(||)";  // basic block node
        पक्ष VT_BRANCH: प्रत्यागम "(&&)";  // if/else branch mode
        पक्ष VT_OUTPUT: प्रत्यागम "(out)";  // var assignment
        स्वभाव: प्रत्यागम "??";  // unknown
        }
    }

सार्वजनिक:
    LatchDetectGraphVertex(V3Graph* graphp, ध्रुव string& name, VertexType type = VT_खण्डम्)
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
    LatchDetectGraphVertex* m_curVertexp = शून्यसूचकः;  // Current latch detection graph vertex
    std::सदिशम्<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    स्थिर LatchDetectGraphVertex* castVertexp(शून्य* vertexp) {
        प्रत्यागम reinterpret_cast<LatchDetectGraphVertex*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    तार्किक latch_Check_आन्तरिकम्(LatchDetectGraphVertex* vertexp) {
        तार्किक परिणाम = असत्य;
        विकल्प (vertexp->type()) {
        पक्ष LatchDetectGraphVertex::VT_OUTPUT:  // Base case
            परिणाम = vertexp->user();
            भङ्ग;
        पक्ष LatchDetectGraphVertex::VT_खण्डम्:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            प्रति (V3GraphEdge& edge : vertexp->बहिर्गमनम्_धाराः()) {
                यदि (latch_Check_आन्तरिकम्(castVertexp(edge.शिखरम्()))) {
                    परिणाम = सत्य;
                    भङ्ग;
                }
            }
            भङ्ग;
        पक्ष LatchDetectGraphVertex::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            LatchDetectGraphVertex* ध्रुव ifp = castVertexp(vertexp->बहिर्गमनम्_धाराः().frontp()->शिखरम्());
            LatchDetectGraphVertex* ध्रुव elsp = castVertexp(vertexp->बहिर्गमनम्_धाराः().backp()->शिखरम्());
            परिणाम = latch_Check_आन्तरिकम्(ifp) && latch_Check_आन्तरिकम्(elsp);
            भङ्ग;
        }
        vertexp->user(परिणाम);
        प्रत्यागम परिणाम;
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
        m_curVertexp = शून्यसूचकः;
    }
    // Add a new control path and connect it to its parent
    LatchDetectGraphVertex* सङ्कलय_मार्गः_Vertex(LatchDetectGraphVertex* parent, ध्रुव string& name,
                                          तार्किक branch = असत्य) {
        m_curVertexp = नूतन LatchDetectGraphVertex{एतत्, name,
                                                  branch ? LatchDetectGraphVertex::VT_BRANCH
                                                         : LatchDetectGraphVertex::VT_खण्डम्};
        नूतन V3GraphEdge{एतत्, parent, m_curVertexp, 1};
        प्रत्यागम m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    LatchDetectGraphVertex* सङ्कलय_Output_Vertex(AstVarRef* nodep) {
        LatchDetectGraphVertex* ध्रुव बहिर्गमनम्_Vertexp
            = नूतन LatchDetectGraphVertex{एतत्, nodep->name(), LatchDetectGraphVertex::VT_OUTPUT};
        nodep->varp()->user1p(बहिर्गमनम्_Vertexp);
        m_outputs.प्रक्षिप_पृष्ठभागः(nodep);
        प्रत्यागम बहिर्गमनम्_Vertexp;
    }
    // Connect an output assignment to its parent control block
    शून्य सङ्कलय_Assignment(AstVarRef* nodep) {
        LatchDetectGraphVertex* बहिर्गमनम्_Vertexp;
        यदि (!nodep->varp()->user1p()) {  // Not seen this output before
            बहिर्गमनम्_Vertexp = सङ्कलय_Output_Vertex(nodep);
        } अन्यथा {
            बहिर्गमनम्_Vertexp = castVertexp(nodep->varp()->user1p());
        }
        नूतन V3GraphEdge{एतत्, m_curVertexp, बहिर्गमनम्_Vertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    शून्य latchCheck(Ast_ग्रन्थिः* nodep, तार्किक latch_expected) {
        तार्किक latch_detected = असत्य;
        प्रति (ध्रुव AstVarRef* ध्रुव vrp : m_outputs) {
            LatchDetectGraphVertex* ध्रुव vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(सत्य);  // Identify the output vertex we are checking paths _to_
            यदि (!latch_Check_आन्तरिकम्(castVertexp(vertices().frontp()))) latch_detected = सत्य;
            यदि (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warn_अधिकम्()
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

वर्ग सक्रिय_Namer final : सार्वजनिक VNVisitor {
    // STATE
    AstScope* m_scopep = शून्यसूचकः;  // Current scope to add statement to
    Ast_सक्रिय* m_sActivep = शून्यसूचकः;  // For current scope, the Static active we're building
    Ast_सक्रिय* m_iActivep = शून्यसूचकः;  // For current scope, the Initial active we're building
    Ast_सक्रिय* m_fActivep = शून्यसूचकः;  // For current scope, the Final active we're building
    Ast_सक्रिय* m_cActivep = शून्यसूचकः;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_वृक्षः>, Ast_सक्रिय*> m_सक्रिय_Map;

    // METHODS
    शून्य सङ्कलय_सक्रिय(Ast_सक्रिय* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->सङ्कलय_Blocksp(nodep);
    }

    // VISITORS
    शून्य दर्शय(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = शून्यसूचकः;
        m_iActivep = शून्यसूचकः;
        m_fActivep = शून्यसूचकः;
        m_cActivep = शून्यसूचकः;
        m_सक्रिय_Map.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    शून्य दर्शय(Ast_Sen_वृक्षः* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt*) override {}  // Accelerate
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    प्रतिरूपम् <प्रकारनाम T_Sen_Item_विधा>
    Ast_सक्रिय*& get_Special_सक्रिय();

सार्वजनिक:
    // METHODS
    AstScope* scopep() { प्रत्यागम m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    Ast_सक्रिय* make_सक्रिय(File_पङ्क्तिः* ध्रुव fl, Ast_Sen_वृक्षः* ध्रुव senTreep) {
        Ast_सक्रिय* ध्रुव activep = नूतन Ast_सक्रिय{fl, "", senTreep};
        activep->sen_वृक्षः_Storep(activep->sentreep());
        सङ्कलय_सक्रिय(activep);
        प्रत्यागम activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    प्रतिरूपम् <प्रकारनाम T_Sen_Item_विधा>
    Ast_सक्रिय* make_Special_सक्रिय(File_पङ्क्तिः* ध्रुव fl) {
        Ast_Sen_वृक्षः* ध्रुव senTreep = नूतन Ast_Sen_वृक्षः{fl, नूतन AstSenItem{fl, T_Sen_Item_विधा{}}};
        प्रत्यागम make_सक्रिय(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    प्रतिरूपम् <प्रकारनाम T_Sen_Item_विधा>
    Ast_सक्रिय* get_Special_सक्रिय(File_पङ्क्तिः* fl) {
        Ast_सक्रिय*& cachep = get_Special_सक्रिय<T_Sen_Item_विधा>();
        यदि (!cachep) cachep = make_Special_सक्रिय<T_Sen_Item_विधा>(fl);
        प्रत्यागम cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    Ast_सक्रिय* get_सक्रिय(File_पङ्क्तिः* fl, Ast_Sen_वृक्षः* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        स्वचालित it = m_सक्रिय_Map.find(*sentreep);
        // If found matching AstActive, return it
        यदि (it != m_सक्रिय_Map.अन्तः_2()) प्रत्यागम it->second;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_वृक्षः* ध्रुव newsenp = sentreep->clone_वृक्षः(असत्य);
        Ast_सक्रिय* ध्रुव activep = नूतन Ast_सक्रिय{fl, "sequent", newsenp};
        activep->sen_वृक्षः_Storep(activep->sentreep());
        सङ्कलय_सक्रिय(activep);
        m_सक्रिय_Map.emplace(*newsenp, activep);
        प्रत्यागम activep;
    }

    // CONSTRUCTORS
    सक्रिय_Namer() = स्वभाव;
    ~सक्रिय_Namer() override = स्वभाव;
    शून्य मुख्यम्(AstScope* nodep) { iterate(nodep); }
};

प्रतिरूपम् <>
Ast_सक्रिय*& सक्रिय_Namer::get_Special_सक्रिय<AstSenItem::Static>() {
    प्रत्यागम m_sActivep;
}
प्रतिरूपम् <>
Ast_सक्रिय*& सक्रिय_Namer::get_Special_सक्रिय<AstSenItem::Initial>() {
    प्रत्यागम m_iActivep;
}
प्रतिरूपम् <>
Ast_सक्रिय*& सक्रिय_Namer::get_Special_सक्रिय<AstSenItem::Final>() {
    प्रत्यागम m_fActivep;
}
प्रतिरूपम् <>
Ast_सक्रिय*& सक्रिय_Namer::get_Special_सक्रिय<AstSenItem::Combo>() {
    प्रत्यागम m_cActivep;
}

//######################################################################
// Latch checking visitor

वर्ग सक्रिय_Latch_Check_Visitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;
    // STATE
    LatchDetectGraph m_graph;  // Graph used to detect latches in combo always
    // VISITORS
    शून्य दर्शय(AstVarRef* nodep) override {
        ध्रुव AstVar* ध्रुव varp = nodep->varp();
        यदि (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->is_प्रयुक्तम्_Loop_Idx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_graph.सङ्कलय_Assignment(nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        यदि (!nodep->isBoundsCheck()) {
            LatchDetectGraphVertex* ध्रुव parentp = m_graph.currentp();
            LatchDetectGraphVertex* ध्रुव branchp = m_graph.सङ्कलय_मार्गः_Vertex(parentp, "BRANCH", सत्य);
            m_graph.सङ्कलय_मार्गः_Vertex(branchp, "IF");
            iterate_And_अग्र_Const_Null(nodep->thensp());
            m_graph.सङ्कलय_मार्गः_Vertex(branchp, "ELSE");
            iterate_And_अग्र_Const_Null(nodep->elsesp());
            m_graph.currentp(parentp);
        } अन्यथा {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    सक्रिय_Latch_Check_Visitor(Ast_ग्रन्थिः* nodep, तार्किक expectLatch) {
        m_graph.begin();
        iterateConst(nodep);
        m_graph.latchCheck(nodep, expectLatch);
    }
    ~सक्रिय_Latch_Check_Visitor() override = स्वभाव;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

वर्ग सक्रिय_Dly_Visitor final : सार्वजनिक VNVisitor {
सार्वजनिक:
    गणना CheckType : uint8_t { CT_COMB, CT_FINAL };

गुप्त:
    // MEMBERS
    ध्रुव CheckType m_check;  // Process type we are checking

    // VISITORS
    शून्य दर्शय(Ast_नियोजय_Dly* nodep) override {
        // Issue appropriate warning
        यदि (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } अन्यथा {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warn_अधिकम्()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(नूतन Ast_नियोजय{
            nodep->fileline(),  //
            nodep->lhsp()->unlink_Fr_पृष्ठभागः(),  //
            nodep->rhsp()->unlink_Fr_पृष्ठभागः(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlink_Fr_पृष्ठभागः() : शून्यसूचकः});
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }

    //--------------------
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    सक्रिय_Dly_Visitor(Ast_ग्रन्थिः* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~सक्रिय_Dly_Visitor() override = स्वभाव;
};

//######################################################################
// Active class functions

वर्ग सक्रिय_Visitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    सक्रिय_Namer m_namer;  // Tracking of active names
    तार्किक m_clockedProcess = असत्य;  // Whether current process is a clocked process
    तार्किक m_all_परिवर्तितम् = असत्य;  // Whether all SenItem in the SenTree are ET_CHANGED
    तार्किक m_walkingBody = असत्य;  // Walking body of a process
    तार्किक m_canBeComb = असत्य;  // Whether current clocked process can be turned into a comb process

    // METHODS
    प्रतिरूपम् <प्रकारनाम T>
    शून्य moveUnderSpecial(Ast_ग्रन्थिः* nodep) {
        Ast_सक्रिय* ध्रुव wantactivep = m_namer.get_Special_सक्रिय<T>(nodep->fileline());
        nodep->unlink_Fr_पृष्ठभागः();
        wantactivep->सङ्कलय_Stmtsp(nodep);
    }

    शून्य दर्शय_Always(Ast_ग्रन्थिः* nodep, Ast_Sen_वृक्षः* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        यदि (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }

        {
            ध्रुव VNUser1_अन्तर्_Use user1_अन्तर्_Use;

            // Walk sensitivity list
            m_clockedProcess = असत्य;
            m_all_परिवर्तितम् = सत्य;
            यदि (oldsentreep) {
                oldsentreep->unlink_Fr_पृष्ठभागः();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            यदि (m_all_परिवर्तितम्) {
                ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;
                m_walkingBody = सत्य;
                m_canBeComb = सत्य;
                iterateChildrenConst(nodep);
                m_walkingBody = असत्य;
                यदि (m_canBeComb) m_clockedProcess = असत्य;
            }
        }

        Ast_सक्रिय* ध्रुव wantactivep
            = !m_clockedProcess ? m_namer.get_Special_सक्रिय<AstSenItem::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.get_सक्रिय(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.get_Special_सक्रिय<AstSenItem::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlink_Fr_पृष्ठभागः();
        wantactivep->सङ्कलय_Stmtsp(nodep);

        // Warn and convert any delayed assignments
        यदि (!m_clockedProcess) सक्रिय_Dly_Visitor{nodep, सक्रिय_Dly_Visitor::CT_COMB};

        // Delete sensitivity list
        यदि (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_वृक्षः(), oldsentreep);

        // check combinational processes for latches
        यदि (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ध्रुव सक्रिय_Latch_Check_Visitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    शून्य दर्शय_Sen_वस्तूनि(Ast_ग्रन्थिः* nodep) {
        यदि (v3Global.opt.timing().is_समुच्चयः_True()) {
            nodep->foreach([एतत्](AstSenItem* senItemp) { दर्शय(senItemp); });
        }
    }

    स्थिर शून्य mark_Event_धाराः(Ast_Sen_वृक्षः* sentreep) {
        प्रति (AstSenItem* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), SenItem)) {
            यदि (!senip->sensp()) अनुवर्त;
            यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = senip->sensp()->dtypep()) {
                यदि (ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp()) {
                    यदि (basicp->isEvent()) senip->edgeType(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    शून्य दर्शय(AstScope* nodep) override {
        m_namer.मुख्यम्(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_सक्रिय* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    शून्य दर्शय(AstInitialStatic* nodep) override { moveUnderSpecial<AstSenItem::Static>(nodep); }
    शून्य दर्शय(AstInitial* nodep) override {
        दर्शय_Sen_वस्तूनि(nodep);
        moveUnderSpecial<AstSenItem::Initial>(nodep);
    }
    शून्य दर्शय(AstFinal* nodep) override {
        ध्रुव सक्रिय_Dly_Visitor dlyvisitor{nodep, सक्रिय_Dly_Visitor::CT_FINAL};
        moveUnderSpecial<AstSenItem::Final>(nodep);
    }
    शून्य दर्शय(AstCoverToggle* nodep) override { moveUnderSpecial<AstSenItem::Combo>(nodep); }
    शून्य दर्शय(AstAlways* nodep) override {
        यदि (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }
        यदि (nodep->keyword() == VAlwaysKwd::CONT_नियोजय) {
            moveUnderSpecial<AstSenItem::Combo>(nodep);
            प्रत्यागम;
        }
        दर्शय_Sen_वस्तूनि(nodep);
        दर्शय_Always(nodep, nodep->sentreep(), nodep->keyword());
    }
    शून्य दर्शय(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_सक्रिय* ध्रुव activep = m_namer.make_Special_सक्रिय<AstSenItem::Combo>(nodep->fileline());
        activep->सङ्कलय_Stmtsp(nodep->unlink_Fr_पृष्ठभागः());
    }
    शून्य दर्शय(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_धाराः(nodep->sentreep());
        Ast_Sen_वृक्षः* ध्रुव sentreep = nodep->sentreep();
        sentreep->unlink_Fr_पृष्ठभागः();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_सक्रिय* ध्रुव activep = m_namer.make_सक्रिय(nodep->fileline(), sentreep);
        activep->सङ्कलय_Stmtsp(nodep->unlink_Fr_पृष्ठभागः());
    }
    शून्य दर्शय(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_धाराः(nodep->sentreep());
        Ast_Sen_वृक्षः* ध्रुव sentreep = nodep->sentreep();
        sentreep->unlink_Fr_पृष्ठभागः();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_सक्रिय* ध्रुव activep = m_namer.make_सक्रिय(nodep->fileline(), sentreep);
        activep->सङ्कलय_Stmtsp(nodep->unlink_Fr_पृष्ठभागः());
    }
    शून्य दर्शय(AstCFunc* nodep) override { दर्शय_Sen_वस्तूनि(nodep); }
    शून्य दर्शय(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        यदि (!nodep->sensp()) प्रत्यागम;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = सत्य;
        यदि (nodep->edgeType() != VEdgeType::ET_परिवर्तितम्) m_all_परिवर्तितम् = असत्य;

        यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->sensp()->dtypep()) {
            यदि (ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp()) {
                यदि (basicp->isEvent()) nodep->edgeType(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ध्रुव AstVarRef* refp) { refp->varScopep()->user1(सत्य); });
    }

    शून्य दर्शय(AstVarRef* nodep) override {
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
    शून्य दर्शय(Ast_नियोजय_Dly* nodep) override {
        m_canBeComb = असत्य;
        यदि (nodep->isTimingControl()) m_clockedProcess = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstFireEvent* nodep) override {
        m_canBeComb = असत्य;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_नियोजय_Force* nodep) override {
        m_canBeComb = असत्य;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstRelease* nodep) override {
        m_canBeComb = असत्य;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstFork* nodep) override {
        यदि (nodep->isTimingControl()) {
            m_canBeComb = असत्य;
            m_clockedProcess = सत्य;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    शून्य दर्शय(AstVar*) override {}  // Accelerate
    शून्य दर्शय(AstVarScope*) override {}  // Accelerate
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        यदि (!v3Global.opt.timing().is_समुच्चयः_True() && m_walkingBody && !m_canBeComb) {
            प्रत्यागम;  // Accelerate
        }
        यदि (!nodep->is_विशुद्धम्()) m_canBeComb = असत्य;
        यदि (nodep->isTimingControl()) {
            m_canBeComb = असत्य;
            m_clockedProcess = सत्य;
            प्रत्यागम;
        }
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट सक्रिय_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~सक्रिय_Visitor() override = स्वभाव;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

वर्ग CovergroupCollectVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = शून्यसूचकः;  // Current covergroup class context, or nullptr

    // VISITORS
    शून्य दर्शय(AstClass* nodep) override {
        यदि (!nodep->isCovergroup()) प्रत्यागम;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstScope* nodep) override { iterateChildren(nodep); }

    शून्य दर्शय(AstCFunc* nodep) override {
        यदि (!m_classp) प्रत्यागम;
        यदि (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    शून्य दर्शय(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlink_Fr_पृष्ठभागः());
        nodep->unlink_Fr_पृष्ठभागः();
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = स्वभाव;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

वर्ग CovergroupEventBindVisitor final : सार्वजनिक VNVisitor {
    AstVarScope* ध्रुव m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_समुच्चयः<ध्रुव AstVar*> m_memberps;  // Non-static variables in the covergroup class

    शून्य दर्शय(AstVarRef* nodep) override {
        यदि (!m_memberps.गणनम्(nodep->varp())) प्रत्यागम;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        AstMemberSel* ध्रुव selp
            = नूतन AstMemberSel{fl, नूतन AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

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

वर्ग CovergroupInjectVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    सक्रिय_Namer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    शून्य दर्शय(AstScope* nodep) override {
        m_namer.मुख्यम्(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ध्रुव varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ध्रुव AstClassRefDType* ध्रुव classRefp = VN_CAST(dtypep, ClassRefDType);
        यदि (!classRefp) प्रत्यागम;

        AstClass* ध्रुव classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_वृक्षः* ध्रुव eventp = VN_CAST(classp->user2p(), Sen_वृक्षः);
        यदि (!eventp) प्रत्यागम;  // No automatic sampling for this covergroup

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
            = नूतन AstCMethodCall{fl, varrefp, sampleCFuncp, शून्यसूचकः};

        cmethodCallp->dtype_समुच्चयः_Void();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_वृक्षः* senTreep = eventp->clone_वृक्षः(असत्य);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        Ast_सक्रिय* ध्रुव activep = m_namer.get_सक्रिय(fl, senTreep);
        VL_DO_DANGLING(प्रक्षिप_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->सङ्कलय_Stmtsp(
            नूतन AstAlways{fl, VAlwaysKwd::ALWAYS_FF, शून्यसूचकः, cmethodCallp->makeStmt()});
    }

    शून्य दर्शय(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        यदि (Ast_Sen_वृक्षः* ध्रुव eventp = VN_CAST(nodep->user2p(), Sen_वृक्षः)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(eventp), eventp);
        }
    }

    शून्य दर्शय(Ast_सक्रिय*) override {}  // Don't iterate into actives
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = स्वभाव;
};

//######################################################################
// Active class functions

शून्य V3_सक्रिय::सक्रिय_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { सक्रिय_Visitor{nodep}; }  // Destruct before checking
    यदि (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ध्रुव VNUser1_अन्तर्_Use user1_अन्तर्_Use;
        ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_वृक्षः("active", 0, dump_वृक्षः_Either_Level() >= 3);
}
