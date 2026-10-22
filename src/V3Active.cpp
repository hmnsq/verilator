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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Active.h"

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Graph.h"

#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

తరగతి Latch_Detect_లేఖాచిత్రం_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Latch_Detect_లేఖాచిత్రం_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
బహిరంగ:
    గణన శీర్షం_Type : uint8_t { VT_ఖండం, VT_BRANCH, VT_OUTPUT };

అంతరంగ:
    స్థిరరాశి string m_name;  // Only used for .dot file generation
    స్థిరరాశి శీర్షం_Type m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() స్థిరరాశి VL_MT_సురక్షిత {  //   "
        ఎంపిక (m_type) {
        స్థితి VT_ఖండం: తిరిగి "(||)";  // basic block node
        స్థితి VT_BRANCH: తిరిగి "(&&)";  // if/else branch mode
        స్థితి VT_OUTPUT: తిరిగి "(out)";  // var assignment
        అప్రమేయ: తిరిగి "??";  // unknown
        }
    }

బహిరంగ:
    Latch_Detect_లేఖాచిత్రం_శీర్షం(V3_లేఖాచిత్రం* graphp, స్థిరరాశి string& name, శీర్షం_Type type = VT_ఖండం)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name + " " + typestr(); }
    string dotColor() స్థిరరాశి override { తిరిగి user() ? "green" : "black"; }
    మాయా పూర్ణాంకం type() స్థిరరాశి { తిరిగి m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

తరగతి Latch_Detect_లేఖాచిత్రం final : బహిరంగ V3_లేఖాచిత్రం {
రక్షిత:
    Latch_Detect_లేఖాచిత్రం_శీర్షం* m_curVertexp = శూన్యసూచిక;  // Current latch detection graph vertex
    std::సదిశ<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    స్థిర Latch_Detect_లేఖాచిత్రం_శీర్షం* castVertexp(శూన్య* vertexp) {
        తిరిగి reinterpret_cast<Latch_Detect_లేఖాచిత్రం_శీర్షం*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    తార్కిక latch_Check_అంతర్గత(Latch_Detect_లేఖాచిత్రం_శీర్షం* vertexp) {
        తార్కిక ఫలితం = అబద్ధం;
        ఎంపిక (vertexp->type()) {
        స్థితి Latch_Detect_లేఖాచిత్రం_శీర్షం::VT_OUTPUT:  // Base case
            ఫలితం = vertexp->user();
            ఆపు;
        స్థితి Latch_Detect_లేఖాచిత్రం_శీర్షం::VT_ఖండం:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->బహిర్గతం_అంచులు()) {
                ఒకవేళ (latch_Check_అంతర్గత(castVertexp(అంచు.శిఖరం()))) {
                    ఫలితం = నిజం;
                    ఆపు;
                }
            }
            ఆపు;
        స్థితి Latch_Detect_లేఖాచిత్రం_శీర్షం::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            Latch_Detect_లేఖాచిత్రం_శీర్షం* స్థిరరాశి ifp = castVertexp(vertexp->బహిర్గతం_అంచులు().frontp()->శిఖరం());
            Latch_Detect_లేఖాచిత్రం_శీర్షం* స్థిరరాశి elsp = castVertexp(vertexp->బహిర్గతం_అంచులు().backp()->శిఖరం());
            ఫలితం = latch_Check_అంతర్గత(ifp) && latch_Check_అంతర్గత(elsp);
            ఆపు;
        }
        vertexp->user(ఫలితం);
        తిరిగి ఫలితం;
    }

బహిరంగ:
    Latch_Detect_లేఖాచిత్రం() { clear(); }
    ~Latch_Detect_లేఖాచిత్రం() override { clear(); }
    // ACCESSORS
    Latch_Detect_లేఖాచిత్రం_శీర్షం* currentp() { తిరిగి m_curVertexp; }
    శూన్య currentp(Latch_Detect_లేఖాచిత్రం_శీర్షం* శీర్షం) { m_curVertexp = శీర్షం; }
    // METHODS
    శూన్య begin() {
        // Start a new if/else tracking graph
        m_curVertexp = కొత్త Latch_Detect_లేఖాచిత్రం_శీర్షం{ఇది, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    శూన్య clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3_లేఖాచిత్రం::clear();
        m_curVertexp = శూన్యసూచిక;
    }
    // Add a new control path and connect it to its parent
    Latch_Detect_లేఖాచిత్రం_శీర్షం* కూడు_మార్గం_శీర్షం(Latch_Detect_లేఖాచిత్రం_శీర్షం* మాతృ, స్థిరరాశి string& name,
                                          తార్కిక branch = అబద్ధం) {
        m_curVertexp = కొత్త Latch_Detect_లేఖాచిత్రం_శీర్షం{ఇది, name,
                                                  branch ? Latch_Detect_లేఖాచిత్రం_శీర్షం::VT_BRANCH
                                                         : Latch_Detect_లేఖాచిత్రం_శీర్షం::VT_ఖండం};
        కొత్త V3_లేఖాచిత్రం_అంచు{ఇది, మాతృ, m_curVertexp, 1};
        తిరిగి m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    Latch_Detect_లేఖాచిత్రం_శీర్షం* కూడు_Output_శీర్షం(AstVarRef* nodep) {
        Latch_Detect_లేఖాచిత్రం_శీర్షం* స్థిరరాశి బహిర్గతం_Vertexp
            = కొత్త Latch_Detect_లేఖాచిత్రం_శీర్షం{ఇది, nodep->name(), Latch_Detect_లేఖాచిత్రం_శీర్షం::VT_OUTPUT};
        nodep->varp()->user1p(బహిర్గతం_Vertexp);
        m_outputs.నెట్టు_వెనుక(nodep);
        తిరిగి బహిర్గతం_Vertexp;
    }
    // Connect an output assignment to its parent control block
    శూన్య కూడు_Assignment(AstVarRef* nodep) {
        Latch_Detect_లేఖాచిత్రం_శీర్షం* బహిర్గతం_Vertexp;
        ఒకవేళ (!nodep->varp()->user1p()) {  // Not seen this output before
            బహిర్గతం_Vertexp = కూడు_Output_శీర్షం(nodep);
        } లేకపోతే {
            బహిర్గతం_Vertexp = castVertexp(nodep->varp()->user1p());
        }
        కొత్త V3_లేఖాచిత్రం_అంచు{ఇది, m_curVertexp, బహిర్గతం_Vertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    శూన్య latchCheck(Ast_కణుపు* nodep, తార్కిక latch_expected) {
        తార్కిక latch_detected = అబద్ధం;
        కొరకు (స్థిరరాశి AstVarRef* స్థిరరాశి vrp : m_outputs) {
            Latch_Detect_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(నిజం);  // Identify the output vertex we are checking paths _to_
            ఒకవేళ (!latch_Check_అంతర్గత(castVertexp(vertices().frontp()))) latch_detected = నిజం;
            ఒకవేళ (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warn_మరిన్ని()
                        << "... Suggest use of always_latch for intentional latches");
                ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(అబద్ధం);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        ఒకవేళ (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

తరగతి చురుకైన_Namer final : బహిరంగ VNVisitor {
    // STATE
    AstScope* m_scopep = శూన్యసూచిక;  // Current scope to add statement to
    Ast_చురుకైన* m_sActivep = శూన్యసూచిక;  // For current scope, the Static active we're building
    Ast_చురుకైన* m_iActivep = శూన్యసూచిక;  // For current scope, the Initial active we're building
    Ast_చురుకైన* m_fActivep = శూన్యసూచిక;  // For current scope, the Final active we're building
    Ast_చురుకైన* m_cActivep = శూన్యసూచిక;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_వృక్షం>, Ast_చురుకైన*> m_చురుకైన_Map;

    // METHODS
    శూన్య కూడు_చురుకైన(Ast_చురుకైన* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->కూడు_Blocksp(nodep);
    }

    // VISITORS
    శూన్య సందర్శించు(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = శూన్యసూచిక;
        m_iActivep = శూన్యసూచిక;
        m_fActivep = శూన్యసూచిక;
        m_cActivep = శూన్యసూచిక;
        m_చురుకైన_Map.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    శూన్య సందర్శించు(Ast_Sen_వృక్షం* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు_Stmt*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    మూస <రకం_పేరు T_Sen_అంశం_రకం>
    Ast_చురుకైన*& get_Special_చురుకైన();

బహిరంగ:
    // METHODS
    AstScope* scopep() { తిరిగి m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    Ast_చురుకైన* make_చురుకైన(File_పంక్తి* స్థిరరాశి fl, Ast_Sen_వృక్షం* స్థిరరాశి senTreep) {
        Ast_చురుకైన* స్థిరరాశి activep = కొత్త Ast_చురుకైన{fl, "", senTreep};
        activep->sen_వృక్షం_Storep(activep->sentreep());
        కూడు_చురుకైన(activep);
        తిరిగి activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    మూస <రకం_పేరు T_Sen_అంశం_రకం>
    Ast_చురుకైన* make_Special_చురుకైన(File_పంక్తి* స్థిరరాశి fl) {
        Ast_Sen_వృక్షం* స్థిరరాశి senTreep = కొత్త Ast_Sen_వృక్షం{fl, కొత్త Ast_Sen_అంశం{fl, T_Sen_అంశం_రకం{}}};
        తిరిగి make_చురుకైన(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    మూస <రకం_పేరు T_Sen_అంశం_రకం>
    Ast_చురుకైన* get_Special_చురుకైన(File_పంక్తి* fl) {
        Ast_చురుకైన*& cachep = get_Special_చురుకైన<T_Sen_అంశం_రకం>();
        ఒకవేళ (!cachep) cachep = make_Special_చురుకైన<T_Sen_అంశం_రకం>(fl);
        తిరిగి cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    Ast_చురుకైన* get_చురుకైన(File_పంక్తి* fl, Ast_Sen_వృక్షం* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        స్వయంచాలక it = m_చురుకైన_Map.కనుగొను(*sentreep);
        // If found matching AstActive, return it
        ఒకవేళ (it != m_చురుకైన_Map.చివర()) తిరిగి it->రెండవ;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_వృక్షం* స్థిరరాశి newsenp = sentreep->clone_వృక్షం(అబద్ధం);
        Ast_చురుకైన* స్థిరరాశి activep = కొత్త Ast_చురుకైన{fl, "sequent", newsenp};
        activep->sen_వృక్షం_Storep(activep->sentreep());
        కూడు_చురుకైన(activep);
        m_చురుకైన_Map.emplace(*newsenp, activep);
        తిరిగి activep;
    }

    // CONSTRUCTORS
    చురుకైన_Namer() = అప్రమేయ;
    ~చురుకైన_Namer() override = అప్రమేయ;
    శూన్య ప్రధాన(AstScope* nodep) { iterate(nodep); }
};

మూస <>
Ast_చురుకైన*& చురుకైన_Namer::get_Special_చురుకైన<Ast_Sen_అంశం::Static>() {
    తిరిగి m_sActivep;
}
మూస <>
Ast_చురుకైన*& చురుకైన_Namer::get_Special_చురుకైన<Ast_Sen_అంశం::Initial>() {
    తిరిగి m_iActivep;
}
మూస <>
Ast_చురుకైన*& చురుకైన_Namer::get_Special_చురుకైన<Ast_Sen_అంశం::Final>() {
    తిరిగి m_fActivep;
}
మూస <>
Ast_చురుకైన*& చురుకైన_Namer::get_Special_చురుకైన<Ast_Sen_అంశం::Combo>() {
    తిరిగి m_cActivep;
}

//######################################################################
// Latch checking visitor

తరగతి చురుకైన_Latch_Check_Visitor final : బహిరంగ VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    // STATE
    Latch_Detect_లేఖాచిత్రం m_లేఖాచిత్రం;  // Graph used to detect latches in combo always
    // VISITORS
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        స్థిరరాశి AstVar* స్థిరరాశి varp = nodep->varp();
        ఒకవేళ (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->is_వాడిన_Loop_Idx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_లేఖాచిత్రం.కూడు_Assignment(nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        ఒకవేళ (!nodep->isBoundsCheck()) {
            Latch_Detect_లేఖాచిత్రం_శీర్షం* స్థిరరాశి parentp = m_లేఖాచిత్రం.currentp();
            Latch_Detect_లేఖాచిత్రం_శీర్షం* స్థిరరాశి branchp = m_లేఖాచిత్రం.కూడు_మార్గం_శీర్షం(parentp, "BRANCH", నిజం);
            m_లేఖాచిత్రం.కూడు_మార్గం_శీర్షం(branchp, "IF");
            iterate_And_తదుపరి_Const_Null(nodep->thensp());
            m_లేఖాచిత్రం.కూడు_మార్గం_శీర్షం(branchp, "ELSE");
            iterate_And_తదుపరి_Const_Null(nodep->elsesp());
            m_లేఖాచిత్రం.currentp(parentp);
        } లేకపోతే {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    చురుకైన_Latch_Check_Visitor(Ast_కణుపు* nodep, తార్కిక expectLatch) {
        m_లేఖాచిత్రం.begin();
        iterateConst(nodep);
        m_లేఖాచిత్రం.latchCheck(nodep, expectLatch);
    }
    ~చురుకైన_Latch_Check_Visitor() override = అప్రమేయ;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

తరగతి చురుకైన_Dly_Visitor final : బహిరంగ VNVisitor {
బహిరంగ:
    గణన CheckType : uint8_t { CT_COMB, CT_FINAL };

అంతరంగ:
    // MEMBERS
    స్థిరరాశి CheckType m_check;  // Process type we are checking

    // VISITORS
    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override {
        // Issue appropriate warning
        ఒకవేళ (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } లేకపోతే {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warn_మరిన్ని()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(కొత్త Ast_కేటాయించు{
            nodep->fileline(),  //
            nodep->lhsp()->unlink_Fr_వెనుక(),  //
            nodep->rhsp()->unlink_Fr_వెనుక(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlink_Fr_వెనుక() : శూన్యసూచిక});
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }

    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    చురుకైన_Dly_Visitor(Ast_కణుపు* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~చురుకైన_Dly_Visitor() override = అప్రమేయ;
};

//######################################################################
// Active class functions

తరగతి చురుకైన_Visitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    చురుకైన_Namer m_namer;  // Tracking of active names
    తార్కిక m_clockedProcess = అబద్ధం;  // Whether current process is a clocked process
    తార్కిక m_all_మారింది = అబద్ధం;  // Whether all SenItem in the SenTree are ET_CHANGED
    తార్కిక m_walkingBody = అబద్ధం;  // Walking body of a process
    తార్కిక m_canBeComb = అబద్ధం;  // Whether current clocked process can be turned into a comb process

    // METHODS
    మూస <రకం_పేరు T>
    శూన్య moveUnderSpecial(Ast_కణుపు* nodep) {
        Ast_చురుకైన* స్థిరరాశి wantactivep = m_namer.get_Special_చురుకైన<T>(nodep->fileline());
        nodep->unlink_Fr_వెనుక();
        wantactivep->కూడు_Stmtsp(nodep);
    }

    శూన్య సందర్శించు_Always(Ast_కణుపు* nodep, Ast_Sen_వృక్షం* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        ఒకవేళ (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            తిరిగి;
        }

        {
            స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;

            // Walk sensitivity list
            m_clockedProcess = అబద్ధం;
            m_all_మారింది = నిజం;
            ఒకవేళ (oldsentreep) {
                oldsentreep->unlink_Fr_వెనుక();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            ఒకవేళ (m_all_మారింది) {
                స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
                m_walkingBody = నిజం;
                m_canBeComb = నిజం;
                iterateChildrenConst(nodep);
                m_walkingBody = అబద్ధం;
                ఒకవేళ (m_canBeComb) m_clockedProcess = అబద్ధం;
            }
        }

        Ast_చురుకైన* స్థిరరాశి wantactivep
            = !m_clockedProcess ? m_namer.get_Special_చురుకైన<Ast_Sen_అంశం::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.get_చురుకైన(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.get_Special_చురుకైన<Ast_Sen_అంశం::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlink_Fr_వెనుక();
        wantactivep->కూడు_Stmtsp(nodep);

        // Warn and convert any delayed assignments
        ఒకవేళ (!m_clockedProcess) చురుకైన_Dly_Visitor{nodep, చురుకైన_Dly_Visitor::CT_COMB};

        // Delete sensitivity list
        ఒకవేళ (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_వృక్షం(), oldsentreep);

        // check combinational processes for latches
        ఒకవేళ (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            స్థిరరాశి చురుకైన_Latch_Check_Visitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    శూన్య సందర్శించు_Sen_వస్తువులు(Ast_కణుపు* nodep) {
        ఒకవేళ (v3Global.opt.timing().is_సమితి_True()) {
            nodep->foreach([ఇది](Ast_Sen_అంశం* senItemp) { సందర్శించు(senItemp); });
        }
    }

    స్థిర శూన్య mark_Event_అంచులు(Ast_Sen_వృక్షం* sentreep) {
        కొరకు (Ast_Sen_అంశం* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), Sen_అంశం)) {
            ఒకవేళ (!senip->sensp()) కొనసాగు;
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = senip->sensp()->dtypep()) {
                ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = dtypep->basicp()) {
                    ఒకవేళ (basicp->isEvent()) senip->అంచు_Type(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstScope* nodep) override {
        m_namer.ప్రధాన(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_చురుకైన* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    శూన్య సందర్శించు(AstInitialStatic* nodep) override { moveUnderSpecial<Ast_Sen_అంశం::Static>(nodep); }
    శూన్య సందర్శించు(AstInitial* nodep) override {
        సందర్శించు_Sen_వస్తువులు(nodep);
        moveUnderSpecial<Ast_Sen_అంశం::Initial>(nodep);
    }
    శూన్య సందర్శించు(AstFinal* nodep) override {
        స్థిరరాశి చురుకైన_Dly_Visitor dlyvisitor{nodep, చురుకైన_Dly_Visitor::CT_FINAL};
        moveUnderSpecial<Ast_Sen_అంశం::Final>(nodep);
    }
    శూన్య సందర్శించు(AstCoverToggle* nodep) override { moveUnderSpecial<Ast_Sen_అంశం::Combo>(nodep); }
    శూన్య సందర్శించు(AstAlways* nodep) override {
        ఒకవేళ (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            తిరిగి;
        }
        ఒకవేళ (nodep->keyword() == VAlwaysKwd::CONT_కేటాయించు) {
            moveUnderSpecial<Ast_Sen_అంశం::Combo>(nodep);
            తిరిగి;
        }
        సందర్శించు_Sen_వస్తువులు(nodep);
        సందర్శించు_Always(nodep, nodep->sentreep(), nodep->keyword());
    }
    శూన్య సందర్శించు(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_చురుకైన* స్థిరరాశి activep = m_namer.make_Special_చురుకైన<Ast_Sen_అంశం::Combo>(nodep->fileline());
        activep->కూడు_Stmtsp(nodep->unlink_Fr_వెనుక());
    }
    శూన్య సందర్శించు(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_అంచులు(nodep->sentreep());
        Ast_Sen_వృక్షం* స్థిరరాశి sentreep = nodep->sentreep();
        sentreep->unlink_Fr_వెనుక();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_చురుకైన* స్థిరరాశి activep = m_namer.make_చురుకైన(nodep->fileline(), sentreep);
        activep->కూడు_Stmtsp(nodep->unlink_Fr_వెనుక());
    }
    శూన్య సందర్శించు(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_అంచులు(nodep->sentreep());
        Ast_Sen_వృక్షం* స్థిరరాశి sentreep = nodep->sentreep();
        sentreep->unlink_Fr_వెనుక();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_చురుకైన* స్థిరరాశి activep = m_namer.make_చురుకైన(nodep->fileline(), sentreep);
        activep->కూడు_Stmtsp(nodep->unlink_Fr_వెనుక());
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override { సందర్శించు_Sen_వస్తువులు(nodep); }
    శూన్య సందర్శించు(Ast_Sen_అంశం* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        ఒకవేళ (!nodep->sensp()) తిరిగి;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = నిజం;
        ఒకవేళ (nodep->అంచు_Type() != VEdgeType::ET_మారింది) m_all_మారింది = అబద్ధం;

        ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->sensp()->dtypep()) {
            ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = dtypep->basicp()) {
                ఒకవేళ (basicp->isEvent()) nodep->అంచు_Type(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](స్థిరరాశి AstVarRef* refp) { refp->varScopep()->user1(నిజం); });
    }

    శూన్య సందర్శించు(AstVarRef* nodep) override {
        AstVarScope* స్థిరరాశి vscp = nodep->varScopep();
        ఒకవేళ (nodep->access().isWriteOnly()) {
            vscp->user2(నిజం);
        } లేకపోతే {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            స్థిరరాశి AstVar* స్థిరరాశి varp = vscp->varp();
            ఒకవేళ (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = అబద్ధం;
            }
        }
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override {
        m_canBeComb = అబద్ధం;
        ఒకవేళ (nodep->isTimingControl()) m_clockedProcess = నిజం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstFireEvent* nodep) override {
        m_canBeComb = అబద్ధం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_Force* nodep) override {
        m_canBeComb = అబద్ధం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstRelease* nodep) override {
        m_canBeComb = అబద్ధం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        ఒకవేళ (nodep->isTimingControl()) {
            m_canBeComb = అబద్ధం;
            m_clockedProcess = నిజం;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    శూన్య సందర్శించు(AstVar*) override {}  // Accelerate
    శూన్య సందర్శించు(AstVarScope*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (!v3Global.opt.timing().is_సమితి_True() && m_walkingBody && !m_canBeComb) {
            తిరిగి;  // Accelerate
        }
        ఒకవేళ (!nodep->is_శుద్ధ()) m_canBeComb = అబద్ధం;
        ఒకవేళ (nodep->isTimingControl()) {
            m_canBeComb = అబద్ధం;
            m_clockedProcess = నిజం;
            తిరిగి;
        }
        iterateChildren(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన చురుకైన_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~చురుకైన_Visitor() override = అప్రమేయ;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

తరగతి CovergroupCollectVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = శూన్యసూచిక;  // Current covergroup class context, or nullptr

    // VISITORS
    శూన్య సందర్శించు(AstClass* nodep) override {
        ఒకవేళ (!nodep->isCovergroup()) తిరిగి;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstScope* nodep) override { iterateChildren(nodep); }

    శూన్య సందర్శించు(AstCFunc* nodep) override {
        ఒకవేళ (!m_classp) తిరిగి;
        ఒకవేళ (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    శూన్య సందర్శించు(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlink_Fr_వెనుక());
        nodep->unlink_Fr_వెనుక();
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = అప్రమేయ;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

తరగతి CovergroupEventBindVisitor final : బహిరంగ VNVisitor {
    AstVarScope* స్థిరరాశి m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_సమితి<స్థిరరాశి AstVar*> m_memberps;  // Non-static variables in the covergroup class

    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (!m_memberps.లెక్క(nodep->varp())) తిరిగి;
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        AstMemberSel* స్థిరరాశి selp
            = కొత్త AstMemberSel{fl, కొత్త AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    CovergroupEventBindVisitor(Ast_Sen_వృక్షం* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* స్థిరరాశి, AstVar* స్థిరరాశి varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = అప్రమేయ;
};

తరగతి CovergroupInjectVisitor final : బహిరంగ VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    చురుకైన_Namer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    శూన్య సందర్శించు(AstScope* nodep) override {
        m_namer.ప్రధాన(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* స్థిరరాశి varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        స్థిరరాశి AstClassRefDType* స్థిరరాశి classRefp = VN_CAST(dtypep, ClassRefDType);
        ఒకవేళ (!classRefp) తిరిగి;

        AstClass* స్థిరరాశి classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_వృక్షం* స్థిరరాశి eventp = VN_CAST(classp->user2p(), Sen_వృక్షం);
        ఒకవేళ (!eventp) తిరిగి;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* స్థిరరాశి sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        AstVarRef* స్థిరరాశి varrefp = కొత్త AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* స్థిరరాశి cmethodCallp
            = కొత్త AstCMethodCall{fl, varrefp, sampleCFuncp, శూన్యసూచిక};

        cmethodCallp->dtype_సమితి_Void();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_వృక్షం* senTreep = eventp->clone_వృక్షం(అబద్ధం);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        Ast_చురుకైన* స్థిరరాశి activep = m_namer.get_చురుకైన(fl, senTreep);
        VL_DO_DANGLING(నెట్టు_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->కూడు_Stmtsp(
            కొత్త AstAlways{fl, VAlwaysKwd::ALWAYS_FF, శూన్యసూచిక, cmethodCallp->makeStmt()});
    }

    శూన్య సందర్శించు(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        ఒకవేళ (Ast_Sen_వృక్షం* స్థిరరాశి eventp = VN_CAST(nodep->user2p(), Sen_వృక్షం)) {
            VL_DO_DANGLING(నెట్టు_Deletep(eventp), eventp);
        }
    }

    శూన్య సందర్శించు(Ast_చురుకైన*) override {}  // Don't iterate into actives
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = అప్రమేయ;
};

//######################################################################
// Active class functions

శూన్య V3_చురుకైన::చురుకైన_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { చురుకైన_Visitor{nodep}; }  // Destruct before checking
    ఒకవేళ (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
        స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_వృక్షం("active", 0, dump_వృక్షం_Either_Level() >= 3);
}
