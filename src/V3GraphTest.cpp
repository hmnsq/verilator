// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Graph tests
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

#define VL_MT_DISABLED_CODE_UNIT 1

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

வகுப்பு V3GraphTest VL_NOT_FINAL {
பாதுகாக்கப்பட்ட:
    // MEMBERS
    V3Graph m_graph;

    // METHODS - for children
    மெய்நிகர் வெறுமை runTest() = 0;  // Run the test
    மெய்நிகர் string name() = 0;  // Name of the test

    // Utilities
    வெறுமை dumpSelf() {
        எனில் (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("v3graphtest_" + name());
    }

பொது:
    V3GraphTest() = இயல்பு;
    மெய்நிகர் ~V3GraphTest() = இயல்பு;

    வெறுமை run() { runTest(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

வகுப்பு V3GraphTestVertex VL_NOT_FINAL : பொது V3GraphVertex {
    VL_RTTI_IMPL(V3GraphTestVertex, V3GraphVertex)
    மாறிலி string m_name;

பொது:
    V3GraphTestVertex(V3Graph* graphp, மாறிலி string& name)
        : V3GraphVertex{graphp}
        , m_name{name} {}
    ~V3GraphTestVertex() override = இயல்பு;
    // ACCESSORS
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
};

வகுப்பு V3GraphTestVarVertex final : பொது V3GraphTestVertex {
பொது:
    V3GraphTestVarVertex(V3Graph* graphp, மாறிலி string& name)
        : V3GraphTestVertex{graphp, name} {}
    ~V3GraphTestVarVertex() override = இயல்பு;
    // ACCESSORS
    string dotColor() மாறிலி override { திரும்பு "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

வகுப்பு V3GraphTestStrong final : பொது V3GraphTest {
பொது:
    string name() override { திரும்பு "strong"; }
    வெறுமை runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = புதிய V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = புதிய V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = புதிய V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = புதிய V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = புதிய V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = புதிய V3GraphTestVarVertex{gp, "g3"};
        V3GraphTestVertex* q = புதிய V3GraphTestVarVertex{gp, "q"};
        புதிய V3GraphEdge{gp, i, a, 2, உண்மை};
        புதிய V3GraphEdge{gp, a, b, 2, உண்மை};
        புதிய V3GraphEdge{gp, b, g1, 2, உண்மை};
        புதிய V3GraphEdge{gp, b, g2, 2, உண்மை};
        புதிய V3GraphEdge{gp, b, g3, 2, உண்மை};
        புதிய V3GraphEdge{gp, g1, a, 2, உண்மை};
        புதிய V3GraphEdge{gp, g3, g2, 2, உண்மை};
        புதிய V3GraphEdge{gp, g2, g3, 2, உண்மை};
        புதிய V3GraphEdge{gp, g1, q, 2, உண்மை};
        புதிய V3GraphEdge{gp, g2, q, 2, உண்மை};
        புதிய V3GraphEdge{gp, g3, q, 2, உண்மை};

        gp->stronglyConnected(&V3GraphEdge::followAlwaysTrue);
        dumpSelf();

        UASSERT(i->color() != a->color() && a->color() != g2->color() && g2->color() != q->color(),
                "SelfTest: Separate colors not assigned");
        UASSERT(a->color() == b->color() && a->color() == g1->color(),
                "SelfTest: Strongly connected nodes not colored together");
        UASSERT(g2->color() == g3->color(),
                "SelfTest: Strongly connected nodes not colored together");
    }
};

வகுப்பு V3GraphTestAcyc final : பொது V3GraphTest {
பொது:
    string name() override { திரும்பு "acyc"; }
    வெறுமை runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = புதிய V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = புதிய V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = புதிய V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = புதிய V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = புதிய V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = புதிய V3GraphTestVarVertex{gp, "g3"};
        புதிய V3GraphEdge{gp, i, a, 2, உண்மை};
        புதிய V3GraphEdge{gp, a, b, 2, உண்மை};
        புதிய V3GraphEdge{gp, b, g1, 2, உண்மை};
        புதிய V3GraphEdge{gp, b, g2, 2, உண்மை};
        புதிய V3GraphEdge{gp, b, g3, 2, உண்மை};
        புதிய V3GraphEdge{gp, g1, a, 2, உண்மை};
        புதிய V3GraphEdge{gp, g2, a, 2, உண்மை};
        புதிய V3GraphEdge{gp, g3, a, 2, உண்மை};

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();
        dumpSelf();
    }
};

வகுப்பு V3GraphTestVars final : பொது V3GraphTest {
பொது:
    string name() override { திரும்பு "vars"; }
    வெறுமை runTest() override {
        V3Graph* மாறிலி gp = &m_graph;

        V3GraphTestVertex* மாறிலி clk = புதிய V3GraphTestVarVertex{gp, "$clk"};

        V3GraphTestVertex* மாறிலி a = புதிய V3GraphTestVarVertex{gp, "$a"};
        V3GraphTestVertex* மாறிலி a_dly = புதிய V3GraphTestVarVertex{gp, "$a_dly"};
        V3GraphTestVertex* மாறிலி a_dlyblk = புதிய V3GraphTestVarVertex{gp, "$a_dlyblk"};
        V3GraphTestVertex* மாறிலி b = புதிய V3GraphTestVarVertex{gp, "$b"};
        V3GraphTestVertex* மாறிலி b_dly = புதிய V3GraphTestVarVertex{gp, "$b_dly"};
        V3GraphTestVertex* மாறிலி b_dlyblk = புதிய V3GraphTestVarVertex{gp, "$b_dlyblk"};
        V3GraphTestVertex* மாறிலி c = புதிய V3GraphTestVarVertex{gp, "$c"};
        V3GraphTestVertex* மாறிலி i = புதிய V3GraphTestVarVertex{gp, "$i"};

        V3GraphTestVertex* மாறிலி ap = புதிய V3GraphTestVarVertex{gp, "$a_pre"};
        V3GraphTestVertex* மாறிலி bp = புதிய V3GraphTestVarVertex{gp, "$b_pre"};
        V3GraphTestVertex* மாறிலி cp = புதிய V3GraphTestVarVertex{gp, "$c_pre"};

        V3GraphTestVertex* n;

        // Logical order between clk, and posedge blocks
        //   implemented by special CLK prod/cons?
        // Required order between first x_DLY<=x_pre and final x<=x_DLY
        //   implemented by producer/consumer on a_dly signals
        // Required order between first x_DLY<=x_pre and x_DLY<=setters
        //   implemented by fake dependency on _dlyblk
        // Required order between x_DLY<=setters and final x<=x_DLY
        //   implemented by producer/consumer on a_dly signals
        // Desired order between different _DLY blocks so we can elim temporaries
        //   implemented by cutable "pre" signal dependencies

        n = புதிய V3GraphTestVertex{gp, "*INPUTS*"};
        {
            புதிய V3GraphEdge{gp, n, clk, 2};
            புதிய V3GraphEdge{gp, n, i, 2};
        }

        V3GraphTestVertex* மாறிலி posedge = n = புதிய V3GraphTestVertex{gp, "*posedge clk*"};
        { புதிய V3GraphEdge{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = புதிய V3GraphTestVertex{gp, "a_dly<PRE=a"};
        {
            புதிய V3GraphEdge{gp, n, a_dlyblk, 2};  // Block ordering
            புதிய V3GraphEdge{gp, n, a_dly, 2};
            புதிய V3GraphEdge{gp, ap, n, 2, உண்மை};  // DESIRED delayed ordering (inp is required)
            புதிய V3GraphEdge{gp, posedge, n, 2};
        }
        n = புதிய V3GraphTestVertex{gp, "b_dly<PRE=b"};
        {
            புதிய V3GraphEdge{gp, n, b_dlyblk, 2};  // Block ordering
            புதிய V3GraphEdge{gp, n, b_dly, 2};
            புதிய V3GraphEdge{gp, bp, n, 2, உண்மை};  // DESIRED delayed ordering
            புதிய V3GraphEdge{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = புதிய V3GraphTestVertex{gp, "a_dly<=b|c"};
        {
            புதிய V3GraphEdge{gp, a_dlyblk, n, 2};  // Block ordering in
            புதிய V3GraphEdge{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            புதிய V3GraphEdge{gp, n, bp, 2};  // DESIRED delayed usage
            புதிய V3GraphEdge{gp, n, cp, 2};  // DESIRED delayed usage
            புதிய V3GraphEdge{gp, posedge, n, 2};
        }
        n = புதிய V3GraphTestVertex{gp, "b_dly<=a"};
        {
            புதிய V3GraphEdge{gp, b_dlyblk, n, 2};  // Block ordering in
            புதிய V3GraphEdge{gp, n, b_dly, 2};
            புதிய V3GraphEdge{gp, n, ap, 2};  // DESIRED delayed usage
            புதிய V3GraphEdge{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = புதிய V3GraphTestVertex{gp, "a=POST=a_dly"};
        {
            புதிய V3GraphEdge{gp, n, a, 3};
            புதிய V3GraphEdge{gp, a_dly, n, 3};
            புதிய V3GraphEdge{gp, posedge, n, 2};
        }
        n = புதிய V3GraphTestVertex{gp, "b=POST=b_dly"};
        {
            புதிய V3GraphEdge{gp, n, b, 3};
            புதிய V3GraphEdge{gp, b_dly, n, 3};
            புதிய V3GraphEdge{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = புதிய V3GraphTestVertex{gp, "c=a|b|i"};
            புதிய V3GraphEdge{gp, n, c, 1, உண்மை};
            புதிய V3GraphEdge{gp, a, n, 1, பொய்};
            புதிய V3GraphEdge{gp, b, n, 1, பொய்};
            புதிய V3GraphEdge{gp, i, n, 1, பொய்};
        }

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();

        // Test debug function
        gp->dumpEdges(std::cout, *ap);

        dumpSelf();
    }
};

//======================================================================

வகுப்பு V3GraphTestImport final : பொது V3GraphTest {

#ifdef GRAPH_IMPORT
    வெறுமை dotImport();
#இல்லையெனில்
    வெறுமை dotImport() {}
#endif

பொது:
    string name() override { திரும்பு "import"; }
    வெறுமை runTest() override {
        V3Graph* மாறிலி gp = &m_graph;
        dotImport();
        dumpSelf();
        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        dumpSelf();
        gp->rank(&V3GraphEdge::followAlwaysTrue);
        dumpSelf();
    }
};

// clang-format off
#ifdef GRAPH_IMPORT
# include "graph_export.cpp"
#endif
// clang-format on

//======================================================================

வெறுமை V3Graph::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3GraphTestStrong{}.run(); }
    { V3GraphTestAcyc{}.run(); }
    { V3GraphTestVars{}.run(); }
    { V3GraphTestImport{}.run(); }
}
